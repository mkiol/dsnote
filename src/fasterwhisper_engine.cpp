/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "fasterwhisper_engine.hpp"

#include <dlfcn.h>
#include <pybind11/numpy.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>

#include "cpu_tools.hpp"
#include "gpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"
#include "text_tools.hpp"

using namespace pybind11::literals;

fasterwhisper_engine::fasterwhisper_engine(config_t config,
                                           callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    m_speech_buf.reserve(m_speech_max_size);
}

fasterwhisper_engine::~fasterwhisper_engine() {
    LOGD("fasterwhisper dtor");

    stop();  
}

void fasterwhisper_engine::stop() {
    stt_engine::stop();

    if (m_model) {
        auto task = py_executor::instance()->execute([&]() {
            try {
                m_model.reset();
            } catch (const std::exception& err) {
                LOGE("py error: " << err.what());
            }
            return std::any{};
        });

        if (task) task->get();
    }

    LOGD("fasterwhisper stopped");
}

void fasterwhisper_engine::push_buf_to_whisper_buf(
    const std::vector<in_buf_t::buf_t::value_type>& buf,
    whisper_buf_t& whisper_buf) {
    // convert s16 to f32 sample format
    std::transform(buf.cbegin(), buf.cend(), std::back_inserter(whisper_buf),
                   [](auto sample) {
                       return static_cast<whisper_buf_t::value_type>(sample) /
                              32768.0F;
                   });
}

void fasterwhisper_engine::push_buf_to_whisper_buf(
    in_buf_t::buf_t::value_type* data, in_buf_t::buf_t::size_type size,
    whisper_buf_t& whisper_buf) {
    // convert s16 to f32 sample format
    whisper_buf.reserve(whisper_buf.size() + size);
    for (size_t i = 0; i < size; ++i) {
        whisper_buf.push_back(static_cast<whisper_buf_t::value_type>(data[i]) /
                              32768.0F);
    }
}

void fasterwhisper_engine::reset_impl() { m_speech_buf.clear(); }

void fasterwhisper_engine::stop_processing_impl() {
    LOGD("fasterwhisper cancel");
    // not implemented
}

void fasterwhisper_engine::start_processing_impl() { create_model(); }

void fasterwhisper_engine::create_model() {
    if (m_model) return;

    LOGD("creating fasterwhisper model");

    auto task = py_executor::instance()->execute([&]() {
        auto n_threads = std::min(
            m_threads,
            std::max(1, static_cast<int>(std::thread::hardware_concurrency())));
        auto use_cuda = m_config.use_gpu &&
                        m_config.gpu_device.api == gpu_api_t::cuda &&
                        gpu_tools::has_cudnn();

        LOGD("cpu info: arch=" << cpu_tools::arch() << ", cores="
                               << std::thread::hardware_concurrency());
        LOGD("using threads: " << n_threads << "/"
                               << std::thread::hardware_concurrency());
        LOGD("using device: " << (use_cuda ? "cuda" : "cpu") << " "
                              << m_config.gpu_device.id);

        try {
            auto fw = py::module_::import("faster_whisper");

            m_model.emplace(fw.attr("WhisperModel")(
                "model_size_or_path"_a = m_config.model_files.model_file,
                "device"_a = use_cuda ? "cuda" : "cpu",
                "device_index"_a = use_cuda ? m_config.gpu_device.id : 0,
                "local_files_only"_a = true, "cpu_threads"_a = n_threads));
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            m_model.reset();
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get())) {
        LOGE("failed to create fasterwhisper model");
        throw std::runtime_error{"failed to create fasterwhisper model"};
    }

    LOGD("fasterwhisper model created");
}

stt_engine::samples_process_result_t fasterwhisper_engine::process_buff() {
    if (!lock_buff_for_processing())
        return samples_process_result_t::wait_for_samples;

    auto eof = m_in_buf.eof;
    auto sof = m_in_buf.sof;

    LOGD("process samples buf: mode="
         << m_config.speech_mode << ", in-buf size=" << m_in_buf.size
         << ", speech-buf size=" << m_speech_buf.size() << ", sof=" << sof
         << ", eof=" << eof);

    if (sof) {
        m_speech_buf.clear();
        m_start_time.reset();
        m_vad.reset();
        reset_segment_counters();
    }

    m_denoiser.process(m_in_buf.buf.data(), m_in_buf.size);

    const auto& vad_buf =
        m_vad.remove_silence(m_in_buf.buf.data(), m_in_buf.size);

    bool vad_status = !vad_buf.empty();

    if (vad_status) {
        LOGD("vad: speech detected");

        if (m_config.speech_mode != speech_mode_t::manual &&
            m_config.speech_mode != speech_mode_t::single_sentence)
            set_speech_detection_status(
                speech_detection_status_t::speech_detected);

        if (m_config.text_format == text_format_t::raw)
            push_buf_to_whisper_buf(vad_buf, m_speech_buf);
        else
            push_buf_to_whisper_buf(m_in_buf.buf.data(), m_in_buf.size,
                                    m_speech_buf);

        restart_sentence_timer();
    } else {
        LOGD("vad: no speech");

        if (m_config.speech_mode == speech_mode_t::single_sentence &&
            m_speech_buf.empty() && sentence_timer_timed_out()) {
            LOGD("sentence timeout");
            m_call_backs.sentence_timeout();
        }

        if (m_config.speech_mode == speech_mode_t::automatic)
            set_speech_detection_status(speech_detection_status_t::no_speech);

        if (m_speech_buf.empty())
            m_segment_time_discarded_before +=
                (1000 * m_in_buf.size) / m_sample_rate;
        else
            m_segment_time_discarded_after +=
                (1000 * m_in_buf.size) / m_sample_rate;
    }

    m_in_buf.clear();

    auto decode_samples = [&] {
        if (m_speech_buf.size() > m_speech_max_size) {
            LOGD("speech buf reached max size");
            return true;
        }

        if (m_speech_buf.empty()) return false;

        if ((m_config.speech_mode == speech_mode_t::manual ||
             m_speech_detection_status ==
                 speech_detection_status_t::speech_detected) &&
            vad_status && !eof)
            return false;

        if ((m_config.speech_mode == speech_mode_t::manual ||
             m_config.speech_mode == speech_mode_t::single_sentence) &&
            m_speech_detection_status == speech_detection_status_t::no_speech &&
            !eof)
            return false;

        return true;
    }();

    if (!decode_samples) {
        if (eof || (m_config.speech_mode == speech_mode_t::manual &&
                    m_speech_detection_status ==
                        speech_detection_status_t::no_speech)) {
            flush(eof ? flush_t::eof : flush_t::regular);
            free_buf();
            return samples_process_result_t::no_samples_needed;
        }

        free_buf();
        return samples_process_result_t::wait_for_samples;
    }

    if (m_thread_exit_requested) {
        free_buf();
        return samples_process_result_t::no_samples_needed;
    }

    set_state(state_t::decoding);

    if (!vad_status) {
        set_speech_detection_status(speech_detection_status_t::no_speech);
    }

    LOGD("speech frame: samples=" << m_speech_buf.size());

    m_segment_time_offset += m_segment_time_discarded_before;
    m_segment_time_discarded_before = 0;

    decode_speech(m_speech_buf);

    m_segment_time_offset += (m_segment_time_discarded_after +
                              (1000 * m_speech_buf.size() / m_sample_rate));
    m_segment_time_discarded_after = 0;

    set_state(state_t::idle);

    if (m_config.speech_mode == speech_mode_t::single_sentence &&
        (!m_intermediate_text || m_intermediate_text->empty())) {
        LOGD("no speech decoded, forcing sentence timeout");
        m_call_backs.sentence_timeout();
    }

    m_speech_buf.clear();

    flush(eof || m_config.speech_mode == speech_mode_t::single_sentence
              ? flush_t::eof
              : flush_t::regular);

    free_buf();

    return samples_process_result_t::wait_for_samples;
}

void fasterwhisper_engine::decode_speech(const whisper_buf_t& buf) {
    LOGD("speech decoding started");

    create_model();

    auto decoding_start = std::chrono::steady_clock::now();

    auto task = py_executor::instance()->execute([&]() {
        try {
            py::array_t<float> array(buf.size());
            auto r = array.mutable_unchecked<1>();
            for (py::ssize_t i = 0; i < r.shape(0); ++i) r(i) = buf[i];

            auto seg_tuple = m_model->attr("transcribe")(
                "audio"_a = array, "beam_size"_a = 5,
                "language"_a = m_config.lang,
                "task"_a = m_config.translate ? "translate" : "transcribe");

            auto segments = *seg_tuple.cast<py::list>().begin();

            std::ostringstream os;

            bool subrip = m_config.text_format == text_format_t::subrip;

            auto i = 0;
            for (auto& segment : segments) {
                auto text = segment.attr("text").cast<std::string>();

                rtrim(text);
                ltrim(text);

                if (text.empty()) continue;
#ifdef DEBUG
                LOGD("segment: " << text);
#endif

                if (subrip) {
                    auto t0 = static_cast<size_t>(std::max(
                                  0.0, segment.attr("start").cast<double>())) *
                              1000;
                    auto t1 = static_cast<size_t>(std::max(
                                  0.0, segment.attr("end").cast<double>())) *
                              1000;

                    t0 += m_segment_time_offset;
                    t1 += m_segment_time_offset;

                    text_tools::segment_t segment{i + 1 + m_segment_offset, t0,
                                                  t1, text};
                    text_tools::break_segment_to_multiline(
                        m_config.sub_config.min_line_length,
                        m_config.sub_config.max_line_length, segment);

                    text_tools::segment_to_subrip_text(segment, os);
                } else {
                    if (i != 0) os << ' ';
                    os << std::move(text);
                }

                ++i;
            }

            m_segment_offset += i;

            return os.str();
        } catch (const std::exception& err) {
            LOGE("fasterwhisper py error: " << err.what());
            return std::string{""};
        }
    });

    if (!task) return;

    auto text = std::any_cast<std::string>(task->get());

    if (m_thread_exit_requested) return;

    auto decoding_dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - decoding_start)
                            .count();

    LOGD("speech decoded, stats: samples="
         << buf.size() << ", duration=" << decoding_dur << "ms ("
         << static_cast<double>(decoding_dur) /
                ((1000 * buf.size()) / static_cast<double>(m_sample_rate))
         << ")");

    auto result = merge_texts(m_intermediate_text.value_or(std::string{}),
                              std::move(text));

#ifdef DEBUG
    LOGD("speech decoded: text=" << result);
#endif

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result);
}

/* Copyright (C) 2024-2025 Cole Leavitt <cole@coleleavitt.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "canary_engine.hpp"

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

canary_engine::canary_engine(config_t config, callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    m_speech_buf.reserve(m_speech_max_size);
    m_auto_lang = m_config.lang == "auto";
}

canary_engine::~canary_engine() {
    LOGD("canary dtor");
    stop();
}

void canary_engine::stop() {
    stt_engine::stop();

    auto task = py_executor::instance()->execute([&]() {
        try {
            m_model.reset();
            py::module_::import("gc").attr("collect")();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();

    LOGD("canary stopped");
}

void canary_engine::push_buf_to_audio_buf(
    const std::vector<in_buf_t::buf_t::value_type>& buf,
    audio_buf_t& audio_buf) {
    std::transform(buf.cbegin(), buf.cend(), std::back_inserter(audio_buf),
                   [](auto sample) {
                       return static_cast<audio_buf_t::value_type>(sample) /
                              32768.0F;
                   });
}

void canary_engine::push_buf_to_audio_buf(in_buf_t::buf_t::value_type* data,
                                          in_buf_t::buf_t::size_type size,
                                          audio_buf_t& audio_buf) {
    audio_buf.reserve(audio_buf.size() + size);
    for (size_t i = 0; i < size; ++i) {
        audio_buf.push_back(static_cast<audio_buf_t::value_type>(data[i]) /
                            32768.0F);
    }
}

void canary_engine::reset_impl() { m_speech_buf.clear(); }

void canary_engine::stop_processing_impl() { LOGD("canary cancel"); }

void canary_engine::start_processing_impl() { create_model(); }

void canary_engine::create_model() {
    if (m_model) return;

    LOGD("creating canary model");

    auto task = py_executor::instance()->execute([&]() {
        auto n_threads = static_cast<int>(
            std::min(m_config.cpu_threads,
                     std::max(1U, std::thread::hardware_concurrency())));
        auto use_cuda =
            m_config.use_gpu && ((m_config.gpu_device.api == gpu_api_t::cuda &&
                                  gpu_tools::has_cudnn()) ||
                                 (m_config.gpu_device.api == gpu_api_t::rocm &&
                                  gpu_tools::has_hip()));

        LOGD("cpu info: arch=" << cpu_tools::arch()
                               << ", cores=" << std::thread::hardware_concurrency());
        LOGD("using threads: " << n_threads << "/"
                               << std::thread::hardware_concurrency());
        LOGD("using device: " << (use_cuda ? "cuda" : "cpu") << " "
                              << m_config.gpu_device.id);

        try {
            auto torch = py::module_::import("torch");
            auto nemo_asr = py::module_::import("nemo.collections.asr");
            auto os_path = py::module_::import("os.path");

            std::string device = use_cuda ? "cuda:" + std::to_string(m_config.gpu_device.id) : "cpu";

            std::string model_path = m_config.model_files.model_file;
            bool is_local_path = !model_path.empty() && 
                                 os_path.attr("exists")(model_path).cast<bool>();
            bool is_hf_model = !model_path.empty() && 
                               model_path.find('/') != std::string::npos &&
                               !is_local_path;

            std::string pretrained_name = "nvidia/canary-1b-v2";
            if (model_path.find("qwen") != std::string::npos ||
                model_path.find("2.5b") != std::string::npos ||
                model_path.find("2_5b") != std::string::npos) {
                pretrained_name = "nvidia/canary-qwen-2.5b";
            }

            LOGD("canary model_path: " << model_path);
            LOGD("canary is_local: " << is_local_path << ", is_hf: " << is_hf_model);
            LOGD("canary pretrained_name: " << pretrained_name);

            py::object model;
            if (is_local_path) {
                model = nemo_asr.attr("models").attr("EncDecMultiTaskModel")
                            .attr("restore_from")(model_path);
            } else if (is_hf_model) {
                model = nemo_asr.attr("models").attr("EncDecMultiTaskModel")
                            .attr("from_pretrained")(model_path);
            } else {
                model = nemo_asr.attr("models").attr("EncDecMultiTaskModel")
                            .attr("from_pretrained")(pretrained_name);
            }

            model.attr("to")(device);
            model.attr("eval")();

            if (use_cuda) {
                torch.attr("cuda").attr("empty_cache")();
            }

            m_model.emplace(std::move(model));
            return true;
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            m_model.reset();
            return false;
        }
    });

    if (!task || !std::any_cast<bool>(task->get())) {
        LOGE("failed to create canary model");
        throw std::runtime_error{"failed to create canary model"};
    }

    LOGD("canary model created");
}

stt_engine::samples_process_result_t canary_engine::process_buff() {
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
            push_buf_to_audio_buf(vad_buf, m_speech_buf);
        else
            push_buf_to_audio_buf(m_in_buf.buf.data(), m_in_buf.size,
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

void canary_engine::decode_speech(const audio_buf_t& buf) {
    LOGD("speech decoding started");

    create_model();

    auto decoding_start = std::chrono::steady_clock::now();

    auto task = py_executor::instance()->execute([&]() {
        try {
            py::array_t<float> array(buf.size());
            auto r = array.mutable_unchecked<1>();
            for (py::ssize_t i = 0; i < r.shape(0); ++i) r(i) = buf[i];

            auto torch = py::module_::import("torch");
            auto sf = py::module_::import("soundfile");
            auto tempfile = py::module_::import("tempfile");
            auto os = py::module_::import("os");

            auto tmp_dir = tempfile.attr("gettempdir")();
            auto tmp_path = py::str(tmp_dir) + py::str("/canary_temp.wav");

            sf.attr("write")(tmp_path, array, m_sample_rate);

            std::string source_lang = m_auto_lang ? "en" : m_config.lang;
            std::string target_lang = m_config.translate ? "en" : source_lang;
            std::string task_type = m_config.translate ? "s2t_translation" : "asr";

            py::list paths;
            paths.append(tmp_path);

            auto result = m_model->attr("transcribe")(
                paths,
                "batch_size"_a = 1,
                "source_lang"_a = source_lang,
                "target_lang"_a = target_lang,
                "task"_a = task_type,
                "pnc"_a = m_config.has_option('i')
            );

            os.attr("unlink")(tmp_path);

            std::string text;
            if (py::isinstance<py::list>(result) && py::len(result) > 0) {
                text = result[py::int_(0)].cast<std::string>();
            }

            rtrim(text);
            ltrim(text);

            std::string auto_lang = m_auto_lang ? "en" : m_config.lang;

            return std::pair<std::string, std::string>(std::move(text),
                                                       std::move(auto_lang));
        } catch (const std::exception& err) {
            LOGE("canary py error: " << err.what());
            return std::pair<std::string, std::string>({}, {});
        }
    });

    if (!task) return;

    auto [text, auto_lang] =
        std::any_cast<std::pair<std::string, std::string>>(task->get());

    if (m_thread_exit_requested) return;

    auto stats = report_stats(
        buf.size(), m_sample_rate,
        static_cast<size_t>(std::max(
            0L, static_cast<long int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - decoding_start)
                        .count()))));

    auto result = merge_texts(m_intermediate_text.value_or(std::string{}),
                              std::move(text));

    if (m_config.insert_stats) result.append(" " + stats);

#ifdef DEBUG
    LOGD("speech decoded: text=" << result);
#endif

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result, auto_lang);
}

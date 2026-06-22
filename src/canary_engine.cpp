/* Copyright (C) 2024-2025 Cole Leavitt <cole@coleleavitt.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "canary_engine.hpp"

#include <dlfcn.h>

#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <sstream>

#include "cpu_tools.hpp"
#include "gpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"
#include "text_tools.hpp"

using namespace pybind11::literals;

namespace {
struct wav_header_t {
    uint8_t riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunk_size = 0;
    uint8_t wave[4] = {'W', 'A', 'V', 'E'};
    uint8_t fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;
    uint16_t num_channels = 1;
    uint32_t sample_rate = 0;
    uint32_t bytes_per_sec = 0;
    uint16_t block_align = 2;
    uint16_t bits_per_sample = 16;
    uint8_t data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size = 0;
};

std::string canary_source_lang(const stt_engine::config_t& config) {
    auto lang = config.lang_code.empty() ? config.lang : config.lang_code;
    if (lang.empty() || lang == "auto" || lang == "multilang") return "en";
    return lang;
}

std::string canary_result_text(const py::handle& result) {
    if (py::isinstance<py::str>(result)) return result.cast<std::string>();

    if (py::hasattr(result, "text")) {
        auto text = result.attr("text");
        if (!text.is_none()) return text.cast<std::string>();
    }

    return py::str(result).cast<std::string>();
}

bool write_canary_wav(QTemporaryFile& file, const std::vector<float>& samples,
                      uint32_t sample_rate) {
    if (!file.open()) return false;

    wav_header_t header;
    header.sample_rate = sample_rate;
    header.data_size =
        static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    header.chunk_size = header.data_size + sizeof(wav_header_t) - 8;
    header.bytes_per_sec = sample_rate * sizeof(int16_t);

    if (file.write(reinterpret_cast<const char*>(&header), sizeof(header)) !=
        static_cast<qint64>(sizeof(header))) {
        return false;
    }

    for (auto sample : samples) {
        sample = std::clamp(sample, -1.0F, 1.0F);
        auto pcm = static_cast<int16_t>(sample * 32767.0F);
        if (file.write(reinterpret_cast<const char*>(&pcm), sizeof(pcm)) !=
            static_cast<qint64>(sizeof(pcm))) {
            return false;
        }
    }

    return file.flush();
}
}

canary_engine::canary_engine(config_t config, callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    m_speech_buf.reserve(m_speech_max_size);
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

            std::string device =
                use_cuda ? "cuda:" + std::to_string(m_config.gpu_device.id)
                         : "cpu";
            auto torch_device = torch.attr("device")(device);

            const std::string model_file = m_config.model_files.model_file;
            const bool is_local_model =
                !model_file.empty() &&
                QFileInfo{QString::fromStdString(model_file)}.exists();
            if (!is_local_model) {
                LOGE("canary model file not found: " << model_file);
                return false;
            }

            auto asr_model = nemo_asr.attr("models").attr("ASRModel");
            LOGD("restoring canary model from: " << model_file);
            auto model = asr_model.attr("restore_from")(
                "restore_path"_a = model_file, "map_location"_a = torch_device);

            model.attr("to")(torch_device);
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
    QTemporaryFile tmp_file{QDir::temp().absoluteFilePath(
        QStringLiteral("dsnote-canary-XXXXXX.wav"))};
    if (!write_canary_wav(tmp_file, buf, m_sample_rate)) {
        LOGE("failed to write canary temp wav: "
             << tmp_file.fileName().toStdString());
        return;
    }
    tmp_file.close();
    const auto tmp_path = tmp_file.fileName().toStdString();

    auto task = py_executor::instance()->execute([&, tmp_path]() {
        try {
            std::string source_lang = canary_source_lang(m_config);
            std::string target_lang = m_config.translate ? "en" : source_lang;
            std::string task_type = m_config.translate ? "s2t_translation" : "asr";
            auto pnc = m_config.has_option('i') ? "yes" : "no";

            py::list paths;
            paths.append(tmp_path);

            auto result = m_model->attr("transcribe")(
                paths,
                "batch_size"_a = 1,
                "source_lang"_a = source_lang,
                "target_lang"_a = target_lang,
                "task"_a = task_type,
                "pnc"_a = pnc,
                "verbose"_a = false);

            std::string text;
            if (py::isinstance<py::list>(result) && py::len(result) > 0) {
                text = canary_result_text(result[py::int_(0)]);
            }

            rtrim(text);
            ltrim(text);

            return std::pair<std::string, std::string>(std::move(text),
                                                       std::move(source_lang));
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

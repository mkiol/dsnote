/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "vosk_engine.hpp"

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>

#include "logger.hpp"

using namespace std::chrono_literals;

static size_t model_max_size() {
#ifdef USE_SFOS
    return 1000000000;
#else
    static auto size = (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE));
    return size;
#endif
}

size_t du(const std::string& path) {
    struct stat stats {};

    if (lstat(path.c_str(), &stats) == 0) {
        if (S_ISREG(stats.st_mode)) {
            return stats.st_size;
        }
    } else {
        LOGE("lstat error");
        return 0;
    }

    auto* dir = opendir(path.c_str());
    if (dir == nullptr) {
        LOGE("opendir error");
        return 0;
    }

    struct dirent* entry;
    size_t total_size = 0;

    for (entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
        auto new_path = path + "/" + entry->d_name;

        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") != 0 &&
                strcmp(entry->d_name, "..") != 0) {
                total_size += du(new_path);
            }
        } else if (lstat(new_path.c_str(), &stats) == 0) {
            total_size += stats.st_size;
        }
    }

    closedir(dir);

    return total_size;
}

vosk_engine::vosk_engine(config_t config, callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    open_vosk_lib();
    m_speech_buf.reserve(m_speech_max_size);
}

vosk_engine::~vosk_engine() {
    LOGD("vosk dtor");

    stop();

    if (m_vosk_api.ok()) {
        if (m_vosk_recognizer)
            m_vosk_api.vosk_recognizer_free(m_vosk_recognizer);
        m_vosk_recognizer = nullptr;
        if (m_vosk_model) m_vosk_api.vosk_model_free(m_vosk_model);
        m_vosk_model = nullptr;
    }

    m_vosk_api = {};

    if (m_vosklib_handle) {
        dlclose(m_vosklib_handle);
        m_vosklib_handle = nullptr;
    }
}

void vosk_engine::open_vosk_lib() {
    m_vosklib_handle = dlopen("libvosk.so", RTLD_LAZY);
    if (m_vosklib_handle == nullptr) {
        LOGE("failed to open vosk lib: " << dlerror());
        throw std::runtime_error("failed to open vosk lib");
    }

    m_vosk_api.vosk_model_new =
        reinterpret_cast<decltype(m_vosk_api.vosk_model_new)>(
            dlsym(m_vosklib_handle, "vosk_model_new"));
    m_vosk_api.vosk_model_free =
        reinterpret_cast<decltype(m_vosk_api.vosk_model_free)>(
            dlsym(m_vosklib_handle, "vosk_model_free"));
    m_vosk_api.vosk_recognizer_new =
        reinterpret_cast<decltype(m_vosk_api.vosk_recognizer_new)>(
            dlsym(m_vosklib_handle, "vosk_recognizer_new"));
    m_vosk_api.vosk_recognizer_reset =
        reinterpret_cast<decltype(m_vosk_api.vosk_recognizer_reset)>(
            dlsym(m_vosklib_handle, "vosk_recognizer_reset"));
    m_vosk_api.vosk_recognizer_free =
        reinterpret_cast<decltype(m_vosk_api.vosk_recognizer_free)>(
            dlsym(m_vosklib_handle, "vosk_recognizer_free"));
    m_vosk_api.vosk_recognizer_accept_waveform_s = reinterpret_cast<decltype(
        m_vosk_api.vosk_recognizer_accept_waveform_s)>(
        dlsym(m_vosklib_handle, "vosk_recognizer_accept_waveform_s"));
    m_vosk_api.vosk_recognizer_partial_result =
        reinterpret_cast<decltype(m_vosk_api.vosk_recognizer_partial_result)>(
            dlsym(m_vosklib_handle, "vosk_recognizer_partial_result"));
    m_vosk_api.vosk_recognizer_final_result =
        reinterpret_cast<decltype(m_vosk_api.vosk_recognizer_final_result)>(
            dlsym(m_vosklib_handle, "vosk_recognizer_final_result"));

    if (!m_vosk_api.ok()) {
        LOGE("failed to register vosk api");
        throw std::runtime_error("failed to register vosk api");
    }
}

void vosk_engine::start_processing_impl() {
    create_vosk_model();
    create_punctuator();
}

void vosk_engine::create_punctuator() {
    if (m_punctuator || m_config.model_files.ttt_model_file.empty()) return;

    LOGD("creating punctuator");
    try {
        m_punctuator.emplace(m_config.model_files.ttt_model_file);

        LOGD("punctuator created");
    } catch (const std::runtime_error& error) {
        LOGE("failed to create punctuator");
    }
}

void vosk_engine::create_vosk_model() {
    if (m_vosk_model) return;

    LOGD("creating vosk model");

    auto size = du(m_config.model_files.model_file);
    LOGD("model size: " << size << " (max: " << model_max_size() << ")");

    if (size > model_max_size()) {
        LOGE("model is too large");
        throw std::runtime_error(
            "failed to create vosk model because it is too large");
    }

    m_vosk_model =
        m_vosk_api.vosk_model_new(m_config.model_files.model_file.c_str());
    if (m_vosk_model == nullptr) {
        LOGE("failed to create vosk model");
        throw std::runtime_error("failed to create vosk model");
    }

    m_vosk_recognizer =
        m_vosk_api.vosk_recognizer_new(m_vosk_model, m_sample_rate);
    if (m_vosk_recognizer == nullptr) {
        LOGE("failed to create vosk recognizer");
        throw std::runtime_error("failed to create vosk recognizer");
    }

    LOGD("vosk model created");
}

void vosk_engine::reset_impl() {
    m_speech_buf.clear();

#ifdef DUMP_AUDIO_TO_FILE
    m_file_audio_input.reset();
    m_file_audio_after_denoise.reset();
    m_file_audio_after_vad.reset();
#endif

    if (m_vosk_recognizer) m_vosk_api.vosk_recognizer_reset(m_vosk_recognizer);
}

void vosk_engine::push_inbuf_to_samples() {
    auto end = m_in_buf.buf.cbegin();
    std::advance(end, m_in_buf.size);
    m_speech_buf.insert(m_speech_buf.end(), m_in_buf.buf.cbegin(), end);
}

stt_engine::samples_process_result_t vosk_engine::process_buff() {
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

        if (m_vosk_recognizer)
            m_vosk_api.vosk_recognizer_reset(m_vosk_recognizer);
    }

#ifdef DUMP_AUDIO_TO_FILE
    if (!m_file_audio_input)
        m_file_audio_input = std::make_unique<std::ofstream>("audio_input.pcm");
    m_file_audio_input->write(
        reinterpret_cast<char*>(m_in_buf.buf.data()),
        m_in_buf.size * sizeof(decltype(m_in_buf.buf)::value_type));
#endif

    m_denoiser.process(m_in_buf.buf.data(), m_in_buf.size);

#ifdef DUMP_AUDIO_TO_FILE
    if (!m_file_audio_after_denoise)
        m_file_audio_after_denoise =
            std::make_unique<std::ofstream>("audio_after_denoise.pcm");
    m_file_audio_after_denoise->write(
        reinterpret_cast<char*>(m_in_buf.buf.data()),
        m_in_buf.size * sizeof(decltype(m_in_buf.buf)::value_type));
#endif

    const auto& vad_buf =
        m_vad.remove_silence(m_in_buf.buf.data(), m_in_buf.size);

#ifdef DUMP_AUDIO_TO_FILE
    if (!m_file_audio_after_vad)
        m_file_audio_after_vad =
            std::make_unique<std::ofstream>("audio_after_vad.pcm");
    m_file_audio_after_vad->write(
        reinterpret_cast<const char*>(vad_buf.data()),
        vad_buf.size() * sizeof(decltype(m_in_buf.buf)::value_type));
#endif

    m_in_buf.clear();

    bool vad_status = !vad_buf.empty();

    m_in_buf.clear();

    if (vad_status) {
        LOGD("vad: speech detected");

        if (m_config.speech_mode != speech_mode_t::manual &&
            m_config.speech_mode != speech_mode_t::single_sentence)
            set_speech_detection_status(
                speech_detection_status_t::speech_detected);

        m_speech_buf.insert(m_speech_buf.end(), vad_buf.cbegin(),
                            vad_buf.cend());

        restart_sentence_timer();
    } else {
        LOGD("vad: no speech");

        if (m_config.speech_mode == speech_mode_t::single_sentence &&
            (!m_intermediate_text || m_intermediate_text->empty()) &&
            sentence_timer_timed_out()) {
            LOGD("sentence timeout");
            m_call_backs.sentence_timeout();
        }
    }

    if (m_thread_exit_requested) {
        free_buf();
        return samples_process_result_t::no_samples_needed;
    }

    auto final_decode = [&] {
        if (eof) return true;
        if (m_config.speech_mode != speech_mode_t::manual &&
            m_intermediate_text && !m_intermediate_text->empty() && !vad_status)
            return true;
        return false;
    }();

    if (final_decode || !m_speech_buf.empty()) {
        set_processing_state(processing_state_t::decoding);

        LOGD("speech frame: samples=" << m_speech_buf.size()
                                      << ", final=" << final_decode);

        decode_speech(m_speech_buf, final_decode);

        set_processing_state(processing_state_t::idle);

        m_speech_buf.clear();

        if (final_decode)
            flush(!eof && m_config.speech_mode == speech_mode_t::automatic
                      ? flush_t::regular
                      : flush_t::eof);
    }

    if (!vad_status && !final_decode &&
        m_config.speech_mode == speech_mode_t::automatic)
        set_speech_detection_status(speech_detection_status_t::no_speech);

    free_buf();

    return samples_process_result_t::wait_for_samples;
}

std::string vosk_engine::get_from_json(const char* name, const char* str) {
    auto json = simdjson::padded_string{str, strlen(str)};

    auto doc = m_parser.iterate(json);
    if (doc.error() != simdjson::SUCCESS) {
        LOGE("failed to simdjson iterate");
        return {};
    }

    std::string_view sv;

    if (!doc[name].get(sv)) return std::string{sv};

    return {};
}

void vosk_engine::decode_speech(const vosk_buf_t& buf, bool eof) {
    LOGD("speech decoding started");

    auto ret = m_vosk_api.vosk_recognizer_accept_waveform_s(
        m_vosk_recognizer, buf.data(), buf.size());

    if (ret < 0) {
        LOGE("error in vosk_recognizer_accept_waveform_s");
        return;
    }

    if (ret == 0 && !eof) {
        // append silence to force partial result

        std::array<vosk_buf_t::value_type, m_in_buf_max_size> silence{};

        auto ret = m_vosk_api.vosk_recognizer_accept_waveform_s(
            m_vosk_recognizer, silence.data(), silence.size());

        if (ret == 0) {
            LOGD("no speech decoded");
            return;
        }
    }

    auto result = [&] {
        if (eof)
            return get_from_json(
                "text",
                m_vosk_api.vosk_recognizer_final_result(m_vosk_recognizer));

        return get_from_json(
            "partial",
            m_vosk_api.vosk_recognizer_partial_result(m_vosk_recognizer));
    }();

#ifdef DEBUG
    LOGD("speech decoded: text=" << result);
#else
    LOGD("speech decoded");
#endif

    if (m_punctuator) result = m_punctuator->process(result);

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result);

    if (eof) m_vosk_api.vosk_recognizer_reset(m_vosk_recognizer);
}

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "whisper_engine.hpp"

#include <dlfcn.h>

#include <algorithm>
#include <chrono>
#include <sstream>

#include "cpu_tools.hpp"
#include "logger.hpp"

whisper_engine::whisper_engine(config_t config, callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    open_whisper_lib();
    m_wparams = make_wparams();
    m_speech_buf.reserve(m_speech_max_size);
}

whisper_engine::~whisper_engine() {
    LOGD("whisper dtor");

    stop();

    if (m_whisper_api.ok()) {
        if (m_whisper_ctx) {
            m_whisper_api.whisper_free(m_whisper_ctx);
            m_whisper_ctx = nullptr;
        }
    }

    m_whisper_api = {};

    if (m_whisperlib_handle) {
        dlclose(m_whisperlib_handle);
        m_whisperlib_handle = nullptr;
    }
}

void whisper_engine::open_whisper_lib() {
#ifdef ARCH_ARM_32
    if (cpu_tools::neon_supported()) {
        m_whisperlib_handle = dlopen("libwhisper.so", RTLD_LAZY);
    } else {
        LOGW("using whisper-fallback");
        m_whisperlib_handle = dlopen("libwhisper-fallback.so", RTLD_LAZY);
    }
#elif ARCH_ARM_64
    m_whisperlib_handle = dlopen("libwhisper.so", RTLD_LAZY);
#else
    if (cpu_tools::avx_avx2_fma_f16c_supported()) {
        m_whisperlib_handle = dlopen("libwhisper.so", RTLD_LAZY);
    } else {
        LOGW("using whisper-fallback");
        m_whisperlib_handle = dlopen("libwhisper-fallback.so", RTLD_LAZY);
    }
#endif

    if (m_whisperlib_handle == nullptr) {
        LOGE("failed to open whisper lib: " << dlerror());
        throw std::runtime_error("failed to open whisper lib");
    }

    m_whisper_api.whisper_init_from_file =
        reinterpret_cast<decltype(m_whisper_api.whisper_init_from_file)>(
            dlsym(m_whisperlib_handle, "whisper_init_from_file"));
    m_whisper_api.whisper_cancel =
        reinterpret_cast<decltype(m_whisper_api.whisper_cancel)>(
            dlsym(m_whisperlib_handle, "whisper_cancel"));
    m_whisper_api.whisper_cancel_clear =
        reinterpret_cast<decltype(m_whisper_api.whisper_cancel_clear)>(
            dlsym(m_whisperlib_handle, "whisper_cancel_clear"));
    m_whisper_api.whisper_print_system_info =
        reinterpret_cast<decltype(m_whisper_api.whisper_print_system_info)>(
            dlsym(m_whisperlib_handle, "whisper_print_system_info"));
    m_whisper_api.whisper_full =
        reinterpret_cast<decltype(m_whisper_api.whisper_full)>(
            dlsym(m_whisperlib_handle, "whisper_full"));
    m_whisper_api.whisper_full_n_segments =
        reinterpret_cast<decltype(m_whisper_api.whisper_full_n_segments)>(
            dlsym(m_whisperlib_handle, "whisper_full_n_segments"));
    m_whisper_api.whisper_full_get_segment_text =
        reinterpret_cast<decltype(m_whisper_api.whisper_full_get_segment_text)>(
            dlsym(m_whisperlib_handle, "whisper_full_get_segment_text"));
    m_whisper_api.whisper_free =
        reinterpret_cast<decltype(m_whisper_api.whisper_free)>(
            dlsym(m_whisperlib_handle, "whisper_free"));
    m_whisper_api.whisper_full_default_params =
        reinterpret_cast<decltype(m_whisper_api.whisper_full_default_params)>(
            dlsym(m_whisperlib_handle, "whisper_full_default_params"));

    if (!m_whisper_api.ok()) {
        LOGE("failed to register whisper api");
        throw std::runtime_error("failed to register whisper api");
    }
}

void whisper_engine::push_buf_to_whisper_buf(
    const std::vector<in_buf_t::buf_t::value_type>& buf,
    whisper_buf_t& whisper_buf) {
    // convert s16 to f32 sample format
    std::transform(buf.cbegin(), buf.cend(), std::back_inserter(whisper_buf),
                   [](auto sample) {
                       return static_cast<whisper_buf_t::value_type>(sample) /
                              32768.0F;
                   });
}

void whisper_engine::reset_impl() { m_speech_buf.clear(); }

void whisper_engine::stop_processing_impl() {
    if (m_whisper_ctx) {
        LOGD("whisper cancel");
        m_whisper_api.whisper_cancel(m_whisper_ctx);
    }
}

void whisper_engine::start_processing_impl() { create_whisper_model(); }

void whisper_engine::create_whisper_model() {
    if (m_whisper_ctx) return;

    LOGD("creating whisper model");

    m_whisper_ctx =
        m_whisper_api.whisper_init_from_file(m_model_files.model_file.c_str());

    if (m_whisper_ctx == nullptr) {
        LOGE("failed to create whisper ctx");
        throw std::runtime_error("failed to create whisper ctx");
    }

    LOGD("whisper model created");
}

stt_engine::samples_process_result_t whisper_engine::process_buff() {
    if (!lock_buff_for_processing())
        return samples_process_result_t::wait_for_samples;

    auto eof = m_in_buf.eof;
    auto sof = m_in_buf.sof;

    LOGD("process samples buf: mode="
         << m_speech_mode << ", in-buf size=" << m_in_buf.size
         << ", speech-buf size=" << m_speech_buf.size() << ", sof=" << sof
         << ", eof=" << eof);

    if (sof) {
        m_speech_buf.clear();
        m_start_time.reset();
        m_vad.reset();
    }

    m_denoiser.process(m_in_buf.buf.data(), m_in_buf.size);

    const auto& vad_buf =
        m_vad.remove_silence(m_in_buf.buf.data(), m_in_buf.size);

    m_in_buf.clear();

    bool vad_status = !vad_buf.empty();

    if (vad_status) {
        LOGD("vad: speech detected");

        if (m_speech_mode != speech_mode_t::manual)
            set_speech_detection_status(
                speech_detection_status_t::speech_detected);

        push_buf_to_whisper_buf(vad_buf, m_speech_buf);

        restart_sentence_timer();
    } else {
        LOGD("vad: no speech");

        if (m_speech_mode == speech_mode_t::single_sentence &&
            m_speech_buf.empty() && sentence_timer_timed_out()) {
            LOGD("sentence timeout");
            m_call_backs.sentence_timeout();
        }

        if (m_speech_mode == speech_mode_t::automatic)
            set_speech_detection_status(speech_detection_status_t::no_speech);
    }

    auto decode_samples = [&] {
        if (m_speech_buf.size() > m_speech_max_size) {
            LOGD("speech buf reached max size");
            return true;
        }

        if (m_speech_buf.empty()) return false;

        if ((m_speech_mode != speech_mode_t::manual ||
             m_speech_detection_status ==
                 speech_detection_status_t::speech_detected) &&
            vad_status && !eof)
            return false;

        if (m_speech_mode == speech_mode_t::manual &&
            m_speech_detection_status == speech_detection_status_t::no_speech &&
            !eof)
            return false;

        return true;
    }();

    if (!decode_samples) {
        if (m_speech_mode == speech_mode_t::manual &&
            m_speech_detection_status == speech_detection_status_t::no_speech) {
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

    set_processing_state(processing_state_t::decoding);

    if (!vad_status) {
        set_speech_detection_status(speech_detection_status_t::no_speech);
    }

    LOGD("speech frame: samples=" << m_speech_buf.size());

    decode_speech(m_speech_buf);

    set_processing_state(processing_state_t::idle);

    m_speech_buf.clear();

    flush(eof ? flush_t::eof : flush_t::regular);

    free_buf();

    return samples_process_result_t::wait_for_samples;
}

whisper_full_params whisper_engine::make_wparams() {
    whisper_full_params wparams =
        m_whisper_api.whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    if (auto pos = m_lang.find('-'); pos != std::string::npos) {
        m_lang = m_lang.substr(0, pos);
    }

    wparams.language = m_lang.c_str();
    wparams.speed_up = true;
    wparams.suppress_blank = true;
    wparams.suppress_non_speech_tokens = true;
    wparams.single_segment = false;
    wparams.translate = m_translate;
    wparams.no_context = true;
    wparams.n_threads = std::min(
        m_threads,
        std::max(1, static_cast<int>(std::thread::hardware_concurrency())));

    LOGD("cpu info: arch=" << cpu_tools::arch() << ", cores="
                           << std::thread::hardware_concurrency());
    LOGD("using threads: " << wparams.n_threads << "/"
                           << std::thread::hardware_concurrency());
    LOGD("system info: " << m_whisper_api.whisper_print_system_info());

    return wparams;
}

void whisper_engine::decode_speech(const whisper_buf_t& buf) {
    LOGD("speech decoding started");

    create_whisper_model();

    auto decoding_start = std::chrono::steady_clock::now();

    std::ostringstream os;

    if (!m_thread_exit_requested)
        m_whisper_api.whisper_cancel_clear(m_whisper_ctx);

    if (auto ret = m_whisper_api.whisper_full(m_whisper_ctx, m_wparams,
                                              buf.data(), buf.size());
        ret == 0) {
        auto n = m_whisper_api.whisper_full_n_segments(m_whisper_ctx);
        LOGD("decoded segments: " << n);

        for (auto i = 0; i < n; ++i) {
            std::string text =
                m_whisper_api.whisper_full_get_segment_text(m_whisper_ctx, i);
            rtrim(text);
            ltrim(text);
#ifdef DEBUG
            LOGD("segment " << i << ": " << text);
#endif
            if (i != 0) os << ' ';

            os << text;
        }
    } else {
        LOGE("whisper error: " << ret);
        return;
    }

    auto decoding_dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - decoding_start)
                            .count();

    LOGD("speech decoded, stats: samples="
         << buf.size() << ", duration=" << decoding_dur << "ms ("
         << static_cast<double>(decoding_dur) /
                ((1000 * buf.size()) / static_cast<double>(m_sample_rate))
         << ")");

    auto result =
        merge_texts(m_intermediate_text.value_or(std::string{}), os.str());

#ifdef DEBUG
    LOGD("speech decoded: text=" << result);
#endif

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result);
}

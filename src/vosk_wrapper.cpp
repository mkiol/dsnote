/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "vosk_wrapper.hpp"

#include <dlfcn.h>

#include <algorithm>
#include <chrono>
#include <fstream>

#include "RSJparser.hpp"
#include "logger.hpp"

using namespace std::chrono_literals;

vosk_wrapper::vosk_wrapper(config_t config, callbacks_t call_backs)
    : engine_wrapper{std::move(config), std::move(call_backs)} {
    open_vosk_lib();
    m_speech_buf.reserve(m_speech_max_size);
    start_engine();
}

vosk_wrapper::~vosk_wrapper() {
    LOGD("vosk dtor");

    stop_processing();

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

void vosk_wrapper::open_vosk_lib() {
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
    m_vosk_api.vosk_recognizer_accept_waveform_s = reinterpret_cast<
        decltype(m_vosk_api.vosk_recognizer_accept_waveform_s)>(
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

void vosk_wrapper::create_vosk_model() {
    if (m_vosk_model) return;

    LOGD("creating vosk model");

    m_vosk_model = m_vosk_api.vosk_model_new(m_model_file.first.c_str());
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

void vosk_wrapper::reset_impl() {
    m_speech_buf.clear();

    if (m_vosk_recognizer) m_vosk_api.vosk_recognizer_reset(m_vosk_recognizer);
}

engine_wrapper::samples_process_result_t vosk_wrapper::process_buff() {
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

        if (m_vosk_recognizer)
            m_vosk_api.vosk_recognizer_reset(m_vosk_recognizer);
    }

    bool vad_status = m_vad.is_speech(m_in_buf.buf.data(), m_in_buf.buf.size());

    m_speech_buf.insert(m_speech_buf.end(), m_in_buf.buf.cbegin(),
                        m_in_buf.buf.cend());

    m_in_buf.clear();

    if (vad_status) {
        LOGD("vad: speech detected");

        if (m_speech_mode != speech_mode_t::manual)
            set_speech_detection_status(
                speech_detection_status_t::speech_detected);

        restart_sentence_timer();
    } else {
        LOGD("vad: no speech");

        if (m_speech_mode == speech_mode_t::single_sentence &&
            (!m_intermediate_text || m_intermediate_text->empty()) &&
            sentence_timer_timed_out()) {
            LOGD("sentence timeout");
            m_call_backs.sentence_timeout();
        }

        //        if (m_speech_mode == speech_mode_t::automatic)
        //            set_speech_detection_status(speech_detection_status_t::no_speech);

        //        if (m_speech_mode == speech_mode_t::single_sentence &&
        //            m_intermediate_text && !m_intermediate_text->empty()) {
        //            set_speech_detection_status(speech_detection_status_t::no_speech);
        //        }
    }

    if (m_thread_exit_requested) {
        free_buf();
        return samples_process_result_t::no_samples_needed;
    }

    auto final_decode = [&] {
        if (eof) return true;
        if (m_speech_mode == speech_mode_t::single_sentence &&
            m_intermediate_text && !m_intermediate_text->empty() && !vad_status)
            return true;
        if (m_speech_mode == speech_mode_t::automatic && !vad_status)
            return true;
        return false;
    }();

    auto old_status = m_speech_detection_status;

    if (final_decode && m_speech_mode != speech_mode_t::automatic)
        set_speech_detection_status(speech_detection_status_t::decoding);

    LOGD("speech frame: samples=" << m_speech_buf.size()
                                  << ", final=" << final_decode);

    decode_speech(m_speech_buf, final_decode);

    m_speech_buf.clear();

    if (final_decode ||
        (m_speech_mode == speech_mode_t::manual && !m_speech_started))
        set_speech_detection_status(speech_detection_status_t::no_speech);
    else
        set_speech_detection_status(old_status);

    if (final_decode)
        flush(!eof && m_speech_mode == speech_mode_t::automatic
                  ? flush_t::regular
                  : flush_t::eof);

    free_buf();

    return samples_process_result_t::wait_for_samples;

    /*auto old_status = m_speech_detection_status;

    if (eof ||
        (m_speech_detection_status == speech_detection_status_t::no_speech &&
         m_speech_mode != speech_mode_t::automatic))
        set_speech_detection_status(speech_detection_status_t::decoding);

    LOGD("speech frame: samples=" << m_speech_buf.size());

    auto final_decode = eof || (m_speech_mode != speech_mode_t::manual &&
                                m_speech_detection_status ==
                                    speech_detection_status_t::no_speech);

    decode_speech(m_speech_buf, final_decode);

    m_speech_buf.clear();

    if (m_speech_mode == speech_mode_t::manual && !m_speech_started)
        set_speech_detection_status(speech_detection_status_t::no_speech);
    else
        set_speech_detection_status(old_status);

    if (eof ||
        m_speech_detection_status == speech_detection_status_t::no_speech) {
        flush(eof ? flush_t::eof : flush_t::regular);
    }

    free_buf();

    if (m_speech_mode == speech_mode_t::manual)
        return samples_process_result_t::no_samples_needed;
    return samples_process_result_t::wait_for_samples;*/
}

void vosk_wrapper::decode_speech(const vosk_buf_t& buf, bool eof) {
    LOGD("speech decoding started");

    create_vosk_model();

    auto ret = m_vosk_api.vosk_recognizer_accept_waveform_s(
        m_vosk_recognizer, buf.data(), buf.size());

    if (ret < 0) {
        LOGE("error in vosk_recognizer_accept_waveform_s");
        return;
    }

    if (ret == 0 && !eof) {
        LOGD("no speech decoded");
        return;
    }

    auto result = [&] {
        if (eof) {
            const auto* str =
                m_vosk_api.vosk_recognizer_final_result(m_vosk_recognizer);
            return RSJresource{str}["text"].as<std::string>();
        }

        const auto* str =
            m_vosk_api.vosk_recognizer_partial_result(m_vosk_recognizer);
        return RSJresource{str}["partial"].as<std::string>();
    }();

    LOGD("speech decoded");

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result);

    if (eof) m_vosk_api.vosk_recognizer_reset(m_vosk_recognizer);
}

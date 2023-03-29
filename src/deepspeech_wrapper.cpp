/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "deepspeech_wrapper.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>

#include "logger.hpp"

using namespace std::chrono_literals;

deepspeech_wrapper::deepspeech_wrapper(config_t config, callbacks_t call_backs)
    : engine_wrapper{std::move(config), std::move(call_backs)} {
    create_ds_model();
    m_speech_buf.reserve(m_speech_max_size);
    start_engine();
}

deepspeech_wrapper::~deepspeech_wrapper() {
    LOGD("ds dtor");

    stop_processing();
}

std::string deepspeech_wrapper::ds_error_msg(int status) {
    auto error_mgs_raw = STT_ErrorCodeToErrorMessage(status);
    std::string error_mgs{error_mgs_raw};
    delete error_mgs_raw;
    return error_mgs;
}

void deepspeech_wrapper::create_ds_model() {
    ModelState* state;

    auto status = STT_CreateModel(m_model_file.first.c_str(), &state);

    if (status == 0) {
        m_ds_model =
            ds_model_t{state, [](ModelState* state) { STT_FreeModel(state); }};
        if (!m_model_file.second.empty()) {
            STT_EnableExternalScorer(m_ds_model.get(),
                                     m_model_file.second.c_str());
        }
    } else {
        LOGE("failed to create model: " << ds_error_msg(status));
        throw std::runtime_error("failed to create model");
    }
}

void deepspeech_wrapper::create_ds_stream() {
    auto status = STT_CreateStream(m_ds_model.get(), &m_ds_stream);

    if (status != 0) {
        LOGD("failed to create stream:" << ds_error_msg(status));
        free_ds_stream();
        throw std::runtime_error("failed to create ds stream");
    }
}

void deepspeech_wrapper::free_ds_stream() {
    if (m_ds_stream) {
        STT_FreeStream(m_ds_stream);
        m_ds_stream = nullptr;
    }
}

void deepspeech_wrapper::reset_impl() { m_speech_buf.clear(); }

engine_wrapper::samples_process_result_t deepspeech_wrapper::process_buff() {
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

    const auto& vad_buf =
        m_vad.process(m_in_buf.buf.data(), m_in_buf.buf.size());

    m_in_buf.clear();

    bool vad_status = !vad_buf.empty();

    m_in_buf.clear();

    if (vad_status) {
        LOGD("vad: speech detected");

        if (m_speech_mode != speech_mode_t::manual)
            set_speech_detection_status(
                speech_detection_status_t::speech_detected);

        m_speech_buf.insert(m_speech_buf.end(), vad_buf.cbegin(),
                            vad_buf.cend());
    } else {
        LOGD("vad: no speech");

        if (m_speech_mode == speech_mode_t::single_sentence &&
            sentence_timer_timed_out()) {
            m_call_backs.sentence_timeout();
        }

        if (m_speech_mode == speech_mode_t::automatic)
            set_speech_detection_status(speech_detection_status_t::no_speech);

        if (m_speech_mode == speech_mode_t::single_sentence &&
            !m_intermediate_text->empty())
            set_speech_detection_status(speech_detection_status_t::no_speech);
    }

    auto do_decode_speech = [&] {
        if (m_speech_buf.size() > m_speech_max_size) {
            LOGD("speech buf reached max size");
            return true;
        }

        if (m_speech_buf.empty()) return false;

        if (m_speech_mode == speech_mode_t::manual &&
            m_speech_detection_status == speech_detection_status_t::no_speech &&
            !eof)
            return false;

        return true;
    }();

    if (!do_decode_speech) {
        if (m_speech_detection_status == speech_detection_status_t::no_speech) {
            free_ds_stream();
            flush(eof ? flush_t::eof : flush_t::regular);
            free_buf();

            if (m_speech_mode == speech_mode_t::manual)
                return samples_process_result_t::no_samples_needed;
            return samples_process_result_t::wait_for_samples;
        }

        free_buf();
        return samples_process_result_t::wait_for_samples;
    }

    if (m_thread_exit_requested) {
        free_buf();
        return samples_process_result_t::no_samples_needed;
    }

    auto old_status = m_speech_detection_status;

    if (m_speech_detection_status == speech_detection_status_t::no_speech)
        set_speech_detection_status(speech_detection_status_t::decoding);

    LOGD("speech frame: samples=" << m_speech_buf.size() << ", duration="
                                  << (m_speech_buf.size() / m_sample_rate));

    decode_speech(m_speech_buf);

    m_speech_buf.clear();

    if (m_speech_mode == speech_mode_t::manual && !m_speech_started)
        set_speech_detection_status(speech_detection_status_t::no_speech);
    else
        set_speech_detection_status(old_status);

    if (eof) flush(flush_t::eof);

    free_buf();

    return samples_process_result_t::wait_for_samples;
}

bool deepspeech_wrapper::sentence_timer_timed_out() {
    if (m_start_time) {
        if (m_speech_buf.empty() && m_intermediate_text->empty() &&
            m_timeout > std::chrono::steady_clock::now() - *m_start_time) {
            LOGD("sentence timeout");
            return true;
        }
    } else {
        LOGD("staring sentence timer");
        m_start_time = std::chrono::steady_clock::now();
    }

    return false;
}

void deepspeech_wrapper::decode_speech(const ds_buf_t& buf) {
    if (!m_ds_stream) create_ds_stream();

    STT_FeedAudioContent(m_ds_stream, buf.data(), buf.size());

    auto* cstr = STT_IntermediateDecode(m_ds_stream);

    std::string result{cstr};
    STT_FreeString(cstr);

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result);
}

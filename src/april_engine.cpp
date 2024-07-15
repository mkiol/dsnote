/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "april_engine.hpp"

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
#include "text_tools.hpp"

using namespace std::chrono_literals;

april_engine::april_engine(config_t config, callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    m_speech_buf.reserve(m_speech_max_size);
    aam_api_init(APRIL_VERSION);
}

april_engine::~april_engine() {
    LOGD("vosk dtor");

    stop();

    if (m_session) {
        aas_free(m_session);
        m_session = nullptr;
    }

    if (m_model) {
        aam_free(m_model);
        m_model = nullptr;
    }
}

void april_engine::start_processing_impl() {
    create_model();

    if (m_config.has_option('i')) create_punctuator();
}

void april_engine::restart_session(bool force) {
    if (!force && !m_session) return;

    if (m_session) aas_free(m_session);

    AprilConfig config{};
    config.handler = &april_engine::decode_handler;
    config.userdata = this;
    config.flags = APRIL_CONFIG_FLAG_ZERO_BIT;

    m_session = aas_create_session(m_model, config);
    if (m_session == nullptr) {
        LOGE("failed to create april session");
        throw std::runtime_error("failed to create april session");
    }
}

void april_engine::create_model() {
    if (m_model) return;

    LOGD("creating april model");

    m_model = aam_create_model(m_config.model_files.model_file.c_str());
    if (m_model == nullptr) {
        LOGE("failed to create april model");
        throw std::runtime_error("failed to create april model");
    }

    LOGD("model: name=" << aam_get_name(m_model)
                        << ", lang=" << aam_get_language(m_model)
                        << ", sample rate=" << aam_get_sample_rate(m_model));

    restart_session(true);

    LOGD("april model created");
}

void april_engine::reset_impl() {
    m_speech_buf.clear();
    restart_session(false);
    m_result.clear();
    m_result_prev_segment.clear();
}

void april_engine::push_inbuf_to_samples() {
    auto end = m_in_buf.buf.cbegin();
    std::advance(end, m_in_buf.size);
    m_speech_buf.insert(m_speech_buf.end(), m_in_buf.buf.cbegin(), end);
}

stt_engine::samples_process_result_t april_engine::process_buff() {
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
        restart_session(false);
        m_result.clear();
        m_result_prev_segment.clear();
        m_prev_segment_end_time.reset();
        m_prev_segment_start_time.reset();
        reset_segment_counters();
        m_segments.clear();
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
            m_speech_buf.insert(m_speech_buf.end(), vad_buf.cbegin(),
                                vad_buf.cend());
        else
            m_speech_buf.insert(m_speech_buf.end(), m_in_buf.buf.cbegin(),
                                m_in_buf.buf.cbegin() + m_in_buf.size);

        restart_sentence_timer();
    } else {
        LOGD("vad: no speech");

        if (m_config.speech_mode == speech_mode_t::single_sentence &&
            (!m_intermediate_text || m_intermediate_text->empty()) &&
            sentence_timer_timed_out()) {
            LOGD("sentence timeout");
            m_call_backs.sentence_timeout();
        }

        if (m_speech_buf.empty())
            m_segment_time_discarded_before +=
                (1000 * m_in_buf.size) / m_sample_rate;
        else
            m_segment_time_discarded_after +=
                (1000 * m_in_buf.size) / m_sample_rate;
    }

    m_in_buf.clear();

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
        set_state(state_t::decoding);

        LOGD("speech frame: samples=" << m_speech_buf.size()
                                      << ", final=" << final_decode);

        m_segment_time_offset += m_segment_time_discarded_before;
        m_segment_time_discarded_before = 0;

        decode_speech(m_speech_buf, final_decode);

        m_segment_time_offset += m_segment_time_discarded_after;
        m_segment_time_discarded_after = 0;

        if (m_config.speech_started) set_state(state_t::idle);

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

void april_engine::decode_handler(void* user_data, AprilResultType result_type,
                                  size_t count, const AprilToken* tokens) {
    if (tokens && (result_type == APRIL_RESULT_RECOGNITION_FINAL ||
                   result_type == APRIL_RESULT_RECOGNITION_PARTIAL)) {
        auto* engine = static_cast<april_engine*>(user_data);

        if (!engine->m_result_prev_segment.empty() &&
            engine->m_result.empty()) {
            engine->m_prev_segment_end_time = tokens->time_ms;
        }

        engine->m_result.clear();

        for (size_t i = 0; i < count; ++i)
            engine->m_result.append(tokens[i].token);

        text_tools::to_lower_case(engine->m_result);

        if (result_type == APRIL_RESULT_RECOGNITION_FINAL) {
            engine->m_result_prev_segment.append(engine->m_result);
            engine->m_result.clear();
            engine->m_prev_segment_start_time = tokens->time_ms;
        }
    }
}

void april_engine::decode_speech(april_buf_t& buf, bool eof) {
    LOGD("speech decoding started");

    if (buf.size() > 0) aas_feed_pcm16(m_session, buf.data(), buf.size());
    if (eof) aas_flush(m_session);

#ifdef DEBUG
    LOGD("speech decoded: text=" << m_result);
#else
    LOGD("speech decoded");
#endif

    bool prev_segment_finished =
        m_config.text_format == text_format_t::subrip &&
        ((m_prev_segment_end_time && m_prev_segment_start_time) || eof);

    if (prev_segment_finished) {
        std::string result{
            m_result_prev_segment.cbegin() + m_result_size_consumed,
            m_result_prev_segment.cend()};

        m_result_size_consumed = m_result_prev_segment.size();

        ltrim(result);
        rtrim(result);

        if (m_prev_segment_start_time) {
            if (!m_prev_segment_end_time ||
                *m_prev_segment_end_time <= *m_prev_segment_start_time) {
                m_prev_segment_end_time =
                    *m_prev_segment_start_time +
                    result.size() * m_prev_segment_dur_per_token;
            } else {
                m_prev_segment_dur_per_token =
                    (*m_prev_segment_end_time - *m_prev_segment_start_time) /
                    result.size();
            }

            m_segments.push_back(
                {++m_segment_offset,
                 *m_prev_segment_start_time + m_segment_time_offset,
                 *m_prev_segment_end_time + m_segment_time_offset,
                 std::move(result)});
        }

        m_prev_segment_start_time.reset();
        m_prev_segment_end_time.reset();
    }

    if (eof && m_config.text_format == text_format_t::subrip) {
        ltrim(m_result_prev_segment);
        rtrim(m_result_prev_segment);

        if (m_punctuator) {
            m_result_prev_segment =
                m_punctuator->process(m_result_prev_segment);
        } else {
            text_tools::restore_caps(m_result_prev_segment);
        }

        text_tools::restore_punctuation_in_segments(m_result_prev_segment,
                                                    m_segments);

        text_tools::break_segments_to_multiline(
            m_config.sub_config.min_line_length,
            m_config.sub_config.max_line_length, m_segments);

        set_intermediate_text(text_tools::segments_to_subrip_text(m_segments),
                              m_config.lang);

        m_result_prev_segment.clear();
        m_result_size_consumed = 0;
        m_segments.clear();
    } else {
        auto result = m_result_prev_segment + m_result;

        ltrim(result);
        rtrim(result);

        if (m_punctuator) {
            result = m_punctuator->process(result);
        } else {
            text_tools::restore_caps(result);
        }

        if (!m_intermediate_text || m_intermediate_text != result) {
            set_intermediate_text(result, m_config.lang);
        }

        if (eof) m_result_prev_segment.clear();
    }
}

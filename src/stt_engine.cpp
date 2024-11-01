/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "stt_engine.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cstring>
#include <iterator>

#include "logger.hpp"

using namespace std::chrono_literals;

std::ostream& operator<<(std::ostream& os, stt_engine::speech_mode_t mode) {
    switch (mode) {
        case stt_engine::speech_mode_t::automatic:
            os << "automatic";
            break;
        case stt_engine::speech_mode_t::manual:
            os << "manual";
            break;
        case stt_engine::speech_mode_t::single_sentence:
            os << "single-sentence";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         stt_engine::speech_detection_status_t status) {
    switch (status) {
        case stt_engine::speech_detection_status_t::no_speech:
            os << "no-speech";
            break;
        case stt_engine::speech_detection_status_t::speech_detected:
            os << "speech-detected";
            break;
        case stt_engine::speech_detection_status_t::decoding:
            os << "decoding";
            break;
        case stt_engine::speech_detection_status_t::initializing:
            os << "initializing";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, stt_engine::lock_type_t lock_type) {
    switch (lock_type) {
        case stt_engine::lock_type_t::borrowed:
            os << "borrowed";
            break;
        case stt_engine::lock_type_t::free:
            os << "free";
            break;
        case stt_engine::lock_type_t::processed:
            os << "processed";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, stt_engine::flush_t flush_type) {
    switch (flush_type) {
        case stt_engine::flush_t::regular:
            os << "regular";
            break;
        case stt_engine::flush_t::eof:
            os << "eof";
            break;
        case stt_engine::flush_t::restart:
            os << "restart";
            break;
        case stt_engine::flush_t::exit:
            os << "exit";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         stt_engine::samples_process_result_t result) {
    switch (result) {
        case stt_engine::samples_process_result_t::no_samples_needed:
            os << "no-samples-needed";
            break;
        case stt_engine::samples_process_result_t::wait_for_samples:
            os << "wait-for-samples";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, stt_engine::state_t state) {
    switch (state) {
        case stt_engine::state_t::idle:
            os << "idle";
            break;
        case stt_engine::state_t::initializing:
            os << "initializing";
            break;
        case stt_engine::state_t::decoding:
            os << "decoding";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, stt_engine::vad_mode_t mode) {
    switch (mode) {
        case stt_engine::vad_mode_t::aggressiveness0:
            os << "aggressiveness-0";
            break;
        case stt_engine::vad_mode_t::aggressiveness1:
            os << "aggressiveness-1";
            break;
        case stt_engine::vad_mode_t::aggressiveness2:
            os << "aggressiveness-2";
            break;
        case stt_engine::vad_mode_t::aggressiveness3:
            os << "aggressiveness-3";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, stt_engine::gpu_api_t api) {
    switch (api) {
        case stt_engine::gpu_api_t::opencl:
            os << "opencl";
            break;
        case stt_engine::gpu_api_t::cuda:
            os << "cuda";
            break;
        case stt_engine::gpu_api_t::rocm:
            os << "rocm";
            break;
        case stt_engine::gpu_api_t::openvino:
            os << "openvino";
            break;
        case stt_engine::gpu_api_t::vulkan:
            os << "vulkan";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, stt_engine::audio_ctx_conf_t conf) {
    switch (conf) {
        case stt_engine::audio_ctx_conf_t::dynamic:
            os << "dynamic";
            break;
        case stt_engine::audio_ctx_conf_t::no_change:
            os << "no-change";
            break;
        case stt_engine::audio_ctx_conf_t::custom:
            os << "custom";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         stt_engine::text_format_t text_format) {
    switch (text_format) {
        case stt_engine::text_format_t::raw:
            os << "raw";
            break;
        case stt_engine::text_format_t::subrip:
            os << "subrip";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const stt_engine::model_files_t& model_files) {
    os << "model-file=" << model_files.model_file
       << ", scorer-file=" << model_files.scorer_file
       << ", openvino-file=" << model_files.openvino_model_file
       << ", ttt-model-file=" << model_files.ttt_model_file;

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const stt_engine::sub_config_t& sub_config) {
    os << "min-segment-dur=" << sub_config.min_segment_dur
       << ", min-line-length=" << sub_config.min_line_length
       << ", max-line-length=" << sub_config.max_line_length;

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const stt_engine::gpu_device_t& gpu_device) {
    os << "id=" << gpu_device.id << ", api=" << gpu_device.api
       << ", name=" << gpu_device.name
       << ", platform-name=" << gpu_device.platform_name
       << ", flash-attn=" << gpu_device.flash_attn;

    return os;
}

std::ostream& operator<<(std::ostream& os, const stt_engine::config_t& config) {
    os << "lang=" << config.lang << ", lang_code=" << config.lang_code
       << ", model-files=[" << config.model_files
       << "], cache-dir=" << config.cache_dir
       << ", speech-mode=" << config.speech_mode
       << ", vad-mode=" << config.vad_mode
       << ", speech-started=" << config.speech_started
       << ", text-format=" << config.text_format
       << ", cpu-threads=" << config.cpu_threads
       << ", beam-search=" << config.beam_search
       << ", audio-ctx-conf=" << config.audio_ctx_conf
       << ", audio-ctx-conf-size=" << config.audio_ctx_size
       << ", options=" << config.options
       << ", insert-stats=" << config.insert_stats
       << ", use-gpu=" << config.use_gpu << ", gpu-device=["
       << config.gpu_device << "]"
       << ", sub-config=[" << config.sub_config << "]"
       << ", translate=" << config.translate;
    return os;
}

stt_engine::stt_engine(config_t config, callbacks_t call_backs)
    : m_config{std::move(config)}, m_call_backs{std::move(call_backs)} {}

stt_engine::~stt_engine() { LOGD("stt dtor"); }

void stt_engine::start() {
    if (started()) {
        LOGW("stt already started");
        return;
    }

    LOGD("stt start");

    m_thread_exit_requested = true;
    if (m_processing_thread.joinable()) m_processing_thread.join();

    m_thread_exit_requested = false;
    reset_segment_counters();

    m_processing_thread = std::thread{&stt_engine::process, this};

    LOGD("stt start completed");
}

bool stt_engine::started() const {
    return m_processing_thread.joinable() && !m_thread_exit_requested;
}

bool stt_engine::stopping() const {
    return m_processing_thread.joinable() && m_thread_exit_requested;
}

void stt_engine::stop_processing_impl() {}
void stt_engine::start_processing_impl() {}

void stt_engine::request_stop() {
    if (m_thread_exit_requested) {
        LOGD("stt stop already requested");
        return;
    }

    LOGD("stt stop requested");

    m_thread_exit_requested = true;

    stop_processing_impl();

    m_processing_cv.notify_all();

    if (m_call_backs.stopping) m_call_backs.stopping();
}

void stt_engine::stop() {
    request_stop();

    if (!m_processing_thread.joinable()) {
        LOGD("stt already stopped");
        return;
    }

    m_processing_cv.notify_all();
    if (m_processing_thread.joinable()) m_processing_thread.join();
    m_config.speech_started = false;
    set_speech_detection_status(speech_detection_status_t::no_speech);
    set_state(state_t::idle);

    LOGD("stt stop completed");
}

void stt_engine::process() {
    LOGD("stt processing started");

    m_thread_exit_requested = false;

    try {
        set_state(state_t::initializing);
        start_processing_impl();
        set_state(state_t::idle);

        while (true) {
            std::unique_lock lock{m_processing_mtx};

            if (m_thread_exit_requested) break;

            if (m_restart_requested) {
                m_restart_requested = false;
                flush(flush_t::restart);
            }

            if (process_buff() == samples_process_result_t::wait_for_samples &&
                !m_thread_exit_requested)
                m_processing_cv.wait(lock);
        }

        flush(flush_t::exit);
    } catch (const std::runtime_error& e) {
        LOGE("stt processing error: " << e.what());

        if (m_call_backs.error) m_call_backs.error();
    }

    reset_in_processing();

    LOGD("stt processing ended");

    if (m_call_backs.stopped) m_call_backs.stopped();
}

bool stt_engine::lock_buf(lock_type_t desired_lock) {
    lock_type_t expected_lock = lock_type_t::free;
    return m_in_buf.lock.compare_exchange_strong(expected_lock, desired_lock);
}

void stt_engine::free_buf(lock_type_t lock) {
    lock_type_t expected_lock = lock;
    m_in_buf.lock.compare_exchange_strong(expected_lock, lock_type_t::free);
}

void stt_engine::free_buf() { m_in_buf.lock.store(lock_type_t::free); }

std::pair<char*, size_t> stt_engine::borrow_buf() {
    decltype(borrow_buf()) c_buf{nullptr, 0};

    if (m_thread_exit_requested) {
        return c_buf;
    }

    if (!lock_buf(lock_type_t::borrowed)) {
        LOGT("failed to lock for borrowing");
        return c_buf;
    }

    if (m_in_buf.full()) {
        LOGD("in-buf is full");
        free_buf();
        m_processing_cv.notify_one();
        return c_buf;
    }

    c_buf.first = reinterpret_cast<char*>(&m_in_buf.buf.at(m_in_buf.size));
    c_buf.second = (m_in_buf.buf.size() - m_in_buf.size) *
                   sizeof(in_buf_t::buf_t::value_type);

    return c_buf;
}

void stt_engine::return_buf(const char* c_buf, size_t size, bool sof,
                            bool eof) {
    if (m_in_buf.lock != lock_type_t::borrowed) return;

    LOGT("lock buff returned: sof=" << sof << ", eof=" << eof
                                    << ", buf size=" << size);

    m_in_buf.size =
        (c_buf - reinterpret_cast<char*>(m_in_buf.buf.data()) + size) /
        sizeof(in_buf_t::buf_t::value_type);
    m_in_buf.eof = eof;
    if (sof) m_in_buf.sof = sof;

    free_buf();
    m_processing_cv.notify_one();
}

bool stt_engine::lock_buff_for_processing() {
    if (!lock_buf(lock_type_t::processed)) {
        LOGT("failed to lock for processing");
        return false;
    }

    LOGT("lock buff for processing: sof=" << m_in_buf.sof
                                          << ", eof=" << m_in_buf.eof
                                          << ", buf size=" << m_in_buf.size);

    if (!m_in_buf.eof && m_in_buf.size < m_in_buf_max_size) {
        free_buf();
        return false;
    }

    return true;
}

void stt_engine::reset_in_processing() {
    LOGD("reset in processing");

    m_in_buf.clear();
    m_start_time.reset();
    m_vad.reset();
    m_intermediate_text.reset();
    set_speech_detection_status(speech_detection_status_t::no_speech);

    reset_impl();
}

stt_engine::samples_process_result_t stt_engine::process_buff() {
    return samples_process_result_t::wait_for_samples;
}

std::string stt_engine::merge_texts(const std::string& old_text,
                                    std::string&& new_text) {
    if (new_text.empty()) return old_text;

    if (old_text.empty()) return std::move(new_text);

    size_t i = 1, idx = 0;
    auto l = std::min(old_text.size(), new_text.size());
    for (; i <= l; ++i) {
        auto beg = old_text.cend();
        std::advance(beg, 0 - i);

        auto end = new_text.cbegin();
        std::advance(end, i);

        if (std::equal(beg, old_text.cend(), new_text.cbegin(), end)) idx = i;
    }

    if (idx > 0) {
        new_text = new_text.substr(idx);
        ltrim(new_text);
    }

    if (new_text.empty()) return old_text;
    return old_text + " " + new_text;
}

void stt_engine::set_intermediate_text(const std::string& text,
                                       const std::string& lang) {
    if (m_intermediate_text != text) {
        m_intermediate_text = text;
        m_intermediate_lang = lang;
        if (m_intermediate_text->empty() ||
            m_intermediate_text->size() >= m_min_text_size) {
            m_call_backs.intermediate_text_decoded(m_intermediate_text.value(),
                                                   m_intermediate_lang);
        }
    }
}

stt_engine::speech_detection_status_t stt_engine::speech_detection_status()
    const {
    switch (m_state) {
        case state_t::initializing:
            return speech_detection_status_t::initializing;
        case state_t::decoding:
            if (m_speech_detection_status ==
                speech_detection_status_t::speech_detected)
                break;
            else
                return speech_detection_status_t::decoding;
        case state_t::idle:
            break;
    }

    return m_speech_detection_status;
}

void stt_engine::set_state(state_t new_state) {
    if (m_state != new_state) {
        auto old_speech_status = speech_detection_status();

        LOGD("stt state: " << m_state << " => " << new_state);

        m_state = new_state;

        auto new_speech_status = speech_detection_status();

        if (old_speech_status != new_speech_status) {
            LOGD("speech detection status: "
                 << old_speech_status << " => " << new_speech_status << " ("
                 << m_speech_detection_status << ")");
            m_call_backs.speech_detection_status_changed(new_speech_status);
        }
    }
}

void stt_engine::flush(flush_t type) {
    LOGD("flush: " << type);

    if (m_config.speech_mode == speech_mode_t::automatic) {
        set_speech_detection_status(speech_detection_status_t::no_speech);
    } else if (type != flush_t::restart &&
               m_config.speech_mode == speech_mode_t::manual) {
        set_speech_started(false);
    }

    if (m_intermediate_text && !m_intermediate_text->empty()) {
        if ((type == flush_t::regular || type == flush_t::eof ||
             m_config.speech_mode != speech_mode_t::single_sentence) &&
            m_intermediate_text->size() >= m_min_text_size) {
            m_call_backs.text_decoded(m_intermediate_text.value(),
                                      m_intermediate_lang);

            if (m_config.speech_mode == speech_mode_t::single_sentence) {
                set_speech_started(false);
            }
        }
        set_intermediate_text("", m_intermediate_lang);
    }

    m_intermediate_text.reset();

    if (type == flush_t::eof) {
        m_call_backs.eof();
    }

    if (type == flush_t::restart) reset_segment_counters();
}

void stt_engine::set_speech_mode(speech_mode_t mode) {
    if (m_config.speech_mode != mode) {
        LOGD("speech mode: " << m_config.speech_mode << " => " << mode);

        m_config.speech_mode = mode;
        set_speech_started(false);
    }
}

void stt_engine::set_speech_started(bool value) {
    if (m_config.speech_started != value) {
        LOGD("speech started: " << m_config.speech_started << " => " << value);

        m_config.speech_started = value;
        m_start_time.reset();
        if (m_config.speech_mode == speech_mode_t::manual ||
            m_config.speech_mode == speech_mode_t::single_sentence) {
            set_speech_detection_status(
                value ? speech_detection_status_t::speech_detected
                      : speech_detection_status_t::no_speech);
        }
    }
}

void stt_engine::set_speech_detection_status(speech_detection_status_t status) {
    if (m_speech_detection_status == status) return;

    auto old_speech_status = speech_detection_status();

    m_speech_detection_status = status;

    auto new_speech_status = speech_detection_status();

    LOGD("speech detection status: " << old_speech_status << " => "
                                     << new_speech_status << " ("
                                     << m_speech_detection_status << ")");

    if (old_speech_status != new_speech_status)
        m_call_backs.speech_detection_status_changed(new_speech_status);
}

void stt_engine::ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

void stt_engine::rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

bool stt_engine::sentence_timer_timed_out() {
    if (m_start_time) {
        if (std::chrono::steady_clock::now() - *m_start_time >= m_timeout) {
            return true;
        }
    } else {
        restart_sentence_timer();
    }

    return false;
}

void stt_engine::restart_sentence_timer() {
    LOGT("staring sentence timer");
    m_start_time = std::chrono::steady_clock::now();
}

void stt_engine::create_punctuator() {
    if (m_punctuator || m_config.model_files.ttt_model_file.empty()) return;

    LOGD("creating punctuator");
    try {
        m_punctuator.emplace(m_config.model_files.ttt_model_file,
                             m_config.use_gpu ? m_config.gpu_device.id : -1);

        LOGD("punctuator created");
    } catch (const std::runtime_error& error) {
        LOGE("failed to create punctuator");
    }
}

void stt_engine::reset_segment_counters() {
    m_segment_offset = 0;
    m_segment_time_offset = 0;
    m_segment_time_discarded_before = 0;
    m_segment_time_discarded_after = 0;
}

std::string stt_engine::report_stats(size_t nb_samples, size_t sample_rate,
                                     size_t processing_duration_ms) {
    auto audio_dur = (1000 * nb_samples) / static_cast<double>(sample_rate);
    LOGD("stats: nb-samples="
         << nb_samples << ", audio-duration=" << static_cast<size_t>(audio_dur)
         << "ms, processing-duration=" << processing_duration_ms << "ms ("
         << static_cast<double>(processing_duration_ms) / audio_dur << ")");

    return fmt::format("[audio-length: {}ms, processing-time: {}ms]",
                       static_cast<size_t>(audio_dur), processing_duration_ms);
}

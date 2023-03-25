/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "engine_wrapper.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <numeric>
#include <sstream>

#include "logger.hpp"

using namespace std::chrono_literals;

std::ostream& operator<<(std::ostream& os, engine_wrapper::speech_mode_t mode) {
    switch (mode) {
        case engine_wrapper::speech_mode_t::automatic:
            os << "automatic";
            break;
        case engine_wrapper::speech_mode_t::manual:
            os << "manual";
            break;
        case engine_wrapper::speech_mode_t::single_sentence:
            os << "single-sentence";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         engine_wrapper::speech_detection_status_t status) {
    switch (status) {
        case engine_wrapper::speech_detection_status_t::no_speech:
            os << "no-speech";
            break;
        case engine_wrapper::speech_detection_status_t::speech_detected:
            os << "speech-detected";
            break;
        case engine_wrapper::speech_detection_status_t::decoding:
            os << "decoding";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         engine_wrapper::lock_type_t lock_type) {
    switch (lock_type) {
        case engine_wrapper::lock_type_t::borrowed:
            os << "borrowed";
            break;
        case engine_wrapper::lock_type_t::free:
            os << "free";
            break;
        case engine_wrapper::lock_type_t::processed:
            os << "processed";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         engine_wrapper::flush_t flush_type) {
    switch (flush_type) {
        case engine_wrapper::flush_t::regular:
            os << "regular";
            break;
        case engine_wrapper::flush_t::eof:
            os << "eof";
            break;
        case engine_wrapper::flush_t::restart:
            os << "restart";
            break;
        case engine_wrapper::flush_t::exit:
            os << "exit";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         engine_wrapper::samples_process_result_t result) {
    switch (result) {
        case engine_wrapper::samples_process_result_t::no_samples_needed:
            os << "no-samples-needed";
            break;
        case engine_wrapper::samples_process_result_t::wait_for_samples:
            os << "wait-for-samples";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, engine_wrapper::vad_mode_t mode) {
    switch (mode) {
        case engine_wrapper::vad_mode_t::aggressiveness0:
            os << "aggressiveness-0";
            break;
        case engine_wrapper::vad_mode_t::aggressiveness1:
            os << "aggressiveness-1";
            break;
        case engine_wrapper::vad_mode_t::aggressiveness2:
            os << "aggressiveness-2";
            break;
        case engine_wrapper::vad_mode_t::aggressiveness3:
            os << "aggressiveness-3";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const engine_wrapper::config_t& config) {
    os << "lang=" << config.lang << ", model-file-1=" << config.model_file.first
       << ", model-file-2=" << config.model_file.second
       << ", speech-mode=" << config.speech_mode
       << ", vad-mode=" << config.vad_mode
       << ", speech-started=" << config.speech_started;

    return os;
}

engine_wrapper::engine_wrapper(config_t config, callbacks_t call_backs)
    : m_model_file{std::move(config.model_file)},
      m_lang{std::move(config.lang)}, m_call_backs{std::move(call_backs)},
      m_processing_thread{&engine_wrapper::start_processing, this},
      m_speech_started{config.speech_started}, m_speech_mode{
                                                   config.speech_mode} {}

engine_wrapper::~engine_wrapper() { LOGD("engine dtor"); }

void engine_wrapper::stop_processing_impl() {}

void engine_wrapper::stop_processing() {
    m_thread_exit_requested = true;

    LOGD("exit requested");

    stop_processing_impl();

    if (!m_processing_thread.joinable()) {
        LOGD("processing thread already stopped");
        return;
    }

    m_processing_cv.notify_all();
    m_processing_thread.join();
    m_speech_started = false;
    set_speech_detection_status(speech_detection_status_t::no_speech);
}

void engine_wrapper::start_processing() {
    LOGD("processing thread started");

    m_thread_exit_requested = false;

    try {
        while (true) {
            LOGT("processing iter");

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
    } catch (const std::system_error& e) {
        LOGE("system error: " << e.what());
    }

    LOGD("processing thread ended");
}

bool engine_wrapper::lock_buf(lock_type_t desired_lock) {
    lock_type_t expected_lock = lock_type_t::free;
    return m_in_buf.lock.compare_exchange_strong(expected_lock, desired_lock);
}

void engine_wrapper::free_buf(lock_type_t lock) {
    lock_type_t expected_lock = lock;
    m_in_buf.lock.compare_exchange_strong(expected_lock, lock_type_t::free);
}

void engine_wrapper::free_buf() { m_in_buf.lock.store(lock_type_t::free); }

std::pair<char*, size_t> engine_wrapper::borrow_buf() {
    decltype(borrow_buf()) c_buf{nullptr, 0};

    if (!lock_buf(lock_type_t::borrowed)) {
        return c_buf;
    }

    if (m_in_buf.full()) {
        LOGD("in-buf is full");
        free_buf();
        return c_buf;
    }

    c_buf.first = reinterpret_cast<char*>(&m_in_buf.buf.at(m_in_buf.size));
    c_buf.second = (m_in_buf.buf.size() - m_in_buf.size) *
                   sizeof(in_buf_t::buf_t::value_type);

    return c_buf;
}

void engine_wrapper::return_buf(const char* c_buf, size_t size, bool sof,
                                bool eof) {
    if (m_in_buf.lock != lock_type_t::borrowed) return;

    m_in_buf.size =
        (c_buf - reinterpret_cast<char*>(m_in_buf.buf.data()) + size) /
        sizeof(in_buf_t::buf_t::value_type);
    m_in_buf.eof = eof;
    if (sof) m_in_buf.sof = sof;

    free_buf();
    m_processing_cv.notify_one();
}

bool engine_wrapper::lock_buff_for_processing() {
    if (!lock_buf(lock_type_t::processed)) {
        LOGW("failed to lock for processing, buf is not free");
        return false;
    }

    LOGT("lock buff for processing: eof=" << m_in_buf.eof << ", buf size"
                                          << m_in_buf.size);

    if (!m_in_buf.eof && m_in_buf.size < m_in_buf_max_size) {
        free_buf();
        return false;
    }

    return true;
}

void engine_wrapper::reset() {
    LOGD("reset");

    m_in_buf.clear();
    m_start_time.reset();
    m_vad.reset();

    reset_impl();
}

engine_wrapper::samples_process_result_t engine_wrapper::process_buff() {
    return samples_process_result_t::wait_for_samples;
}

std::string engine_wrapper::merge_texts(const std::string& old_text,
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

void engine_wrapper::set_intermediate_text(const std::string& text) {
    if (m_intermediate_text != text) {
        m_intermediate_text = text;
        if (m_intermediate_text->empty() ||
            m_intermediate_text->size() >= m_min_text_size) {
            m_call_backs.intermediate_text_decoded(m_intermediate_text.value());
        }
    }
}

void engine_wrapper::flush(flush_t type) {
    LOGD("flush: " << type);

    if (m_speech_mode == speech_mode_t::automatic) {
        set_speech_detection_status(speech_detection_status_t::no_speech);
    } else if (type != flush_t::restart &&
               m_speech_mode == speech_mode_t::manual) {
        set_speech_started(false);
    }

    if (m_intermediate_text && !m_intermediate_text->empty()) {
        if ((type == flush_t::regular || type == flush_t::eof ||
             m_speech_mode != speech_mode_t::single_sentence) &&
            m_intermediate_text->size() >= m_min_text_size) {
            m_call_backs.text_decoded(m_intermediate_text.value());

            if (m_speech_mode == speech_mode_t::single_sentence) {
                set_speech_started(false);
            }
        }
        set_intermediate_text("");
    }

    m_intermediate_text.reset();

    if (type == flush_t::eof) {
        m_call_backs.eof();
    }
}

void engine_wrapper::set_speech_mode(speech_mode_t mode) {
    if (m_speech_mode != mode) {
        LOGD("speech mode: " << m_speech_mode << " => " << mode);

        m_speech_mode = mode;
        set_speech_started(false);
    }
}

void engine_wrapper::set_speech_started(bool value) {
    if (m_speech_started != value) {
        LOGD("speech started: " << m_speech_started << " => " << value);

        m_speech_started = value;
        m_start_time.reset();
        if (m_speech_mode == speech_mode_t::manual ||
            m_speech_mode == speech_mode_t::single_sentence) {
            set_speech_detection_status(
                value ? speech_detection_status_t::speech_detected
                      : speech_detection_status_t::no_speech);
        }
    }
}

void engine_wrapper::set_speech_detection_status(
    speech_detection_status_t status) {
    if (m_speech_detection_status != status) {
        LOGD("speech detection status: " << m_speech_detection_status << " => "
                                         << status);
        m_speech_detection_status = status;
        m_call_backs.speech_detection_status_changed(status);
    }
}

void engine_wrapper::ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

void engine_wrapper::rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

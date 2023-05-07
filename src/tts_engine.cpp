/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tts_engine.hpp"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>

#include "logger.hpp"

std::ostream& operator<<(std::ostream& os,
                         const tts_engine::model_files_t& model_files) {
    os << "model-path=" << model_files.model_path
       << ", vocoder-path=" << model_files.vocoder_path;

    return os;
}

std::ostream& operator<<(std::ostream& os, const tts_engine::config_t& config) {
    os << "lang=" << config.lang << ", speaker=" << config.speaker
       << ", model-files=[" << config.model_files << "]";

    return os;
}

std::ostream& operator<<(std::ostream& os, tts_engine::state_t state) {
    switch (state) {
        case tts_engine::state_t::idle:
            os << "idle";
            break;
        case tts_engine::state_t::initializing:
            os << "initializing";
            break;
        case tts_engine::state_t::encoding:
            os << "encoding";
            break;
        case tts_engine::state_t::error:
            os << "error";
            break;
    }

    return os;
}

tts_engine::tts_engine(config_t config, callbacks_t call_backs)
    : m_config{std::move(config)},
      m_call_backs{std::move(call_backs)},
      m_processing_thread{&tts_engine::process, this} {}

tts_engine::~tts_engine() {
    LOGD("tts dtor");
}

void tts_engine::stop() {
    LOGD("tts stop started");

    m_shutting_down = true;
    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    LOGD("tts stop completed");
}

std::string tts_engine::first_file_with_ext(std::string dir_path,
                                            std::string&& ext) {
    auto* dirp = opendir(dir_path.c_str());
    if (!dirp) return {};

    while (auto* dirent = readdir(dirp)) {
        if (dirent->d_type != DT_REG) continue;

        std::string fn{dirent->d_name};

        if (fn.substr(fn.find_last_of('.') + 1) == ext)
            return dir_path.append("/").append(fn);
    }

    return {};
}

std::string tts_engine::find_file_with_name_prefix(std::string dir_path,
                                                   std::string&& prefix) {
    auto* dirp = opendir(dir_path.c_str());
    if (!dirp) return {};

    while (auto* dirent = readdir(dirp)) {
        if (dirent->d_type != DT_REG) continue;

        std::string fn{dirent->d_name};

        if (fn.size() < prefix.size()) continue;

        if (fn.substr(0, prefix.size()) == prefix)
            return dir_path.append("/").append(fn);
    }

    return {};
}

void tts_engine::encode_speech(std::string text) {
    if (m_shutting_down) return;

    {
        std::lock_guard lock{m_mutex};
        m_queue.push(std::move(text));
    }

    LOGD("task pushed");

    m_cv.notify_one();
}

void tts_engine::set_state(state_t new_state) {
    if (m_state != new_state) {
        LOGD("state: " << m_state << " => " << new_state);
        m_state = new_state;
        m_call_backs.state_changed(m_state);
    }
}

std::string tts_engine::path_to_output_file(const std::string& text) const {
    auto hash = std::hash<std::string>{}(
        text + m_config.model_files.model_path +
        m_config.model_files.vocoder_path + m_config.speaker);
    return m_config.cache_dir + "/" + std::to_string(hash) + ".wav";
}

static bool file_exists(const std::string& file_path) {
    struct stat buffer {};
    return stat(file_path.c_str(), &buffer) == 0;
}

void tts_engine::process() {
    LOGD("tts prosessing started");

    decltype(m_queue) queue;

    while (!m_shutting_down && m_state != state_t::error) {
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_cv.wait(lock, [this] {
                return (m_shutting_down || m_state == state_t::error) ||
                       !m_queue.empty();
            });
            std::swap(queue, m_queue);
        }

        if (m_shutting_down || m_state == state_t::error) break;

        if (!model_created()) {
            set_state(state_t::initializing);

            create_model();

            if (!model_created()) {
                set_state(state_t::error);
                if (m_call_backs.error) m_call_backs.error();
                break;
            }
        }

        set_state(state_t::encoding);

        while (!m_shutting_down && !queue.empty()) {
            auto text = std::move(queue.front());
            queue.pop();

            auto output_file = path_to_output_file(text);

            LOGD("tts out file: " << output_file);

            if (!file_exists(output_file) &&
                !encode_speech_impl(text, output_file)) {
                if (m_call_backs.error) m_call_backs.error();
                break;
            }

            if (m_call_backs.speech_encoded) {
                m_call_backs.speech_encoded(output_file);
            }
        }

        set_state(state_t::idle);
    }

    if (m_shutting_down) set_state(state_t::idle);

    LOGD("tts processing done");
}

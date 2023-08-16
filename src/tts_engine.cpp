/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tts_engine.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>

#include "logger.hpp"
#include "text_tools.hpp"

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
      m_call_backs{std::move(call_backs)} {}

tts_engine::~tts_engine() {
    LOGD("tts dtor");
}

void tts_engine::start() {
    LOGD("tts start");

    m_shutting_down = true;
    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    m_queue = std::queue<task_t>{};
    m_state = state_t::idle;
    m_shutting_down = false;
    m_processing_thread = std::thread{&tts_engine::process, this};

    LOGD("tts start completed");
}

void tts_engine::stop() {
    LOGD("tts stop started");

    m_shutting_down = true;

    set_state(state_t::idle);

    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    LOGD("tts stop completed");
}

void tts_engine::request_stop() {
    LOGD("tts stop requested");

    m_shutting_down = true;

    set_state(state_t::idle);
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
                                                   std::string prefix) {
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

    auto tasks = make_tasks(text);

    {
        std::lock_guard lock{m_mutex};
        for (auto& task : tasks) {
            LOGD("task: " << task.text);
            m_queue.push(std::move(task));
        }
    }

    LOGD("task pushed");

    m_cv.notify_one();
}

void tts_engine::set_state(state_t new_state) {
    if (m_shutting_down) new_state = state_t::idle;

    if (m_state != new_state) {
        LOGD("tts engine state: " << m_state << " => " << new_state);
        m_state = new_state;
        m_call_backs.state_changed(m_state);
    }
}

std::string tts_engine::path_to_output_file(const std::string& text) const {
    auto hash = std::hash<std::string>{}(
        text + m_config.model_files.model_path +
        m_config.model_files.vocoder_path + m_config.speaker + m_config.lang);
    return m_config.cache_dir + "/" + std::to_string(hash) + ".wav";
}

static bool file_exists(const std::string& file_path) {
    struct stat buffer {};
    return stat(file_path.c_str(), &buffer) == 0;
}

std::vector<tts_engine::task_t> tts_engine::make_tasks(const std::string& text,
                                                       bool split) const {
    std::vector<tts_engine::task_t> tasks;

    if (split) {
        auto [parts, _] = text_tools::split(text, m_config.nb_data);
        if (!parts.empty()) {
            tasks.reserve(parts.size());

            for (auto& part : parts)
                tasks.push_back(task_t{std::move(part), false});

            tasks.back().last = true;
        }
    } else {
        tasks.push_back(task_t{text, true});
    }

    return tasks;
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
            auto task = std::move(queue.front());
            queue.pop();

            auto output_file = path_to_output_file(task.text);

            LOGD("tts out file: " << output_file);

            if (!file_exists(output_file)) {
                if (!encode_speech_impl(task.text, output_file)) {
                    unlink(output_file.c_str());
                    if (m_call_backs.error) m_call_backs.error();
                    break;
                }
            }

            if (m_call_backs.speech_encoded) {
                m_call_backs.speech_encoded(task.text, output_file, task.last);
            }
        }

        set_state(state_t::idle);
    }

    if (m_shutting_down) set_state(state_t::idle);

    LOGD("tts processing done");
}

// borrowed from:
// https://github.com/rhasspy/piper/blob/master/src/cpp/wavfile.hpp
void tts_engine::write_wav_header(int sample_rate, int sample_width,
                                  int channels, uint32_t num_samples,
                                  std::ostream& wav_file) {
    wav_header header;
    header.data_size = num_samples * sample_width * channels;
    header.chunk_size = header.data_size + sizeof(wav_header) - 8;
    header.sample_rate = sample_rate;
    header.num_channels = channels;
    header.bytes_per_sec = sample_rate * sample_width * channels;
    header.block_align = sample_width * channels;
    wav_file.write(reinterpret_cast<const char*>(&header), sizeof(wav_header));
}

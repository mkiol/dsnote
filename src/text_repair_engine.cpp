/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "text_repair_engine.hpp"

#include <dirent.h>
#include <fmt/format.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <locale>

#include "logger.hpp"

std::ostream& operator<<(std::ostream& os, text_repair_engine::gpu_api_t api) {
    switch (api) {
        case text_repair_engine::gpu_api_t::opencl:
            os << "opencl";
            break;
        case text_repair_engine::gpu_api_t::cuda:
            os << "cuda";
            break;
        case text_repair_engine::gpu_api_t::rocm:
            os << "rocm";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         text_repair_engine::text_format_t text_format) {
    switch (text_format) {
        case text_repair_engine::text_format_t::raw:
            os << "raw";
            break;
        case text_repair_engine::text_format_t::subrip:
            os << "subrip";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const text_repair_engine::model_files_t& model_files) {
    os << "diacritizer_he=" << model_files.diacritizer_path_he
       << ", diacritizer_ar=" << model_files.diacritizer_path_ar
       << ", punctuator=" << model_files.punctuator_path;

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const text_repair_engine::gpu_device_t& gpu_device) {
    os << "id=" << gpu_device.id << ", api=" << gpu_device.api
       << ", name=" << gpu_device.name
       << ", platform-name=" << gpu_device.platform_name;

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         text_repair_engine::task_type_t task_type) {
    switch (task_type) {
        case text_repair_engine::task_type_t::restore_diacritics_ar:
            os << "restore-diacritics-ar";
            break;
        case text_repair_engine::task_type_t::restore_diacritics_he:
            os << "restore-diacritics-he";
            break;
        case text_repair_engine::task_type_t::restore_punctuation:
            os << "restore-punctuation";
            break;
        case text_repair_engine::task_type_t::none:
            os << "none";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const text_repair_engine::config_t& config) {
    os << "model-files=[" << config.model_files << "]"
       << ", text-format=" << config.text_format
       << ", share-dir=" << config.share_dir << ", data-dir=" << config.data_dir
       << ", use-gpu=" << config.use_gpu << ", gpu-device=["
       << config.gpu_device << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, text_repair_engine::state_t state) {
    switch (state) {
        case text_repair_engine::state_t::idle:
            os << "idle";
            break;
        case text_repair_engine::state_t::stopping:
            os << "stopping";
            break;
        case text_repair_engine::state_t::stopped:
            os << "stopped";
            break;
        case text_repair_engine::state_t::processing:
            os << "processing";
            break;
        case text_repair_engine::state_t::error:
            os << "error";
            break;
    }

    return os;
}

text_repair_engine::text_repair_engine(config_t config, callbacks_t call_backs)
    : m_config{std::move(config)}, m_call_backs{std::move(call_backs)} {}

text_repair_engine::~text_repair_engine() {
    LOGD("text-repair dtor");
}

void text_repair_engine::start() {
    LOGD("text-repair start");

    m_state = state_t::stopping;
    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    m_queue = std::queue<task_t>{};
    m_state = state_t::idle;
    m_processing_thread = std::thread{&text_repair_engine::process, this};

    LOGD("text-repair start completed");
}

void text_repair_engine::stop() {
    LOGD("text-repair stop started");

    set_state(state_t::stopping);

    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    set_state(state_t::stopped);

    LOGD("text-repair stop completed");
}

void text_repair_engine::request_stop() {
    LOGD("text-repair stop requested");

    set_state(state_t::stopping);
    m_cv.notify_one();
}

void text_repair_engine::repair_text(const std::string& text,
                                     task_type_t task_type) {
    if (is_shutdown()) return;

    LOGD("text-repair task: " << task_type);

    auto tasks = make_tasks(text, task_type);

    if (tasks.empty()) {
        LOGW("no task to process");
        tasks.push_back(task_t{"", task_type, true, true});
    }

    {
        std::lock_guard lock{m_mutex};
        for (auto& task : tasks) m_queue.push(std::move(task));
    }

    LOGD("task pushed");

    m_cv.notify_one();
}

void text_repair_engine::set_state(state_t new_state) {
    if (is_shutdown()) {
        if (m_state == state_t::error) return;

        switch (new_state) {
            case state_t::idle:
            case state_t::stopping:
            case state_t::processing:
                new_state = state_t::stopping;
                break;
            case state_t::stopped:
            case state_t::error:
                break;
        }
    }

    if (m_state != new_state) {
        LOGD("text-repair engine state: " << m_state << " => " << new_state);
        m_state = new_state;
        m_call_backs.state_changed(m_state);
    }
}

std::vector<text_repair_engine::task_t> text_repair_engine::make_tasks(
    const std::string& text, task_type_t task_type) const {
    std::vector<text_repair_engine::task_t> tasks;

    if (m_config.text_format == text_format_t::subrip) {
        auto subrip_start_idx = text_tools::subrip_text_start(text, 100);
        if (subrip_start_idx) {
            auto segments =
                text_tools::subrip_text_to_segments(text, *subrip_start_idx);

            if (!segments.empty()) {
                tasks.reserve(segments.size());

                tasks.push_back(task_t{std::move(segments.front().text),
                                       task_type, true, false});

                for (auto it = segments.begin() + 1; it != segments.end();
                     ++it) {
                    tasks.push_back(
                        task_t{std::move(it->text), task_type, false, false});
                }

                tasks.back().last = true;

                return tasks;
            }
        }

        LOGW("text-repair fallback to plain text");
    }

    if (task_type != task_type_t::restore_punctuation) {
        auto [parts, _] =
            text_tools::split(text, text_tools::split_engine_t::astrunc, [&] {
                switch (task_type) {
                    case task_type_t::restore_diacritics_ar:
                        return "ar";
                    case task_type_t::restore_diacritics_he:
                        return "he";
                    case task_type_t::restore_punctuation:
                    case task_type_t::none:
                        break;
                }
                throw std::runtime_error{"invalid task type"};
            }());
        if (!parts.empty()) {
            tasks.reserve(parts.size());
            tasks.push_back(
                task_t{std::move(parts.front()), task_type, true, false});

            for (auto it = parts.begin() + 1; it != parts.end(); ++it) {
                text_tools::trim_line(*it);
                if (!it->empty())
                    tasks.push_back(
                        task_t{std::move(*it), task_type, false, false});
            }

            tasks.back().last = true;
        }
    } else {
        tasks.push_back(task_t{text, task_type, true, true});
    }

    return tasks;
}

void text_repair_engine::process_task(task_t& task,
                                      std::string& repaired_text) {
    switch (task.type) {
        case task_type_t::restore_diacritics_ar:
        case task_type_t::restore_diacritics_he:
            if (!m_text_processor)
                m_text_processor.emplace(
                    m_config.use_gpu ? m_config.gpu_device.id : -1);
            break;
        case task_type_t::restore_punctuation:
            if (!m_punctuator)
                m_punctuator.emplace(
                    m_config.model_files.punctuator_path,
                    m_config.use_gpu ? m_config.gpu_device.id : -1);
            break;
        case task_type_t::none:
            throw std::runtime_error{"invalid task type"};
    }

    switch (task.type) {
        case task_type_t::restore_diacritics_ar:
            m_text_processor->arabic_diacritize(
                task.text, m_config.model_files.diacritizer_path_ar);
            break;
        case task_type_t::restore_diacritics_he:
            m_text_processor->hebrew_diacritize(
                task.text, m_config.model_files.diacritizer_path_he);
            break;
        case task_type_t::restore_punctuation:
            task.text = m_punctuator->process(task.text);
            break;
        case task_type_t::none:
            throw std::runtime_error{"invalid task type"};
    }

    if (task.text.empty()) return;

    if (!repaired_text.empty()) repaired_text.append(" ");
    repaired_text.append(task.text);
}

void text_repair_engine::process() {
    LOGD("text-repair prosessing started");

    decltype(m_queue) queue;

    while (!is_shutdown()) {
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_cv.wait(lock,
                      [this] { return is_shutdown() || !m_queue.empty(); });
            std::swap(queue, m_queue);
        }

        if (is_shutdown()) break;

        std::string repaired_text;

        while (!is_shutdown() && !queue.empty()) {
            set_state(state_t::processing);

            auto task = std::move(queue.front());

            if (task.first) repaired_text.clear();

            queue.pop();

            if (task.empty() && task.last) {
                if (m_call_backs.text_repaired) {
                    m_call_backs.text_repaired(repaired_text);
                }
                continue;
            }

            try {
                process_task(task, repaired_text);

                if (m_call_backs.text_repaired && task.last) {
                    m_call_backs.text_repaired(repaired_text);
                    repaired_text.clear();
                }
            } catch (const std::exception& error) {
                LOGE("error: " << error.what());
                set_state(state_t::error);
            }
        }

        if (!is_shutdown()) set_state(state_t::idle);
    }

    if (m_state != state_t::error) set_state(state_t::stopped);

    LOGD("text-repair processing done");
}

/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef TEXT_REPAIR_ENGINE_HPP
#define TEXT_REPAIR_ENGINE_HPP

#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "text_tools.hpp"

class text_repair_engine {
   public:
    enum class state_t { idle, stopping, processing, stopped, error };
    friend std::ostream& operator<<(std::ostream& os, state_t state);

    enum class gpu_api_t { opencl, cuda, rocm };
    friend std::ostream& operator<<(std::ostream& os, gpu_api_t api);

    enum class text_format_t { raw, subrip };
    friend std::ostream& operator<<(std::ostream& os,
                                    text_format_t text_format);

    struct model_files_t {
        std::string diacritizer_path_he;
        std::string diacritizer_path_ar;
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const model_files_t& model_files);

    struct callbacks_t {
        std::function<void(const std::string& text)> text_repaired;
        std::function<void(state_t state)> state_changed;
        std::function<void()> error;
    };

    struct gpu_device_t {
        int id = -1;
        gpu_api_t api = gpu_api_t::opencl;
        std::string name;
        std::string platform_name;

        inline bool operator==(const gpu_device_t& rhs) const {
            return platform_name == rhs.platform_name && name == rhs.name &&
                   id == rhs.id;
        }
        inline bool operator!=(const gpu_device_t& rhs) const {
            return !(*this == rhs);
        }
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const gpu_device_t& gpu_device);

    enum class task_type_t {
        none = 0,
        restore_diacritics_ar = 1,
        restore_diacritics_he = 2
    };
    friend std::ostream& operator<<(std::ostream& os, task_type_t task_type);

    struct config_t {
        model_files_t model_files;
        std::string data_dir;
        std::string config_dir;
        std::string share_dir;
        text_format_t text_format = text_format_t::raw;
        task_type_t task = task_type_t::none;
        std::string options;
        bool use_gpu = false;
        gpu_device_t gpu_device;
        inline bool has_option(char c) const {
            return options.find(c) != std::string::npos;
        }
    };
    friend std::ostream& operator<<(std::ostream& os, const config_t& config);

    text_repair_engine(config_t config, callbacks_t call_backs);
    ~text_repair_engine();
    void start();
    void stop();
    void request_stop();
    inline auto model_files() const { return m_config.model_files; }
    inline auto state() const { return m_state; }
    inline auto use_gpu() const { return m_config.use_gpu; }
    inline auto gpu_device() const { return m_config.gpu_device; }
    inline auto text_format() const { return m_config.text_format; }
    inline void set_text_format(text_format_t value) {
        m_config.text_format = value;
    }
    void repair_text(const std::string& text, task_type_t task_type);

   protected:
    struct task_t {
        std::string text;
        task_type_t type;
        bool first = false;
        bool last = false;

        inline bool empty() const { return text.empty(); }
    };

    config_t m_config;
    callbacks_t m_call_backs;
    std::thread m_processing_thread;
    std::queue<task_t> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    state_t m_state = state_t::idle;
    text_tools::processor m_text_processor;

    void set_state(state_t new_state);
    void process();
    void process_task(task_t& task, std::string& repaired_text);
    std::vector<task_t> make_tasks(const std::string& text, bool split,
                                   task_type_t task_type) const;
    inline bool is_shutdown() const {
        return m_state == state_t::stopping || m_state == state_t::stopped ||
               m_state == state_t::error;
    }
};

#endif // TEXT_REPAIR_ENGINE_HPP

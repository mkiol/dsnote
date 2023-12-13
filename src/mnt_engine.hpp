/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MNT_ENGINE_HPP
#define MNT_ENGINE_HPP

#include <bergamot_api.h>

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

class mnt_engine {
   public:
    enum class state_t { idle, initializing, translating, error };
    friend std::ostream& operator<<(std::ostream& os, state_t state);

    struct model_files_t {
        std::string model_path_first;
        std::string model_path_second;

        inline bool operator==(const model_files_t& rhs) const {
            return model_path_first == rhs.model_path_first &&
                   model_path_second == rhs.model_path_second;
        };
        inline bool operator!=(const model_files_t& rhs) const {
            return !(*this == rhs);
        };
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const model_files_t& model_files);

    struct callbacks_t {
        std::function<void(const std::string& in_text,
                           const std::string& in_lang, std::string&& out_text,
                           const std::string& out_lang)>
            text_translated;
        std::function<void(state_t state)> state_changed;
        std::function<void()> error;
    };

    struct config_t {
        std::string lang;
        model_files_t model_files;
        std::string nb_data;
        std::string out_lang;
        bool text_is_html = false;
        std::string options;
        bool clean_text = false;
    };
    friend std::ostream& operator<<(std::ostream& os, const config_t& config);

    mnt_engine(config_t config, callbacks_t call_backs);
    virtual ~mnt_engine();
    void start();
    void stop();
    void request_stop();
    inline auto lang() const { return m_config.lang; }
    inline auto model_files() const { return m_config.model_files; }
    inline auto text_is_html() const { return m_config.text_is_html; }
    inline void set_text_is_html(bool value) { m_config.text_is_html = value; }
    inline void set_clean_text(bool value) { m_config.clean_text = value; }
    inline auto state() const { return m_state; }
    void translate(std::string text);

   private:
    struct task_t {
        std::string text;
    };

    struct bergamot_api_api {
        void* (*bergamot_api_make)(const char* model_path,
                                   const char* src_vocab_path,
                                   const char* trg_vocab_path,
                                   const char* shortlist_path,
                                   size_t num_workers, size_t cache_size,
                                   const char* log_level) = nullptr;
        void (*bergamot_api_delete)(void* handle) = nullptr;
        const char* (*bergamot_api_translate)(void* handle, const char* text,
                                              bool text_is_html) = nullptr;
        void (*bergamot_api_cancel)(void* handle) = nullptr;
        inline auto ok() const {
            return bergamot_api_make && bergamot_api_delete &&
                   bergamot_api_translate && bergamot_api_cancel;
        }
    };

    config_t m_config;
    callbacks_t m_call_backs;
    bergamot_api_api m_bergamot_api_api;
    void* m_bergamotlib_handle = nullptr;
    std::thread m_processing_thread;
    bool m_shutting_down = false;
    std::queue<task_t> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    state_t m_state = state_t::idle;
    void* m_bergamot_ctx_first = nullptr;
    void* m_bergamot_ctx_second = nullptr;

    static std::string find_file_with_name_prefix(std::string dir_path,
                                                  std::string prefix);

    bool model_created() const;
    void create_model();
    void set_state(state_t new_state);
    void process();
    std::string translate_internal(std::string text);
    void open_bergamot_lib();
};

#endif // MNT_ENGINE_HPP

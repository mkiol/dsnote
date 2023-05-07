/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef TTS_ENGINE_HPP
#define TTS_ENGINE_HPP

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

class tts_engine {
   public:
    enum class state_t { idle, initializing, encoding, error };
    friend std::ostream& operator<<(std::ostream& os, state_t state);

    struct model_files_t {
        std::string model_path;
        std::string vocoder_path;

        inline bool operator==(const model_files_t& rhs) const {
            return model_path == rhs.model_path &&
                   vocoder_path == rhs.vocoder_path;
        };
        inline bool operator!=(const model_files_t& rhs) const {
            return !(*this == rhs);
        };
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const model_files_t& model_files);

    struct callbacks_t {
        std::function<void(const std::string& wav_file_path)> speech_encoded;
        std::function<void(state_t state)> state_changed;
        std::function<void()> error;
    };

    struct config_t {
        std::string lang;
        model_files_t model_files;
        std::string speaker;
        std::string cache_dir;
    };
    friend std::ostream& operator<<(std::ostream& os, const config_t& config);

    tts_engine(config_t config, callbacks_t call_backs);
    virtual ~tts_engine();
    inline auto lang() const { return m_config.lang; }
    inline auto model_files() const { return m_config.model_files; }
    inline auto state() const { return m_state; }
    inline auto speaker() const { return m_config.speaker; }
    void encode_speech(std::string text);

   protected:
    config_t m_config;
    callbacks_t m_call_backs;
    std::thread m_processing_thread;
    bool m_shutting_down = false;
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    state_t m_state = state_t::idle;

    static std::string first_file_with_ext(std::string dir_path,
                                           std::string&& ext);
    static std::string find_file_with_name_prefix(std::string dir_path,
                                                  std::string&& prefix);
    virtual bool model_created() const = 0;
    virtual void create_model() = 0;
    virtual bool encode_speech_impl(const std::string& text,
                                    const std::string& out_file) = 0;
    void stop();
    void set_state(state_t new_state);
    std::string path_to_output_file(const std::string& text) const;
    void process();
};

#endif // TTS_ENGINE_HPP

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ENGINE_WRAPPER_H
#define ENGINE_WRAPPER_H

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <thread>
#include <utility>

#include "vad_wrapper.hpp"

using namespace std::chrono_literals;

class engine_wrapper {
   public:
    using model_file_t = std::pair<std::string, std::string>;

    enum class speech_mode_t { automatic = 0, manual = 1, single_sentence = 2 };
    friend std::ostream& operator<<(std::ostream& os, speech_mode_t mode);

    enum class speech_detection_status_t {
        no_speech = 0,
        speech_detected = 1,
        decoding = 2
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    speech_detection_status_t status);

    enum class vad_mode_t {
        aggressiveness0 = 0,
        aggressiveness1 = 1,
        aggressiveness2 = 2,
        aggressiveness3 = 3
    };
    friend std::ostream& operator<<(std::ostream& os, vad_mode_t mode);

    struct callbacks_t {
        std::function<void(const std::string& text)> text_decoded;
        std::function<void(const std::string& text)> intermediate_text_decoded;
        std::function<void(speech_detection_status_t status)>
            speech_detection_status_changed;
        std::function<void()> sentence_timeout;
        std::function<void()> eof;
    };

    struct config_t {
        std::string lang;
        model_file_t model_file;
        speech_mode_t speech_mode = speech_mode_t::automatic;
        vad_mode_t vad_mode = vad_mode_t::aggressiveness3;
        bool translate = false; /*extra whisper feature*/
        bool speech_started = false;
    };
    friend std::ostream& operator<<(std::ostream& os, const config_t& config);

    engine_wrapper(config_t config, callbacks_t call_backs);
    virtual ~engine_wrapper();
    std::pair<char*, size_t> borrow_buf();
    void return_buf(const char* c_buf, size_t size, bool sof, bool eof);
    inline void restart() { m_restart_requested = true; }
    inline auto speech_detection_status() const {
        return m_speech_detection_status;
    }
    void set_speech_mode(speech_mode_t mode);
    inline auto speech_mode() const { return m_speech_mode; }
    void set_speech_started(bool value);
    inline auto speech_status() const { return m_speech_started; }
    inline const model_file_t& model_file() const { return m_model_file; }
    inline const std::string& lang() const { return m_lang; }
    inline auto translate() const { return m_translate; }

   protected:
    enum class lock_type_t { free, processed, borrowed };
    friend std::ostream& operator<<(std::ostream& os, lock_type_t lock_type);

    enum class flush_t { regular, eof, exit, restart };
    friend std::ostream& operator<<(std::ostream& os, flush_t flush_type);

    enum class samples_process_result_t { wait_for_samples, no_samples_needed };
    friend std::ostream& operator<<(std::ostream& os,
                                    samples_process_result_t result);

    inline static const size_t m_sample_rate = 16000;  // 1s
    inline static const size_t m_in_buf_max_size = 24000;
    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s
    inline static const unsigned int m_min_text_size = 4;
    inline static const auto m_timeout = 10s;

    struct in_buf_t {
        using buf_t = std::array<int16_t, m_in_buf_max_size>;
        buf_t buf{};
        buf_t::size_type size = 0;
        bool sof = true;
        bool eof = false;
        std::atomic<lock_type_t> lock = lock_type_t::free;
        [[nodiscard]] inline bool full() const { return size == buf.size(); }
        inline void clear() {
            size = 0;
            sof = false;
            eof = false;
        }
    };

    model_file_t m_model_file;
    std::string m_lang;
    callbacks_t m_call_backs;
    std::thread m_processing_thread;
    std::mutex m_processing_mtx;
    std::condition_variable m_processing_cv;
    bool m_thread_exit_requested = false;
    in_buf_t m_in_buf;
    std::optional<std::string> m_intermediate_text;
    vad_wrapper m_vad;
    speech_detection_status_t m_speech_detection_status =
        speech_detection_status_t::no_speech;
    bool m_speech_started = false;
    speech_mode_t m_speech_mode;
    bool m_restart_requested = false;
    std::optional<std::chrono::steady_clock::time_point> m_start_time;
    bool m_translate = false;

    void flush(flush_t type);
    void start_processing();
    void stop_processing();
    virtual samples_process_result_t process_buff();
    bool lock_buf(lock_type_t desired_lock);
    bool lock_buff_for_processing();
    void free_buf(lock_type_t lock);
    void free_buf();
    void set_speech_detection_status(speech_detection_status_t status);
    void set_intermediate_text(const std::string& text);
    static std::string merge_texts(const std::string& old_text,
                                   std::string&& new_text);
    void reset();
    virtual void reset_impl() = 0;
    virtual void stop_processing_impl();
    static void ltrim(std::string& s);
    static void rtrim(std::string& s);
};

#endif  // ENGINE_WRAPPER_H

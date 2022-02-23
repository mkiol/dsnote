/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DEEP_SPEECH_WRAPPER_H
#define DEEP_SPEECH_WRAPPER_H

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include "coqui-stt.h"

class deepspeech_wrapper {
   public:
    enum class speech_mode_type {
        automatic = 0,
        manual = 1,
        single_sentence = 2
    };
    struct callbacks_type {
        std::function<void(const std::string& text)> text_decoded;
        std::function<void(const std::string& text)> intermediate_text_decoded;
        std::function<void(bool speech_detected)> speech_status_changed;
    };

    deepspeech_wrapper(
        const std::string& model_file, const std::string& scorer_file,
        const callbacks_type& call_backs,
        speech_mode_type speech_mode = speech_mode_type::automatic,
        bool speech_started = false);
    ~deepspeech_wrapper();
    std::pair<char*, int64_t> borrow_buff();
    void return_buff(const char* c_buff, int64_t size);
    inline void restart() { restart_requested = true; }
    [[nodiscard]] inline bool speech_detected() const {
        return speech_detected_value;
    }
    void set_speech_mode(speech_mode_type mode);
    [[nodiscard]] inline speech_mode_type speech_mode() const {
        return speech_mode_value;
    }
    void set_speech_started(bool value);
    [[nodiscard]] inline bool speech_status() const {
        return speech_started_value;
    }
    [[nodiscard]] inline const std::string& model_file() const {
        return model_file_value;
    }
    [[nodiscard]] inline const std::string& scorer_file() const {
        return scorer_file_value;
    }

   private:
    static const int frame_size = 16000;         // 1s
    static const unsigned int silent_level = 2;  // number of frames
    static const unsigned int min_text_size = 4;

    typedef std::array<short, 2 * frame_size> buff_type;
    typedef std::shared_ptr<ModelState> model_ptr;

    enum class lock_type { free, processed, borrowed };
    enum class flush_type { regular, exit, restart };

    struct buff_struct_type {
        buff_type buff;
        buff_type::size_type size = 0;
        std::atomic<lock_type> lock = lock_type::free;
        [[nodiscard]] inline bool full() const { return size == buff.size(); }
    };

    std::string model_file_value;
    std::string scorer_file_value;
    callbacks_type call_backs;
    std::thread processing_thread;
    std::mutex processing_mtx;
    std::condition_variable processing_cv;
    bool thread_exit_requested = false;
    buff_struct_type buff_struct;
    std::optional<std::string> intermediate_text;
    unsigned int frames_without_change = 0;
    model_ptr model;
    StreamingState* stream = nullptr;
    bool speech_detected_value = false;
    bool speech_started_value = false;
    bool speech_stop_value = false;
    bool last_frame_done = false;
    speech_mode_type speech_mode_value;
    bool restart_requested = false;

    static std::string error_msg(int status);
    void create_model();
    void create_stream();
    void free_stream();
    int sample_rate() const;
    void flush(flush_type type = flush_type::regular);
    void start_processing();
    bool process_buff();
    void process_buff(buff_type::const_iterator begin,
                      buff_type::const_iterator end);
    void trim_buff(buff_type::const_iterator begin);
    static unsigned int accumulate_abs(buff_type::const_iterator begin,
                                       buff_type::const_iterator end);
    [[nodiscard]] bool detect_silent(buff_type::const_iterator begin,
                                     buff_type::const_iterator end);
    bool lock_buff(lock_type desired_lock);
    bool lock_buff_for_processing();
    void free_buff(lock_type lock);
    void free_buff();
    void set_speech_detected(bool detected);
    void set_intermediate_text(const char* text);
};

#endif  // DEEP_SPEECH_WRAPPER_H

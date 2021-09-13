/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DEEP_SPEECH_WRAPPER_H
#define DEEP_SPEECH_WRAPPER_H

#include <memory>
#include <array>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <utility>
#include <functional>
#include <optional>

#include "deepspeech.h"

class deepspeech_wrapper
{
public:
    enum class speech_mode_type { automatic = 0, manual = 1 };
    struct callbacks_type {
        std::function<void(const std::string& text)> text_decoded;
        std::function<void(const std::string& text)> intermediate_text_decoded;
        std::function<void(bool speech_detected)> speech_status_changed;
    };

    deepspeech_wrapper(const std::string& model_file,
                       const std::string& scorer_file,
                       const callbacks_type& call_backs,
                       speech_mode_type speech_mode = speech_mode_type::automatic);
    ~deepspeech_wrapper();
    std::pair<char*, int64_t> borrow_buff();
    void return_buff(char* c_buff, int64_t size);
    bool ok() const;
    void flush();
    void set_speech_status(bool started);
    [[nodiscard]] inline bool speech_detected() const { return speech_detected_value; }

private:
    static const int frame_size = 16000; // 1s
    static const unsigned int silent_level = 2; // number of frames
    static const unsigned int min_text_size = 4;

    typedef std::array<short, 2 * frame_size> buff_type;
    typedef std::shared_ptr<ModelState> model_ptr;

    enum class lock_type { free, processed, borrowed };

    struct buff_struct_type {
        buff_type buff;
        buff_type::size_type size = 0;
        std::atomic<lock_type> lock = lock_type::free;
        [[nodiscard]] inline bool full() const { return size == buff.size(); }
    };

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
    bool last_frame_done = false;
    speech_mode_type speech_mode_value;

    static std::string error_msg(int status);
    void create_model(const std::string& model_file, const std::string& scorer_file = {});
    void create_stream();
    void free_stream();
    int sample_rate() const;
    void start_processing();
    bool process_buff();
    void process_buff(buff_type::const_iterator begin,
                      buff_type::const_iterator end);
    void trim_buff(buff_type::const_iterator begin);
    unsigned int accumulate_abs(buff_type::const_iterator begin,
                                buff_type::const_iterator end) const;
    [[nodiscard]] bool detect_silent(buff_type::const_iterator begin,
                       buff_type::const_iterator end);
    bool lock_buff(lock_type desired_lock);
    bool lock_buff_for_processing();
    void free_buff(lock_type lock);
    void free_buff();
    void set_speech_detected(bool detected);
    void set_intermediate_text(const char *text);
};

#endif // DEEP_SPEECH_WRAPPER_H

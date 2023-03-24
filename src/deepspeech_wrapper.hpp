/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DEEP_SPEECH_WRAPPER_H
#define DEEP_SPEECH_WRAPPER_H

#include <coqui-stt.h>

#include <memory>
#include <string>
#include <vector>

#include "engine_wrapper.hpp"

class deepspeech_wrapper : public engine_wrapper {
   public:
    deepspeech_wrapper(config_t config, callbacks_t call_backs);
    ~deepspeech_wrapper() override;

   private:
    using ds_model_t = std::shared_ptr<ModelState>;
    using ds_buf_t = std::vector<int16_t>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s

    ds_buf_t m_speech_buf;
    ds_model_t m_ds_model;
    StreamingState* m_ds_stream = nullptr;

    static std::string ds_error_msg(int status);
    void create_ds_model();
    void create_ds_stream();
    void free_ds_stream();
    samples_process_result_t process_buff() override;
    void decode_speech(const ds_buf_t& buf);
    void reset_impl() override;
    bool sentence_timer_timed_out();
};

#endif  // DEEP_SPEECH_WRAPPER_H

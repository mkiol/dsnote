/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef APRIL_ENGINE_H
#define APRIL_ENGINE_H

#include <april-asr/april_api.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "stt_engine.hpp"

struct VoskModel;
struct VoskRecognizer;

class april_engine : public stt_engine {
   public:
    april_engine(config_t config, callbacks_t call_backs);
    ~april_engine() override;

   private:
    using april_buf_t = std::vector<int16_t>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s

    AprilASRModel m_model = nullptr;
    AprilASRSession m_session = nullptr;
    april_buf_t m_speech_buf;
    std::string m_result;
    std::string m_result_prev;
    std::string m_result_prev_segment;

    void create_model();
    samples_process_result_t process_buff() override;
    void decode_speech(april_buf_t& buf, bool eof);
    void reset_impl() override;
    void start_processing_impl() override;
    void push_inbuf_to_samples();

    static void decode_handler(void* user_data, AprilResultType result_type,
                               size_t size, const AprilToken* token);
};

#endif  // APRIL_ENGINE_H

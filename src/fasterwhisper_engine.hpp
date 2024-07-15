/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef FASTERWHISPER_ENGINE_H
#define FASTERWHISPER_ENGINE_H

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "stt_engine.hpp"

namespace py = pybind11;

class fasterwhisper_engine : public stt_engine {
   public:
    fasterwhisper_engine(config_t config, callbacks_t call_backs);
    ~fasterwhisper_engine() override;

   private:
    using whisper_buf_t = std::vector<float>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s
    inline static const int m_threads = 8;

    std::optional<py::object> m_model;
    whisper_buf_t m_speech_buf;
    bool m_auto_lang = false;

    void create_model();
    samples_process_result_t process_buff() override;
    void decode_speech(const whisper_buf_t& buf);
    static void push_buf_to_whisper_buf(
        const std::vector<in_buf_t::buf_t::value_type>& buf,
        whisper_buf_t& whisper_buf);
    static void push_buf_to_whisper_buf(in_buf_t::buf_t::value_type* data,
                                        in_buf_t::buf_t::size_type size,
                                        whisper_buf_t& whisper_buf);

    void reset_impl() override;
    void stop_processing_impl() override;
    void start_processing_impl() override;
    void stop();
};

#endif  // FASTERWHISPER_ENGINE_H

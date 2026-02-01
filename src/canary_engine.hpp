/* Copyright (C) 2024-2025 Cole Leavitt <cole@coleleavitt.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CANARY_ENGINE_H
#define CANARY_ENGINE_H

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

class canary_engine : public stt_engine {
   public:
    canary_engine(config_t config, callbacks_t call_backs);
    ~canary_engine() override;

   private:
    using audio_buf_t = std::vector<float>;

    inline static const size_t m_speech_max_size = m_sample_rate * 30;
    inline static const int m_threads = 8;

    std::optional<py::object> m_model;
    audio_buf_t m_speech_buf;
    bool m_auto_lang = false;

    void create_model();
    samples_process_result_t process_buff() override;
    void decode_speech(const audio_buf_t& buf);
    static void push_buf_to_audio_buf(
        const std::vector<in_buf_t::buf_t::value_type>& buf,
        audio_buf_t& audio_buf);
    static void push_buf_to_audio_buf(in_buf_t::buf_t::value_type* data,
                                      in_buf_t::buf_t::size_type size,
                                      audio_buf_t& audio_buf);

    void reset_impl() override;
    void stop_processing_impl() override;
    void start_processing_impl() override;
    void stop();
};

#endif

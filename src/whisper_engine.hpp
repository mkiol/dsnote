/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef WHISPER_ENGINE_H
#define WHISPER_ENGINE_H

#include <whisper.h>

#include <memory>
#include <string>
#include <vector>

#include "stt_engine.hpp"

class whisper_engine : public stt_engine {
   public:
    whisper_engine(config_t config, callbacks_t call_backs);
    ~whisper_engine() override;

   private:
    using whisper_buf_t = std::vector<float>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s
    inline static const int m_threads = 5;

    struct whisper_api {
        whisper_context* (*whisper_init_from_file)(const char* path_model) =
            nullptr;
        void (*whisper_cancel)(whisper_context* ctx) = nullptr;
        void (*whisper_cancel_clear)(whisper_context* ctx) = nullptr;
        const char* (*whisper_print_system_info)() = nullptr;
        int (*whisper_full)(whisper_context* ctx, whisper_full_params params,
                            const float* samples, int n_samples) = nullptr;
        int (*whisper_full_n_segments)(whisper_context* ctx) = nullptr;
        const char* (*whisper_full_get_segment_text)(whisper_context* ctx,
                                                     int i_segment) = nullptr;
        void (*whisper_free)(whisper_context* ctx) = nullptr;
        whisper_full_params (*whisper_full_default_params)(
            whisper_sampling_strategy strategy) = nullptr;
        inline auto ok() const {
            return whisper_init_from_file && whisper_cancel &&
                   whisper_cancel_clear && whisper_print_system_info &&
                   whisper_full && whisper_full_n_segments &&
                   whisper_full_get_segment_text && whisper_free &&
                   whisper_full_default_params;
        }
    };

    whisper_buf_t m_speech_buf;
    whisper_api m_whisper_api;
    void* m_whisperlib_handle = nullptr;
    whisper_context* m_whisper_ctx = nullptr;
    whisper_full_params m_wparams{};

    void open_whisper_lib();
    void create_whisper_model();
    samples_process_result_t process_buff() override;
    void decode_speech(const whisper_buf_t& buf);
    static void push_buf_to_whisper_buf(
        const std::vector<in_buf_t::buf_t::value_type>& buf,
        whisper_buf_t& whisper_buf);
    whisper_full_params make_wparams();
    void reset_impl() override;
    void stop_processing_impl() override;
    void start_processing_impl() override;
};

#endif  // WHISPER_ENGINE_H

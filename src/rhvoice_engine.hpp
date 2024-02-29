/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef RHVOICE_ENGINE_HPP
#define RHVOICE_ENGINE_HPP

#include <optional>
#include <string>

#include "tts_engine.hpp"

struct RHVoice_tts_engine;
struct RHVoice_init_params;
struct RHVoice_message;
struct RHVoice_synth_params;

typedef enum {
    RHVoice_message_text,
    RHVoice_message_ssml,
    RHVoice_message_characters,
    RHVoice_message_key
} RHVoice_message_type;

class rhvoice_engine : public tts_engine {
   public:
    rhvoice_engine(config_t config, callbacks_t call_backs);
    ~rhvoice_engine() override;
    static bool available();

   private:
    struct rhvoice_api {
        RHVoice_tts_engine* (*RHVoice_new_tts_engine)(
            const RHVoice_init_params* init_params) = nullptr;
        void (*RHVoice_delete_tts_engine)(RHVoice_tts_engine* tts_engine) =
            nullptr;
        RHVoice_message* (*RHVoice_new_message)(
            RHVoice_tts_engine* tts_engine, const char* text,
            unsigned int length, RHVoice_message_type message_type,
            const RHVoice_synth_params* synth_params,
            void* user_data) = nullptr;
        int (*RHVoice_speak)(RHVoice_message* message) = nullptr;
        void (*RHVoice_delete_message)(RHVoice_message* message) = nullptr;

        inline auto ok() const {
            return RHVoice_new_tts_engine && RHVoice_delete_tts_engine &&
                   RHVoice_new_message && RHVoice_speak &&
                   RHVoice_delete_message;
        }
    };

    int m_sample_rate = 0;
    rhvoice_api m_rhvoice_api;
    RHVoice_tts_engine* m_rhvoice_engine = nullptr;
    void* m_lib_handle = nullptr;

    void open_lib();
    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text,
                            const std::string& out_file) final;
    static int play_speech_callback(const short* samples, unsigned int count,
                                    void* user_data);
    static int set_sample_rate_callback(int sample_rate, void* user_data);
};

#endif  // RHVOICE_ENGINE_HPP

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef RHVOICE_ENGINE_HPP
#define RHVOICE_ENGINE_HPP

#include <RHVoice.h>

#include <optional>
#include <string>

#include "tts_engine.hpp"

class rhvoice_engine : public tts_engine {
   public:
    rhvoice_engine(config_t config, callbacks_t call_backs);
    ~rhvoice_engine() override;

   private:
    int m_sample_rate = 0;
    RHVoice_tts_engine m_rhvoice_engine = nullptr;

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

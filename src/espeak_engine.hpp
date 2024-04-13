/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ESPEAK_ENGINE_HPP
#define ESPEAK_ENGINE_HPP

#include <espeak-ng/speak_lib.h>

#include <string>

#include "tts_engine.hpp"

class espeak_engine : public tts_engine {
   public:
    espeak_engine(config_t config, callbacks_t call_backs);
    ~espeak_engine() override;

   private:
    bool m_ok = false;
    int m_sample_rate = 0;

    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text, unsigned int speed,
                            const std::string& out_file) final;
    static int synth_callback(short* wav, int size, espeak_EVENT* event);
};

#endif  // ESPEAK_ENGINE_HPP

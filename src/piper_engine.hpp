/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PIPER_ENGINE_HPP
#define PIPER_ENGINE_HPP

#include <piper_api.h>

#include <optional>
#include <string>

#include "tts_engine.hpp"

class piper_engine : public tts_engine {
   public:
    piper_engine(config_t config, callbacks_t call_backs);
    ~piper_engine() override;

   private:
    std::optional<piper_api> m_piper;
    float m_initial_length_scale = 1.0F;

    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text,
                            const std::string& out_file) final;
};

#endif  // PIPER_ENGINE_HPP

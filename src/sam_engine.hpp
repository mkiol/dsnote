/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SAM_ENGINE_HPP
#define SAM_ENGINE_HPP

#include <sam_api.h>

#include <string>

#include "tts_engine.hpp"

class sam_engine : public tts_engine {
   public:
    sam_engine(config_t config, callbacks_t call_backs);
    ~sam_engine() override;

   private:
    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text, unsigned int speed,
                            const std::string& out_file) final;
};

#endif  // SAM_ENGINE_HPP

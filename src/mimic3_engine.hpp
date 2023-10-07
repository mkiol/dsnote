/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MIMIC3_ENGINE_HPP
#define MIMIC3_ENGINE_HPP

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <string>

#include "tts_engine.hpp"

namespace py = pybind11;

class mimic3_engine : public tts_engine {
   public:
    mimic3_engine(config_t config, callbacks_t call_backs);
    ~mimic3_engine() override;

   private:
    std::optional<py::object> m_tts;
    float m_initial_length_scale = 1.0f;

    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text,
                            const std::string& out_file) final;
    void stop();
};

#endif  // MIMIC3_ENGINE_HPP

/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef KOKORO_ENGINE_HPP
#define KOKORO_ENGINE_HPP

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <string>

#include "tts_engine.hpp"

namespace py = pybind11;

class kokoro_engine : public tts_engine {
   public:
    kokoro_engine(config_t config, callbacks_t call_backs);
    ~kokoro_engine() override;

   private:
    static const int s_sample_rate = 24000;
    std::optional<py::object> m_model;
    std::optional<py::object> m_pipeline;
    std::string m_voice_file;
    std::string m_device_str;

    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text, unsigned int speed,
                            const std::string& out_file) final;
    void stop();
};

#endif  // KOKORO_ENGINE_HPP

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef COQUI_ENGINE_HPP
#define COQUI_ENGINE_HPP

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <string>

#include "tts_engine.hpp"

namespace py = pybind11;

class coqui_engine : public tts_engine {
   public:
    coqui_engine(config_t config, callbacks_t call_backs);
    ~coqui_engine() override;

   private:
    inline static const auto* const config_temp_file =
        "/tmp/tmp_coqui_config.json";
    inline static const auto* const vocoder_config_temp_file =
        "/tmp/tmp_coqui_vocoder_config.json";

    std::optional<py::object> m_tts;
    std::optional<float> m_initial_length_scale;
    std::optional<float> m_initial_duration_threshold;

    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text,
                            const std::string& out_file) final;
    void stop();
    static std::string fix_config_file(const std::string& config_file,
                                       const std::string& dir, bool vocoder);
};

#endif  // COQUI_ENGINE_HPP

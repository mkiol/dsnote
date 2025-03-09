/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PARLER_ENGINE_HPP
#define PARLER_ENGINE_HPP

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <string>

#include "tts_engine.hpp"

namespace py = pybind11;

class parler_engine : public tts_engine {
   public:
    parler_engine(config_t config, callbacks_t call_backs);
    ~parler_engine() override;

   private:
    std::optional<py::object> m_model;
    std::optional<py::object> m_tokenizer;
    std::optional<py::object> m_desc_input_ids;
    std::optional<py::object> m_desc_attention_mask;

    std::string m_device_str;

    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    void reset_ref_voice() final;
    bool encode_speech_impl(const std::string& text, unsigned int speed,
                            const std::string& out_file) final;
    void stop();
    void create_desc();
};

#endif  // PARLER_ENGINE_HPP

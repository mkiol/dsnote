/* Copyright (C) 2026 James David Clarke <james@jamesdavidclarke.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef QWEN3TTS_ENGINE_HPP
#define QWEN3TTS_ENGINE_HPP

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <string>
#include <vector>

#include "tts_engine.hpp"

namespace py = pybind11;

class qwen3tts_engine : public tts_engine {
   public:
    qwen3tts_engine(config_t config, callbacks_t call_backs);
    ~qwen3tts_engine() override;

   private:
    std::optional<py::object> m_model;
    std::optional<py::object> m_clone_prompt;
    std::vector<py::object> m_hook_handles;
    std::string m_device_str;
    std::string m_model_type;

    bool model_created() const final;
    bool model_supports_speed() const final;
    void create_model() final;
    bool encode_speech_impl(const std::string& text, unsigned int speed,
                            const std::string& out_file) final;
    void reset_ref_voice() final;
    void stop();
    std::string prepare_model_dir() const;
};

#endif  // QWEN3TTS_ENGINE_HPP

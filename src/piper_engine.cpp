/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "piper_engine.hpp"

#include <fmt/format.h>

#include "logger.hpp"

piper_engine::piper_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {}

piper_engine::~piper_engine() {
    LOGD("piper dtor");

    stop();
}

bool piper_engine::model_created() const { return static_cast<bool>(m_piper); }

void piper_engine::create_model() {
    auto model_file =
        first_file_with_ext(m_config.model_files.model_path, "onnx");
    auto config_file =
        first_file_with_ext(m_config.model_files.model_path, "json");

    if (model_file.empty() || config_file.empty()) {
        LOGE("failed to find model or config files");
        return;
    }

    try {
        int64_t speaker_id = -1;
        try {
            speaker_id = std::stoll(m_config.speaker_id);
        } catch ([[maybe_unused]] const std::invalid_argument& err) {
        }

        m_piper.emplace(std::move(model_file), std::move(config_file),
                        m_config.data_dir, speaker_id);

        m_initial_length_scale = m_piper->length_scale();
        LOGD("initial length scale: " << m_initial_length_scale);
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
    }

    if (!m_piper)
        LOGE("failed to create piper model");
    else
        LOGD("piper model created");
}

bool piper_engine::model_supports_speed() const { return true; }

bool piper_engine::encode_speech_impl(const std::string& text,
                                      unsigned int speed,
                                      const std::string& out_file) {
    auto length_scale = vits_length_scale(speed, m_initial_length_scale);

    LOGD("length_scale: " << length_scale);

    try {
        m_piper->text_to_wav_file(text, out_file, length_scale);
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
        return false;
    }

    LOGD("voice synthesized successfully");

    return true;
}

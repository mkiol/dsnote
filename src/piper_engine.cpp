/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "piper_engine.hpp"

#include <fmt/format.h>

#include "config.h"
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
#ifdef USE_SFOS
        m_piper.emplace(
            std::move(model_file), std::move(config_file),
            fmt::format("/usr/share/{}/espeak-ng-data", APP_BINARY_ID));
#else
        m_piper.emplace(std::move(model_file), std::move(config_file));
#endif
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
    }
}

bool piper_engine::encode_speech_impl(const std::string& text,
                                      const std::string& out_file) {
    try {
        m_piper->text_to_wav_file(text, out_file);
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
        return false;
    }

    LOGD("voice synthesized successfully");

    return true;
}

/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sam_engine.hpp"

#include <fmt/format.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>

#include "logger.hpp"

sam_engine::sam_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {}

sam_engine::~sam_engine() {
    LOGD("sam dtor");

    stop();
}

bool sam_engine::model_created() const { return true; }

void sam_engine::create_model() {}

bool sam_engine::model_supports_speed() const { return true; }

bool sam_engine::encode_speech_impl(const std::string& text, unsigned int speed,
                                    const std::string& out_file) {
    auto rate = [speed]() {
        const int default_speed = 72;

        if (speed < 1 || speed > 20 || speed == 10) {
            return default_speed;
        }

        return std::clamp<int>(static_cast<float>(default_speed) *
                                   (10.0f / static_cast<float>(speed)),
                               10, 100);
    }();

    LOGD("requested speed: " << speed << " " << rate);

    if (is_shutdown()) {
        unlink(out_file.c_str());
        return false;
    }

    const char* u8_buff = nullptr;
    int u8_buff_size = 0;

    if (sam_text_to_u8_buff(text.c_str(), rate, &u8_buff, &u8_buff_size) !=
        SAM_SUCCESS) {
        unlink(out_file.c_str());
        return false;
    }

    std::ofstream of{out_file, std::ios::binary};
    if (of.bad()) {
        LOGE("failed to open file for writting: " << out_file);
        return false;
    }

    write_wav_header(22050, sizeof(short), 1, u8_buff_size, of);

    // covert u8 samples to s16
    std::vector<short> s16_buff;
    s16_buff.reserve(u8_buff_size);
    for (int i = 0; i < u8_buff_size; ++i) {
        s16_buff.push_back(static_cast<short>(u8_buff[i] - 128) << 8);
    }
    of.write(reinterpret_cast<char*>(s16_buff.data()),
             s16_buff.size() * sizeof(short));

    LOGD("voice synthesized successfully");

    return true;
}

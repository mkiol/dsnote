/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "espeak_engine.hpp"

#include <fmt/format.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>

#include "logger.hpp"

espeak_engine::espeak_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {}

espeak_engine::~espeak_engine() {
    LOGD("espeak dtor");

    stop();

    espeak_Terminate();
}

bool espeak_engine::model_created() const { return m_ok; }

void espeak_engine::create_model() {
    if (m_config.speaker.empty()) {
        LOGE("voice name missing");
        return;
    }

    auto mb_voice = m_config.speaker.size() > 3 && m_config.speaker[0] == 'm' &&
                    m_config.speaker[1] == 'b' && m_config.speaker[2] == '-';

    if (mb_voice && !m_config.model_files.model_path.empty()) {
        mkdir(fmt::format("{}/mbrola", m_config.data_dir).c_str(), 0777);

        auto link_target = fmt::format("{}/mbrola/{}", m_config.data_dir,
                                       &m_config.speaker[3]);
        remove(link_target.c_str());

        (void)symlink(m_config.model_files.model_path.c_str(),
                      link_target.c_str());
    }

    m_sample_rate = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0,
                                      m_config.data_dir.c_str(), 0);

    if (m_sample_rate == EE_INTERNAL_ERROR) {
        LOGE("failed to init espeak");
        return;
    }

    if (mb_voice) m_sample_rate = 16000;

    m_ok = espeak_SetVoiceByName(m_config.speaker.c_str()) == EE_OK;

    if (!m_ok) LOGE("failed to set espeak voice");
}

struct callback_data {
    espeak_engine* engine = nullptr;
    std::ofstream wav_file;
};

int espeak_engine::synth_callback(short* wav, int size, espeak_EVENT* event) {
    auto cb_data = static_cast<callback_data*>(event->user_data);

    if (wav == nullptr) {
        LOGD("end of espeak synth");
        return 0;
    }

    if (cb_data->engine->m_shutting_down) {
        LOGD("end of espeak synth due to shutdown");
        return 1;
    }

    cb_data->wav_file.write(reinterpret_cast<char*>(wav), size * sizeof(short));

    return 0;
}

bool espeak_engine::encode_speech_impl(const std::string& text,
                                       const std::string& out_file) {
    espeak_SetSynthCallback(&synth_callback);

    callback_data cb_data{this, std::ofstream{out_file, std::ios::binary}};

    if (cb_data.wav_file.bad()) {
        LOGE("failed to open file for writting: " << out_file);
        return false;
    }

    cb_data.wav_file.seekp(sizeof(wav_header));

    if (espeak_Synth(text.c_str(), text.size(), 0, POS_CHARACTER,
                     espeakCHARS_AUTO, 0, nullptr, &cb_data) != EE_OK) {
        LOGE("error in espeak synth");
        cb_data.wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    if (espeak_Synchronize() != EE_OK) {
        LOGE("error in espeak synchronize");
        cb_data.wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    if (m_shutting_down) {
        cb_data.wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    auto data_size = cb_data.wav_file.tellp();

    if (data_size == sizeof(wav_header)) {
        LOGE("no audio data");
        cb_data.wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    cb_data.wav_file.seekp(0);

    write_wav_header(m_sample_rate, sizeof(short), 1, data_size / sizeof(short),
                     cb_data.wav_file);

    LOGD("voice synthesized successfully");

    return true;
}

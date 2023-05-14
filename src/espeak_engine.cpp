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

#include <fstream>

#include "logger.hpp"

struct wav_header {
    uint8_t RIFF[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunk_size = 0;
    uint8_t WAVE[4] = {'W', 'A', 'V', 'E'};
    uint8_t fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;
    uint16_t num_channels = 0;
    uint32_t sample_rate = 0;
    uint32_t bytes_per_sec = 0;
    uint16_t block_align = 2;
    uint16_t bits_per_sample = 16;
    uint8_t data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size = 0;
};

// borrowed from:
// https://github.com/rhasspy/piper/blob/master/src/cpp/wavfile.hpp
static void write_wav_header(int sample_rate, int sample_width, int channels,
                             uint32_t num_samples, std::ostream& wav_file) {
    wav_header header;
    header.data_size = num_samples * sample_width * channels;
    header.chunk_size = header.data_size + sizeof(wav_header) - 8;
    header.sample_rate = sample_rate;
    header.num_channels = channels;
    header.bytes_per_sec = sample_rate * sample_width * channels;
    header.block_align = sample_width * channels;
    wav_file.write(reinterpret_cast<const char*>(&header), sizeof(wav_header));
}

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
        mkdir(fmt::format("{}/mbrola", m_config.espeak_data_dir).c_str(), 0777);

        symlink(m_config.model_files.model_path.c_str(),
                fmt::format("{}/mbrola/{}", m_config.espeak_data_dir,
                            &m_config.speaker[3])
                    .c_str());
    }

    m_sample_rate = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0,
                                      m_config.espeak_data_dir.c_str(), 0);

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

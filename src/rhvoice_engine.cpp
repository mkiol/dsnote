/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "rhvoice_engine.hpp"

#include <fmt/format.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>

#include "logger.hpp"

rhvoice_engine::rhvoice_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {}

rhvoice_engine::~rhvoice_engine() {
    LOGD("rhvoice dtor");

    stop();

    if (m_rhvoice_engine) {
        RHVoice_delete_tts_engine(m_rhvoice_engine);
        m_rhvoice_engine = nullptr;
    }
}

bool rhvoice_engine::model_created() const {
    return m_rhvoice_engine != nullptr;
}

void rhvoice_engine::create_model() {
    if (m_config.speaker.empty()) {
        LOGE("voice name missing");
        return;
    }

    mkdir(fmt::format("{}/voices", m_config.data_dir).c_str(), 0777);

    auto link_target =
        fmt::format("{}/voices/{}", m_config.data_dir, m_config.speaker);

    remove(link_target.c_str());

    if (symlink(m_config.model_files.model_path.c_str(), link_target.c_str()) !=
        0) {
        LOGE("symlink error: " << m_config.model_files.model_path << " => "
                               << link_target);
    }

    RHVoice_callbacks callbacks{/*set_sample_rate=*/&set_sample_rate_callback,
                                /*play_speech=*/&play_speech_callback,
                                /*process_mark=*/nullptr,
                                /*word_starts=*/nullptr,
                                /*word_ends=*/nullptr,
                                /*sentence_starts=*/nullptr,
                                /*sentence_ends=*/nullptr,
                                /*play_audio=*/nullptr,
                                /*done=*/nullptr};

    RHVoice_init_params init_params{/*data_path=*/m_config.data_dir.c_str(),
                                    /*config_path=*/m_config.config_dir.c_str(),
                                    /*resource_paths=*/nullptr,
                                    /*callbacks=*/callbacks,
                                    /*options=*/0};

    m_rhvoice_engine = RHVoice_new_tts_engine(&init_params);

    if (!m_rhvoice_engine) {
        LOGE("failed to init rhvoice engine");
    }
}

struct callback_data {
    rhvoice_engine* engine = nullptr;
    std::ofstream wav_file;
};

int rhvoice_engine::set_sample_rate_callback(int sample_rate, void* user_data) {
    auto* cb_data = static_cast<callback_data*>(user_data);

    cb_data->engine->m_sample_rate = sample_rate;

    return 1;
}

int rhvoice_engine::play_speech_callback(const short* samples,
                                         unsigned int count, void* user_data) {
    auto* cb_data = static_cast<callback_data*>(user_data);

    if (samples == nullptr) {
        LOGD("no rhvoice samples");
        return 0;
    }

    if (cb_data->engine->m_shutting_down) {
        LOGD("end of rhvoice play speech due to shutdown");
        return 0;
    }

    cb_data->wav_file.write(reinterpret_cast<const char*>(samples),
                            count * sizeof(short));

    return 1;
}

bool rhvoice_engine::encode_speech_impl(const std::string& text,
                                        const std::string& out_file) {
    callback_data cb_data{this, std::ofstream{out_file, std::ios::binary}};

    if (cb_data.wav_file.bad()) {
        LOGE("failed to open file for writting: " << out_file);
        return false;
    }

    cb_data.wav_file.seekp(sizeof(wav_header));

    RHVoice_synth_params synth_params{
        /*voice_profile=*/m_config.speaker.c_str(),
        /*absolute_rate=*/0.0,
        /*absolute_pitch=*/0.0,
        /*absolute_volume=*/0.0,
        /*relative_rate=*/1.0,
        /*relative_pitch=*/1.0,
        /*relative_volume=*/1.0,
        /*punctuation_mode=*/RHVoice_punctuation_default,
        /*punctuation_list=*/"",
        /*capitals_mode=*/RHVoice_capitals_default,
        /*flags=*/0};

    auto message =
        RHVoice_new_message(m_rhvoice_engine, text.c_str(), text.size(),
                            RHVoice_message_text, &synth_params, &cb_data);
    if (!message) {
        LOGE("failed to create rhvoice message");
        cb_data.wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    if (RHVoice_speak(message) == 0) {
        LOGE("rhvoice speek failed");
        RHVoice_delete_message(message);
        cb_data.wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    RHVoice_delete_message(message);

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

    LOGD("sample rate: " << m_sample_rate);

    write_wav_header(m_sample_rate, sizeof(short), 1, data_size / sizeof(short),
                     cb_data.wav_file);

    LOGD("voice synthesized successfully");

    return true;
}

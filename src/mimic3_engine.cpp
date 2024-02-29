/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mimic3_engine.hpp"

#include <fmt/format.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string_view>
#include <utility>

#include "logger.hpp"
#include "py_executor.hpp"

using namespace pybind11::literals;

mimic3_engine::mimic3_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {}

mimic3_engine::~mimic3_engine() {
    LOGD("mimic3 dtor");

    stop();
}

void mimic3_engine::stop() {
    tts_engine::stop();

    if (m_tts) {
        auto task = py_executor::instance()->execute([&]() {
            try {
                m_tts->attr("_loaded_voices")["lang/model"]
                    .attr("_SHARED_MODELS")
                    .attr("clear")();
                m_tts.reset();
            } catch (const std::exception& err) {
                LOGE("py error: " << err.what());
            }
            return std::any{};
        });

        if (task) task->get();
    }

    LOGD("mimic3 stopped");
}

void mimic3_engine::create_model() {
    const auto& voices_dir = m_config.data_dir;
    auto lang_dir = fmt::format("{}/lang", voices_dir);

    mkdir(voices_dir.c_str(), 0777);
    mkdir(lang_dir.c_str(), 0777);

    auto link_target =
        fmt::format("{}/model", lang_dir, m_config.model_files.model_path);

    remove(link_target.c_str());

    if (symlink(m_config.model_files.model_path.c_str(), link_target.c_str()) !=
        0) {
        LOGE("symlink error: " << m_config.model_files.model_path << " => "
                               << link_target);
    }

    auto task = py_executor::instance()->execute([&]() {
        try {
            auto api = py::module_::import("mimic3_tts");

            auto dirs = py::list();
            dirs.attr("append")(voices_dir);

            m_tts =
                api.attr("Mimic3TextToSpeechSystem")(api.attr("Mimic3Settings")(
                    "voices_directories"_a = dirs, "no_download"_a = true));
            m_tts->attr("voice") = "lang/model";

            if (!m_config.speaker_id.empty())
                m_tts->attr("speaker") = m_config.speaker_id;

            m_tts->attr("preload_voice")("lang/model");

            if (py::len(m_tts->attr("_loaded_voices")) == 0 ||
                !m_tts->attr("_loaded_voices").contains("lang/model")) {
                LOGE("voice not loaded");
                m_tts.reset();
                return false;
            }

            m_initial_length_scale = m_tts->attr("_loaded_voices")["lang/model"]
                                         .attr("config")
                                         .attr("inference")
                                         .attr("length_scale")
                                         .cast<float>();

            LOGD("initial length scale: " << m_initial_length_scale);
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            m_tts.reset();
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get()))
        LOGE("failed to create mimic3 model");
    else
        LOGD("mimic3 model created");
}

bool mimic3_engine::model_created() const { return static_cast<bool>(m_tts); }

bool mimic3_engine::encode_speech_impl(const std::string& text,
                                      const std::string& out_file) {
    auto length_scale =
        vits_length_scale(m_config.speech_speed, m_initial_length_scale);

    LOGD("length_scale: " << length_scale);

    std::ofstream wav_file{out_file, std::ios::binary};

    if (wav_file.bad()) {
        LOGE("failed to open file for writting: " << out_file);
        return false;
    }

    wav_file.seekp(sizeof(wav_header));

    int sample_rate = 0;

    auto task = py_executor::instance()->execute([&]() {
        try {
            m_tts->attr("settings").attr("length_scale") = length_scale;

            m_tts->attr("begin_utterance")();
            m_tts->attr("speak_text")(text);
            auto results = m_tts->attr("end_utterance")();

            for (auto& result : results) {
                sample_rate = result.attr("sample_rate_hz").cast<int>();

                auto bytes = result.attr("audio_bytes");

                std::vector<unsigned char> data;
                data.reserve(py::len(bytes));

                for (const auto& byte : bytes)
                    data.push_back(byte.cast<unsigned char>());

                wav_file.write(reinterpret_cast<char*>(data.data()),
                               data.size());
            }
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }

        return true;
    });

    if (!task || !std::any_cast<bool>(task->get())) {
        wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    auto data_size = wav_file.tellp();

    if (data_size == sizeof(wav_header)) {
        LOGE("no audio data");
        wav_file.close();
        unlink(out_file.c_str());
        return false;
    }

    wav_file.seekp(0);

    LOGD("sample rate: " << sample_rate);

    write_wav_header(sample_rate, sizeof(short), 1, data_size / sizeof(short),
                     wav_file);

    LOGD("voice synthesized successfully");

    return true;
}

bool mimic3_engine::model_supports_speed() const {
    return static_cast<bool>(m_initial_length_scale);
}

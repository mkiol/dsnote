/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "coqui_engine.hpp"

#include <fmt/format.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string_view>
#include <utility>

#include "logger.hpp"
#include "py_executor.hpp"
#include "simdjson.h"

using namespace pybind11::literals;

coqui_engine::coqui_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {}

coqui_engine::~coqui_engine() {
    LOGD("coqui dtor");

    stop();
}

void coqui_engine::stop() {
    tts_engine::stop();

    auto* pe = py_executor::instance();

    try {
        pe->execute([&]() {
              try {
                  m_tts.reset();
              } catch (const std::exception& err) {
                  LOGE("py error: " << err.what());
              }
              return std::string{};
          }).get();
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
    }
}

static std::string replace_all(std::string str, const std::string& from,
                               const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.size(), to);
        start_pos += to.size();
    }
    return str;
}

std::string coqui_engine::fix_config_file(const std::string& config_file,
                                          const std::string& dir) {
    std::string path_to_replace = [&] {
        try {
            auto json = simdjson::padded_string::load(config_file);
            if (json.error() != simdjson::SUCCESS) {
                LOGE("failed to simdjson load");
                return std::string{};
            }

            simdjson::ondemand::parser parser;

            auto doc = parser.iterate(json);
            if (doc.error() != simdjson::SUCCESS) {
                LOGE("failed to simdjson iterate");
                return std::string{};
            }

            std::string_view sv;

            if (!doc["speakers_file"].get(sv)) return std::string{sv};

            if (!doc["model_args"]["speakers_file"].get(sv))
                return std::string{sv};

            if (!doc["audio"]["stats_path"].get(sv)) return std::string{sv};
        } catch (const simdjson::simdjson_error& err) {
            LOGD("error: " << err.what());
        }
        return std::string{};
    }();

    if (path_to_replace.empty()) return config_file;

    std::ifstream ifs{config_file, std::ios::in};
    auto config_data = std::string{std::istreambuf_iterator<char>(ifs),
                                   std::istreambuf_iterator<char>()};

    auto speaker_file_in_config =
        path_to_replace.substr(path_to_replace.find_last_of('/'));

    // rename speaker file in config if needed
    if (const auto* prefix{"speaker"};
        speaker_file_in_config.size() > strlen(prefix) + 1 &&
        speaker_file_in_config.substr(1, strlen(prefix)) == prefix) {
        speaker_file_in_config = speaker_file_in_config.substr(1);

        if (find_file_with_name_prefix(dir, speaker_file_in_config).empty()) {
            LOGD("speaker file in config missing");

            auto speaker_file = find_file_with_name_prefix(dir, prefix);
            speaker_file =
                speaker_file.substr(speaker_file.find_last_of('/') + 1);

            if (speaker_file.empty()) {
                LOGD("speaker file missing");
            } else {
                LOGD("speaker file replace: " << speaker_file_in_config
                                              << " => " << speaker_file);
                config_data = replace_all(config_data, speaker_file_in_config,
                                          speaker_file);
            }
        }
    }

    path_to_replace =
        path_to_replace.substr(0, path_to_replace.find_last_of('/'));

    LOGD("path replace: " << path_to_replace << " => " << dir);

    config_data = replace_all(config_data, path_to_replace, dir);

    std::ofstream out_file{config_temp_file, std::ios::out};
    out_file << config_data;

    return config_temp_file;
}

void coqui_engine::create_model() {
    auto* pe = py_executor::instance();

    try {
        pe->execute([&]() {
              auto model_file = find_file_with_name_prefix(
                  m_config.model_files.model_path, "model_file");
              if (model_file.empty())
                  model_file = first_file_with_ext(
                      m_config.model_files.model_path, "pth");
              if (model_file.empty())
                  model_file = first_file_with_ext(
                      m_config.model_files.model_path, "tar");

              auto config_file = find_file_with_name_prefix(
                  m_config.model_files.model_path, "config.json");

              if (model_file.empty() || config_file.empty()) {
                  LOGE("failed to find model or config files");
                  return std::string{};
              }

              LOGD("model files: " << model_file << " " << config_file);

              config_file =
                  fix_config_file(config_file, m_config.model_files.model_path);

              auto vocoder_model_file =
                  first_file_with_ext(m_config.model_files.vocoder_path, "pth");
              auto vocoder_config_file = first_file_with_ext(
                  m_config.model_files.vocoder_path, "json");

              try {
                  auto api = py::module_::import("TTS.api");

                  m_tts = [&] {
                      if (vocoder_model_file.empty() ||
                          vocoder_config_file.empty())
                          return api.attr("TTS")("model_path"_a = model_file,
                                                 "config_path"_a = config_file);
                      return api.attr("TTS")(
                          "model_path"_a = model_file,
                          "config_path"_a = config_file,
                          "vocoder_path"_a = vocoder_model_file,
                          "vocoder_config_path"_a = vocoder_config_file);
                  }();

                  if (m_tts) {
                      auto model = m_tts->attr("synthesizer").attr("tts_model");
                      if (py::hasattr(model, "length_scale")) {
                          m_initial_length_scale =
                              model.attr("length_scale").cast<float>();
                          LOGD("initial length scale: "
                               << *m_initial_length_scale);
                      }
                  }
              } catch (const std::exception& err) {
                  LOGE("py error: " << err.what());
              }
              return std::string{};
          }).get();
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
    }
}

bool coqui_engine::model_created() const { return static_cast<bool>(m_tts); }

bool coqui_engine::encode_speech_impl(const std::string& text,
                                      const std::string& out_file) {
    auto* pe = py_executor::instance();

    auto length_scale = [this]() {
        if (m_initial_length_scale) {
            switch (m_config.speech_speed) {
                case speech_speed_t::very_slow:
                    return m_initial_length_scale.value() * 1.8f;
                case speech_speed_t::slow:
                    return m_initial_length_scale.value() * 1.4f;
                case speech_speed_t::fast:
                    return m_initial_length_scale.value() * 0.8f;
                case speech_speed_t::very_fast:
                    return m_initial_length_scale.value() * 0.6f;
                case speech_speed_t::normal:
                    break;
            }
            return m_initial_length_scale.value();
        }
        return 1.0f;
    }();

    try {
        return pe->execute([&]() {
                     try {
                         auto model =
                             m_tts->attr("synthesizer").attr("tts_model");
                         if (py::hasattr(model, "length_scale")) {
                             model.attr("length_scale") = length_scale;
                         }

                         if (m_config.speaker.empty()) {
                             m_tts->attr("tts_to_file")(
                                 "text"_a = uroman(text),
                                 "file_path"_a = out_file);
                         } else {
                             m_tts->attr("tts_to_file")(
                                 uroman(text), "file_path"_a = out_file,
                                 "speaker"_a = m_config.speaker,
                                 "length_scale"_a = length_scale);
                         }
                     } catch (const std::exception& err) {
                         LOGE("py error: " << err.what());
                         return std::string{"false"};
                     }

                     LOGD("voice synthesized successfully");
                     return std::string{"true"};
                 }).get() == "true";
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
        return false;
    }
}

// source: https://stackoverflow.com/a/64359731/7767358
static std::pair<FILE*, FILE*> popen2(const char* __command) {
    int pipes[2][2];

    pipe(pipes[0]);
    pipe(pipes[1]);

    if (fork() > 0) {
        // parent
        close(pipes[0][0]);
        close(pipes[1][1]);

        return {fdopen(pipes[0][1], "w"), fdopen(pipes[1][0], "r")};
    } else {
        // child
        close(pipes[0][1]);
        close(pipes[1][0]);

        dup2(pipes[0][0], STDIN_FILENO);
        dup2(pipes[1][1], STDOUT_FILENO);

        execl("/bin/sh", "/bin/sh", "-c", __command, NULL);

        exit(1);
    }
}

bool coqui_engine::model_supports_speed() const {
    return static_cast<bool>(m_initial_length_scale);
}

std::string coqui_engine::uroman(const std::string& text) const {
    if (m_config.uromanpl_path.empty() || m_config.lang_code.empty() ||
        (m_config.lang_code != "amh" && m_config.lang_code != "kor" &&
         m_config.lang_code != "hak" && m_config.lang_code != "nan")) {
        // uroman not needed
        return text;
    }

    auto [p_stdin, p_stdout] = popen2(
        fmt::format("perl {} -l {}", m_config.uromanpl_path, m_config.lang_code)
            .c_str());

    std::string result;

    if (p_stdin == nullptr || p_stdout == nullptr) {
        LOGE("popen error");
        return result;
    }

    fwrite(text.c_str(), 1, text.size(), p_stdin);
    fclose(p_stdin);

    char buf[1024];
    while (fgets(buf, 1024, p_stdout)) result.append(buf);

    return result;
}

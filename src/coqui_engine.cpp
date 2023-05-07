/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "coqui_engine.hpp"

#include <unistd.h>

#include <fstream>
#include <string_view>

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
        } catch (const simdjson::simdjson_error& err) {
            LOGD("error: " << err.what());
        }
        return std::string{};
    }();

    if (path_to_replace.empty()) return config_file;

    path_to_replace =
        path_to_replace.substr(0, path_to_replace.find_last_of('/'));

    LOGD("path to replace: " << path_to_replace);

    std::ifstream ifs{config_file, std::ios::in};

    auto config_data = replace_all(
        {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()},
        path_to_replace, dir);

    std::ofstream out_file{config_temp_file, std::ios::out};
    out_file << config_data;

    return config_temp_file;
}

void coqui_engine::create_model() {
    auto* pe = py_executor::instance();

    try {
        pe->execute([&]() {
              auto model_file =
                  first_file_with_ext(m_config.model_files.model_path, "pth");
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

    try {
        return pe->execute([&]() {
                     try {
                         if (m_config.speaker.empty())
                             m_tts->attr("tts_to_file")(
                                 "text"_a = text, "file_path"_a = out_file);
                         else
                             m_tts->attr("tts_to_file")(
                                 "text"_a = text, "file_path"_a = out_file,
                                 "speaker"_a = m_config.speaker);
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

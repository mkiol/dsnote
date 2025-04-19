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

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"
#include "simdjson.h"

using namespace pybind11::literals;

coqui_engine::coqui_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {
    if ((cpu_tools::cpuinfo().feature_flags &
         cpu_tools::feature_flags_t::avx) == 0) {
        LOGE("avx not supported but coqui engine needs it");
        throw std::runtime_error(
            "failed to init coqui engine: avx not supported");
    }
}

coqui_engine::~coqui_engine() {
    LOGD("coqui dtor");

    stop();
}

void coqui_engine::stop() {
    tts_engine::stop();

    auto task = py_executor::instance()->execute([&]() {
        try {
            m_model.reset();

            // release mem
            py::module_::import("gc").attr("collect")();
            py::module_::import("torch").attr("cuda").attr("empty_cache")();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();

    LOGD("coqui stopped");
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
                                          const std::string& dir,
                                          bool vocoder) {
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
            if (!doc["model_args"]["speaker_encoder_config_path"].get(sv))
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

    std::ofstream out_file{
        vocoder ? vocoder_config_temp_file : config_temp_file, std::ios::out};
    out_file << config_data;

    return vocoder ? vocoder_config_temp_file : config_temp_file;
}

bool coqui_engine::register_cancel_hook(const char* field, py::object obj) {
    // run only under py thread
    auto hook =
        py::cpp_function{[this]([[maybe_unused]] const py::args& args,
                                [[maybe_unused]] const py::kwargs& kwargs) {
            if (is_shutdown()) {
                throw std::runtime_error{"engine shutdown"};
            }
        }};

    if (field == nullptr) {
        if (py::hasattr(obj, "register_forward_hook"))
            obj.attr("register_forward_hook")(hook);
        if (py::hasattr(obj, "register_forward_pre_hook"))
            obj.attr(field).attr("register_forward_pre_hook")(hook);
        return true;
    }

    if (py::hasattr(obj, field)) {
        if (py::hasattr(obj.attr(field), "register_forward_hook"))
            obj.attr(field).attr("register_forward_hook")(hook);
        if (py::hasattr(obj.attr(field), "register_forward_pre_hook"))
            obj.attr(field).attr("register_forward_pre_hook")(hook);
        return true;
    }

    return false;
}

void coqui_engine::create_model() {
    auto task = py_executor::instance()->execute([&]() {
        auto model_file = find_file_with_name_prefix(
            m_config.model_files.model_path, "model_file");
        if (model_file.empty())
            model_file =
                first_file_with_ext(m_config.model_files.model_path, "pth");
        if (model_file.empty())
            model_file =
                first_file_with_ext(m_config.model_files.model_path, "tar");

        auto config_file = find_file_with_name_prefix(
            m_config.model_files.model_path, "config.json");

        if (model_file.empty() || config_file.empty()) {
            LOGE("failed to find model or config files");
            return false;
        }

        LOGD("model files: " << model_file << " " << config_file);

        config_file = fix_config_file(config_file,
                                      m_config.model_files.model_path, false);

        auto vocoder_model_file =
            first_file_with_ext(m_config.model_files.vocoder_path, "pth");
        auto vocoder_config_file =
            first_file_with_ext(m_config.model_files.vocoder_path, "json");

        if (!vocoder_config_file.empty())
            vocoder_config_file = fix_config_file(
                vocoder_config_file, m_config.model_files.vocoder_path, true);

        bool dir_instead_of_file =
            model_file.find("fairseq") != std::string::npos ||
            model_file.find("xtts") != std::string::npos;

        auto use_cuda =
            m_config.use_gpu &&
            (py_executor::instance()->libs_availability->torch_cuda ||
             py_executor::instance()->libs_availability->torch_hip);

        LOGD("using device: " << (use_cuda ? "cuda" : "cpu") << " "
                              << m_config.gpu_device.id);

        try {
            auto api = py::module_::import("TTS.utils.synthesizer");

            m_model = api.attr("Synthesizer")(
                "tts_checkpoint"_a =
                    dir_instead_of_file
                        ? static_cast<py::object>(py::none())
                        : static_cast<py::object>(py::str(model_file)),
                "tts_config_path"_a =
                    dir_instead_of_file
                        ? static_cast<py::object>(py::str(py::none()))
                        : static_cast<py::object>(py::str(config_file)),
                "tts_speakers_file"_a = py::none(),
                "tts_languages_file"_a = py::none(),
                "vocoder_checkpoint"_a =
                    dir_instead_of_file || vocoder_model_file.empty()
                        ? static_cast<py::object>(py::none())
                        : static_cast<py::object>(py::str(vocoder_model_file)),
                "vocoder_config"_a =
                    dir_instead_of_file || vocoder_config_file.empty()
                        ? static_cast<py::object>(py::none())
                        : static_cast<py::object>(py::str(vocoder_config_file)),
                "encoder_checkpoint"_a = py::none(),
                "encoder_config"_a = py::none(),
                "model_dir"_a = dir_instead_of_file
                                    ? static_cast<py::object>(py::str(
                                          m_config.model_files.model_path))
                                    : static_cast<py::object>(py::none()),
                "use_cuda"_a = use_cuda);

            auto model = m_model->attr("tts_model");

            auto model_class_name =
                model.get_type().attr("__name__").cast<std::string>();
            LOGD("model class name: " << model_class_name);

            if (py::hasattr(model, "length_scale")) {
                m_initial_length_scale =
                    model.attr("length_scale").cast<float>();
                LOGD("initial length scale: " << *m_initial_length_scale);
            } else if (py::hasattr(model, "duration_threshold")) {
                m_initial_duration_threshold =
                    model.attr("duration_threshold").cast<float>();
                LOGD("initial duration threshold: "
                     << *m_initial_duration_threshold);
            } else if (model_class_name == "Xtts") {
                m_speed_supported = true;
            } else {
                LOGD("model does not have initial speed");
            }

            if (register_cancel_hook("gpt", model)) {
                register_cancel_hook("gpt_inference", model.attr("gpt"));
            }
            register_cancel_hook("hifigan_decoder", model);
            register_cancel_hook("text_encoder", model);
            register_cancel_hook("posterior_encoder", model);
            register_cancel_hook("flow", model);
            register_cancel_hook("waveform_decoder", model);
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get()))
        LOGE("failed to create coqui model");
    else
        LOGD("coqui model created");
}

bool coqui_engine::model_created() const { return static_cast<bool>(m_model); }

bool coqui_engine::encode_speech_impl(const std::string& text,
                                      unsigned int speed,
                                      const std::string& out_file) {
    auto task = py_executor::instance()->execute([&, speed]() {
        try {
            float speed_f = 1.0;

            if (m_speed_supported) {
                speed_f = static_cast<float>(speed) / 10.0F;
            } else {
                auto model = m_model->attr("tts_model");
                if (py::hasattr(model, "length_scale")) {
                    auto length_scale =
                        m_initial_length_scale
                            ? vits_length_scale(speed, *m_initial_length_scale)
                            : 1.0F;
                    model.attr("length_scale") = length_scale;

                    LOGD("speed: length_scale=" << length_scale);

                } else if (py::hasattr(model, "duration_threshold")) {
                    auto duration_threshold =
                        m_initial_duration_threshold
                            ? overflow_duration_threshold(
                                  speed, *m_initial_duration_threshold)
                            : 0.55F;

                    LOGD("speed: duration_threshold=" << duration_threshold);
                    model.attr("duration_threshold") = duration_threshold;
                }
            }

            auto wav = m_model->attr("tts")(
                "text"_a = text,
                "speaker_name"_a =
                    m_config.speaker_id.empty()
                        ? static_cast<py::object>(py::none())
                        : static_cast<py::object>(py::str(m_config.speaker_id)),
                "language_name"_a = m_config.lang_code.empty()
                                        ? m_config.lang
                                        : m_config.lang_code,
                "speaker_wav"_a = m_ref_voice_wav_file.empty()
                                      ? static_cast<py::object>(py::none())
                                      : static_cast<py::object>(
                                            py::str(m_ref_voice_wav_file)),
                "reference_wav"_a = py::none(), "style_wav"_a = py::none(),
                "style_text"_a = py::none(),
                "reference_speaker_name"_a = py::none(), "speed"_a = speed_f);

            if (is_shutdown()) throw std::runtime_error{"engine shutdown"};

            m_model->attr("save_wav")("wav"_a = wav, "path"_a = out_file);
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }

        LOGD("voice synthesized successfully");
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get())) return false;

    return true;
}

bool coqui_engine::model_supports_speed() const {
    return m_speed_supported || m_initial_length_scale ||
           m_initial_duration_threshold;
}

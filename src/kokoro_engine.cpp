/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "kokoro_engine.hpp"

#include <fmt/format.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"

using namespace pybind11::literals;

kokoro_engine::kokoro_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {
    if ((cpu_tools::cpuinfo().feature_flags &
         cpu_tools::feature_flags_t::avx) == 0) {
        LOGE("avx not supported but kokoro engine needs it");
        throw std::runtime_error(
            "failed to init kokoro engine: avx not supported");
    }

    if (m_config.model_files.hub_path.empty()) {
        LOGE("hub path missing but kokoro engine needs it");
        throw std::runtime_error("failed to init kokoro engine: no hub path");
    }
}

kokoro_engine::~kokoro_engine() {
    LOGD("kokoro dtor");

    stop();
}

void kokoro_engine::stop() {
    tts_engine::stop();

    auto task = py_executor::instance()->execute([&]() {
        try {
            m_pipeline.reset();
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

    LOGD("kokoro stopped");
}

void kokoro_engine::create_model() {
    auto task = py_executor::instance()->execute([&]() {
        auto model_file =
            first_file_with_ext(m_config.model_files.model_path, "pth");
        if (model_file.empty()) {
            LOGD("can't find model file in dir: "
                 << m_config.model_files.model_path);
            return false;
        }
        auto config_file =
            first_file_with_ext(m_config.model_files.model_path, "json");
        if (config_file.empty()) {
            LOGD("can't find config file in dir: "
                 << m_config.model_files.model_path);
            return false;
        }
        m_voice_file = fmt::format("{}/{}.pt", m_config.model_files.model_path,
                                   m_config.speaker_id);
        if (!file_exists(m_voice_file)) {
            LOGD("can't find voice file: " << m_voice_file);
            return false;
        }

        auto use_cuda =
            m_config.use_gpu &&
            (py_executor::instance()->libs_availability->torch_cuda ||
             py_executor::instance()->libs_availability->torch_hip);

        LOGD("using device: " << (use_cuda ? "cuda" : "cpu") << " "
                              << m_config.gpu_device.id);

        m_device_str = use_cuda ? "cuda" : "cpu";

        try {
            auto kokoro_api = py::module_::import("kokoro");
            m_model = kokoro_api.attr("KModel")("model"_a = model_file,
                                                "config"_a = config_file);
            m_pipeline = kokoro_api.attr("KPipeline")(
                "model"_a = m_model.value(),
                "lang_code"_a = m_config.speaker_id.empty()
                                    ? "a"
                                    : m_config.speaker_id.substr(0, 1),
                "device"_a = m_device_str);
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get()))
        LOGE("failed to create kokoro model");
    else
        LOGD("kokoro model created");
}

bool kokoro_engine::model_created() const {
    return m_model.has_value() && m_pipeline.has_value();
}

bool kokoro_engine::encode_speech_impl(const std::string& text,
                                       unsigned int speed,
                                       const std::string& out_file) {
    auto speech_speed = std::clamp(speed, 1U, 20U) / 10.0;

    auto task = py_executor::instance()->execute([&]() {
        try {
            auto generator = m_pipeline.value()(text, "voice"_a = m_voice_file,
                                                "speed"_a = speech_speed,
                                                "model"_a = m_model.value());
            auto next = generator.attr("__iter__")().attr("__next__");

            std::ofstream os{out_file, std::ios::binary};
            os.seekp(sizeof(wav_header));
            auto data_start = os.tellp();

            while (true) {
                if (is_shutdown()) break;

                try {
                    auto item = next();
                    py::array_t<float,
                                py::array::c_style | py::array::forcecast>
                        audio_arr =
                            item.attr("output").attr("audio").attr("squeeze")();
                    auto buffer = audio_arr.request();
                    for (ssize_t i = 0; i < buffer.size; ++i) {
                        // convert f32 to s16 sample format
                        auto sample = static_cast<int16_t>(
                            static_cast<float*>(buffer.ptr)[i] * 32768.0F);
                        os.write(reinterpret_cast<char*>(&sample), 2);
                    }
                } catch (const py::error_already_set& e) {
                    if (e.matches(PyExc_StopIteration)) {
                        break;
                    } else {
                        throw;
                    }
                }
            }

            auto data_size = os.tellp() - data_start;

            os.seekp(0);
            write_wav_header(s_sample_rate, sizeof(int16_t), 1,
                             data_size / sizeof(int16_t), os);

        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            unlink(out_file.c_str());
            return false;
        }

        LOGD("voice synthesized successfully");
        return true;
    });

    return task && std::any_cast<bool>(task->get());
}

bool kokoro_engine::model_supports_speed() const { return true; }

/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "qwen3tts_engine.hpp"

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

qwen3tts_engine::qwen3tts_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {
    if ((cpu_tools::cpuinfo().feature_flags &
         cpu_tools::feature_flags_t::avx) == 0) {
        LOGE("avx not supported but qwen3tts engine needs it");
        throw std::runtime_error(
            "failed to init qwen3tts engine: avx not supported");
    }
}

qwen3tts_engine::~qwen3tts_engine() {
    LOGD("qwen3tts dtor");

    stop();
}

void qwen3tts_engine::stop() {
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

    LOGD("qwen3tts stopped");
}

void qwen3tts_engine::create_model() {
    auto task = py_executor::instance()->execute([&]() {
        auto use_cuda =
            m_config.use_gpu &&
            (py_executor::instance()->libs_availability->torch_cuda ||
             py_executor::instance()->libs_availability->torch_hip);

        LOGD("using device: " << (use_cuda ? "cuda" : "cpu") << " "
                              << m_config.gpu_device.id);

        m_device_str = use_cuda ? "cuda" : "cpu";

        try {
            auto qwen_tts = py::module_::import("qwen_tts");
            auto torch = py::module_::import("torch");

            // Use local model path if available, otherwise use HF Hub name
            auto model_path = m_config.model_files.model_path;
            if (model_path.empty() || model_path == ".") {
                model_path = "Qwen/Qwen3-TTS-12Hz-0.6B-CustomVoice";
            }

            LOGD("loading qwen3tts model from: " << model_path);

            auto dtype = use_cuda ? torch.attr("float16")
                                  : torch.attr("float32");

            auto cache_dir = m_config.cache_dir;
            if (cache_dir.empty()) {
                m_model = qwen_tts.attr("QwenTTS")(
                    "model_path"_a = model_path, "device"_a = m_device_str,
                    "dtype"_a = dtype);
            } else {
                m_model = qwen_tts.attr("QwenTTS")(
                    "model_path"_a = model_path, "device"_a = m_device_str,
                    "dtype"_a = dtype, "cache_dir"_a = cache_dir);
            }

        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get()))
        LOGE("failed to create qwen3tts model");
    else
        LOGD("qwen3tts model created");
}

bool qwen3tts_engine::model_created() const { return m_model.has_value(); }

bool qwen3tts_engine::encode_speech_impl(const std::string& text,
                                          unsigned int speed,
                                          const std::string& out_file) {
    auto speech_speed = std::clamp(speed, 1U, 20U) / 10.0;

    auto task = py_executor::instance()->execute([&]() {
        try {
            // Determine speaker from config, default to French female voice
            auto speaker = m_config.speaker_id;
            if (speaker.empty()) {
                speaker = "Chelsie";
            }

            LOGD("qwen3tts generating speech with speaker: "
                 << speaker << ", speed: " << speech_speed);

            // Use generate_custom_voice for preset speakers
            auto result = m_model->attr("generate_custom_voice")(
                text, "speaker"_a = speaker, "speed"_a = speech_speed);

            // result is a dict with 'audio' (numpy array) and 'sample_rate'
            py::array_t<float, py::array::c_style | py::array::forcecast>
                audio_arr = result.attr("__getitem__")("audio")
                                .attr("squeeze")()
                                .attr("cpu")()
                                .attr("numpy")();

            auto result_sample_rate =
                result.attr("__getitem__")("sample_rate").cast<int>();

            auto buffer = audio_arr.request();

            std::ofstream os{out_file, std::ios::binary};
            os.seekp(sizeof(wav_header));
            auto data_start = os.tellp();

            for (ssize_t i = 0; i < buffer.size; ++i) {
                if (is_shutdown()) throw std::runtime_error{"engine shutdown"};

                // convert f32 to s16 sample format
                auto sample = static_cast<int16_t>(
                    std::clamp(static_cast<float*>(buffer.ptr)[i], -1.0F,
                               1.0F) *
                    32767.0F);
                os.write(reinterpret_cast<char*>(&sample), 2);
            }

            auto data_size = os.tellp() - data_start;

            os.seekp(0);
            write_wav_header(result_sample_rate, sizeof(int16_t), 1,
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

bool qwen3tts_engine::model_supports_speed() const { return true; }

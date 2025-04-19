/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "f5_engine.hpp"

#include <fmt/format.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"

using namespace pybind11::literals;

f5_engine::f5_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {
    if ((cpu_tools::cpuinfo().feature_flags &
         cpu_tools::feature_flags_t::avx) == 0) {
        LOGE("avx not supported but f5 engine needs it");
        throw std::runtime_error("failed to init f5 engine: avx not supported");
    }

    if (m_config.model_files.hub_path.empty()) {
        LOGE("hub path missing but f5 engine needs it");
        throw std::runtime_error("failed to init f5 engine: no hub path");
    }
}

f5_engine::~f5_engine() {
    LOGD("f5 dtor");

    stop();
}

void f5_engine::stop() {
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

    LOGD("f5 stopped");
}

void f5_engine::create_model() {
    auto task = py_executor::instance()->execute([&]() {
        auto model_file =
            first_file_with_ext(m_config.model_files.model_path, "safetensors");
        if (model_file.empty()) {
            LOGD("can't find model file in dir: "
                 << m_config.model_files.model_path);
            return false;
        }
        auto vocab_file =
            first_file_with_ext(m_config.model_files.model_path, "txt");
        if (vocab_file.empty()) {
            LOGD("can't find vocab file in dir: "
                 << m_config.model_files.model_path);
            return false;
        }

        auto use_cuda =
            m_config.use_gpu &&
            (py_executor::instance()->libs_availability->torch_cuda ||
             py_executor::instance()->libs_availability->torch_hip);

        LOGD("using device: " << (use_cuda ? "cuda" : "cpu") << " "
                              << m_config.gpu_device.id);

        m_device_str =
            use_cuda ? fmt::format("cuda:{}", m_config.gpu_device.id) : "cpu";

        make_hf_link("charactr--vocos-mel-24khz", m_config.model_files.hub_path,
                     m_config.cache_dir);

        try {
            auto f5_api = py::module_::import("f5_tts.api");
            m_model = f5_api.attr("F5TTS")("ckpt_file"_a = model_file,
                                           "vocab_file"_a = vocab_file,
                                           "device"_a = m_device_str);

            auto hook = py::cpp_function{
                [this]([[maybe_unused]] const py::args& args,
                       [[maybe_unused]] const py::kwargs& kwargs) {
                    if (is_shutdown()) {
                        throw std::runtime_error{"engine shutdown"};
                    }
                }};
            m_model->attr("ema_model")
                .attr("transformer")
                .attr("register_forward_hook")(hook);
            m_model->attr("ema_model")
                .attr("transformer")
                .attr("register_forward_pre_hook")(hook);
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get()))
        LOGE("failed to create f5 model");
    else
        LOGD("f5 model created");
}

bool f5_engine::model_created() const { return m_model.has_value(); }

bool f5_engine::encode_speech_impl(const std::string& text,
                                       [[maybe_unused]] unsigned int speed,
                                       const std::string& out_file) {
    auto speech_speed = std::clamp(speed, 1U, 20U) / 10.0;

    auto task = py_executor::instance()->execute([&]() {
        try {
            // to speed up decrease "nfe_step", e.g. "nfe_step"_a = 16
            m_model->attr("infer")(
                "ref_file"_a = m_ref_voice_wav_file,
                "ref_text"_a = m_ref_voice_text, "gen_text"_a = text,
                "file_wave"_a = out_file, "speed"_a = speech_speed,
                "remove_silence"_a = false, "seed"_a = py::none(),
                "file_spec"_a = py::none());

            if (is_shutdown()) throw std::runtime_error{"engine shutdown"};
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }

        LOGD("voice synthesized successfully");
        return true;
    });

    return task && std::any_cast<bool>(task->get());
}

bool f5_engine::model_supports_speed() const { return true; }

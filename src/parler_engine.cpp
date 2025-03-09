/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "parler_engine.hpp"

#include <fmt/format.h>
#include <pybind11/numpy.h>

#include <utility>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"

using namespace pybind11::literals;

parler_engine::parler_engine(config_t config, callbacks_t call_backs)
    : tts_engine{std::move(config), std::move(call_backs)} {
    if ((cpu_tools::cpuinfo().feature_flags &
         cpu_tools::feature_flags_t::avx) == 0) {
        LOGE("avx not supported but parler engine needs it");
        throw std::runtime_error(
            "failed to init parler engine: avx not supported");
    }
}

parler_engine::~parler_engine() {
    LOGD("parler dtor");

    stop();
}

void parler_engine::stop() {
    tts_engine::stop();

    auto task = py_executor::instance()->execute([&]() {
        try {
            m_desc_input_ids.reset();
            m_desc_attention_mask.reset();
            m_model.reset();
            m_tokenizer.reset();

            // release mem
            py::module_::import("gc").attr("collect")();
            py::module_::import("torch").attr("cuda").attr("empty_cache")();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();

    LOGD("parler stopped");
}

// must be run in py thread after create_model
void parler_engine::create_desc() {
    if (m_desc_input_ids && m_desc_attention_mask) return;

    auto desc_encoding = m_tokenizer.value()(
        m_config.ref_prompt, "return_tensors"_a = "pt", "padding"_a = true);
    m_desc_input_ids = desc_encoding.attr("input_ids").attr("to")(m_device_str);
    m_desc_attention_mask =
        desc_encoding.attr("attention_mask").attr("to")(m_device_str);
}

void parler_engine::reset_ref_voice() {
    auto task = py_executor::instance()->execute([&]() {
        LOGD("reset ref prompt");
        try {
            m_desc_input_ids.reset();
            m_desc_attention_mask.reset();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();
}

void parler_engine::create_model() {
    auto task = py_executor::instance()->execute([&]() {
        auto use_cuda =
            m_config.use_gpu &&
            (py_executor::instance()->libs_availability->torch_cuda ||
             py_executor::instance()->libs_availability->torch_hip);

        LOGD("using device: " << (use_cuda ? "cuda" : "cpu") << " "
                              << m_config.gpu_device.id);

        m_device_str =
            use_cuda ? fmt::format("cuda:{}", m_config.gpu_device.id) : "cpu";

        try {
            auto parler_api = py::module_::import("parler_tts");
            auto transformers_api = py::module_::import("transformers");

            m_model =
                parler_api.attr("ParlerTTSForConditionalGeneration")
                    .attr("from_pretrained")(m_config.model_files.model_path,
                                             "low_cpu_mem_usage"_a = true)
                    .attr("to")(m_device_str);
            m_tokenizer =
                transformers_api.attr("AutoTokenizer")
                    .attr("from_pretrained")(m_config.model_files.model_path,
                                             "use_fast"_a = true);

            create_desc();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }
        return true;
    });

    if (!task || !std::any_cast<bool>(task->get()))
        LOGE("failed to create parler model");
    else
        LOGD("parler model created");
}

bool parler_engine::model_created() const {
    return m_model && m_tokenizer && m_desc_input_ids && m_desc_attention_mask;
}

bool parler_engine::encode_speech_impl(const std::string& text,
                                       [[maybe_unused]] unsigned int speed,
                                       const std::string& out_file) {
    auto task = py_executor::instance()->execute([&]() {
        try {
            create_desc();

            auto prompt_input_ids =
                m_tokenizer
                    .value()(text, "return_tensors"_a = "pt",
                             "padding"_a = true)
                    .attr("input_ids")
                    .attr("to")(m_device_str);
            auto generation = m_model->attr("generate")(
                "input_ids"_a = m_desc_input_ids.value(),
                "prompt_input_ids"_a = prompt_input_ids,
                "attention_mask"_a = m_desc_attention_mask.value());

            py::array_t<float, py::array::c_style | py::array::forcecast>
                audio_arr =
                    generation.attr("cpu")().attr("numpy")().attr("squeeze")();

            // save to wav file
            auto buffer = audio_arr.request();
            std::ofstream os{out_file, std::ios::binary};
            write_wav_header(
                m_model->attr("config").attr("sampling_rate").cast<int>(),
                sizeof(int16_t), 1, buffer.size, os);
            for (ssize_t i = 0; i < buffer.size; ++i) {
                // convert f32 to s16 sample format
                auto sample = static_cast<int16_t>(
                    static_cast<float*>(buffer.ptr)[i] * 32768.0F);
                os.write(reinterpret_cast<char*>(&sample), 2);
            }
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            return false;
        }

        LOGD("voice synthesized successfully");
        return true;
    });

    return task && std::any_cast<bool>(task->get());
}

bool parler_engine::model_supports_speed() const { return false; }

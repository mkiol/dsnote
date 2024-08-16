/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "punctuator.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>

#include "logger.hpp"
#include "py_executor.hpp"

using namespace pybind11::literals;

punctuator::punctuator(const std::string& model_path, int device) {
    auto task = py_executor::instance()->execute(
        [&, dev = (py_executor::instance()->libs_availability->torch_cuda ||
                   py_executor::instance()->libs_availability->torch_hip)
                      ? device
                      : -1]() {
            try {
                LOGD("creating punctuator: device=" << dev);

                auto trans_module = py::module_::import("transformers");
                auto tokenizer_class = trans_module.attr("AutoTokenizer");
                auto model_class =
                    trans_module.attr("AutoModelForTokenClassification");
                auto pipeline_class =
                    trans_module.attr("TokenClassificationPipeline");

                auto tokenizer = tokenizer_class.attr("from_pretrained")(
                    model_path, "local_files_only"_a = true,
                    "low_cpu_mem_usage"_a = true);
                auto model = model_class.attr("from_pretrained")(
                    model_path, "local_files_only"_a = true,
                    "low_cpu_mem_usage"_a = true);

                m_pipeline = pipeline_class(
                    "model"_a = model, "tokenizer"_a = tokenizer,
                    "aggregation_strategy"_a = "simple", "device"_a = dev);
            } catch (const std::exception& err) {
                LOGE("py error: " << err.what());
                throw std::runtime_error(std::string{"py error: "} +
                                         err.what());
            }

            return std::any{};
        });

    if (task) task->get();
}

punctuator::~punctuator() {
    LOGD("puntuator dtor");

    auto task = py_executor::instance()->execute([&]() {
        try {
            m_pipeline.reset();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();

    LOGD("puntuator stopped");
}

std::string punctuator::process(std::string text) {
    auto task =
        py_executor::instance()->execute(
            [&]() {
                try {
                    auto result = m_pipeline->attr("__call__")(text);

                    if (!result.is_none()) {
                        auto list = result.cast<py::list>();

                        text = std::accumulate(
                            list.begin(), list.end(), std::string{},
                            [](auto text, const auto& item) {
                                const auto& dict =
                                    item.template cast<py::dict>();
                                const auto& eg =
                                    dict["entity_group"]
                                        .template cast<decltype(text)>();
                                auto word =
                                    dict["word"]
                                        .template cast<decltype(text)>();

                                if (!word.empty() &&
                                    (text.empty() || text.back() == '.' ||
                                     text.back() == '?' || text.back() == '!'))
                                    word.front() = std::toupper(word.front());

                                if (!text.empty()) text += " ";

                                text += word;

                                if (eg != "0") text += eg;

                                return text;
                            });
                    }
                } catch (const std::exception& err) {
                    LOGE(
                        "failed to restore punctuation, error: " << err.what());
                }

                return text;
            });

    if (task) return std::any_cast<std::string>(task->get());

    return text;
}

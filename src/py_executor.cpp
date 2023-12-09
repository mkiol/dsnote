/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "py_executor.hpp"

#include <fmt/format.h>

#include <cstdlib>
#include <stdexcept>
#include <string>

#include "logger.hpp"
#include "settings.h"

py_executor::~py_executor() {
    LOGD("py_executor dtor");

    stop();
}

void py_executor::start() {
    if (!m_thread.joinable()) m_thread = std::thread{&py_executor::loop, this};
}

void py_executor::stop() {
    LOGD("shutdown requested");

    m_shutting_down = true;
    m_cv.notify_one();

    if (m_thread.joinable()) m_thread.join();

    LOGD("shutdown completed");
}

std::future<std::string> py_executor::execute(task_t task) {
    if (m_shutting_down)
        throw std::runtime_error("failed to execute task due to shutdown");

    {
        std::lock_guard lock{m_mutex};

        m_task.emplace(std::move(task));
        m_promise.emplace();
    }

    LOGD("task pushed");

    m_cv.notify_one();

    return m_promise->get_future();
}

static std::string add_to_env_path(const std::string& dir) {
    try {
        auto* old_path = getenv("PYTHONPATH");
        if (old_path)
            setenv("PYTHONPATH", fmt::format("{}:{}", dir, old_path).c_str(),
                   true);
        else
            setenv("PYTHONPATH", dir.c_str(), false);
    } catch (const std::runtime_error& err) {
        qWarning() << "error:" << err.what();
    }

    auto* new_path = getenv("PYTHONPATH");
    return new_path ? std::string{new_path} : std::string{};
}

void py_executor::loop() {
    LOGD("py executor loop started");

    setenv("PYTHONIOENCODING", "utf-8", true);

    py_tools::init_module();

    auto py_path = settings::instance()->py_path().toStdString();
    if (!py_path.empty()) {
        LOGD("adding to PYTHONPATH: " << py_path);
        auto new_path = add_to_env_path(py_path);
        LOGD("new PYTHONPATH: " << new_path);
    }

    try {
        m_py_interpreter.emplace();

        if (settings::instance()->py_feature_scan()) {
            libs_availability = py_tools::libs_availability();
        } else {
            LOGW("py scan is off");
            libs_availability = py_tools::libs_availability_t{};
        }

        while (!m_shutting_down) {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_cv.wait(lock,
                      [this] { return m_shutting_down || m_task.has_value(); });

            auto task = std::move(m_task);
            m_task.reset();

            if (m_shutting_down) {
                if (task) m_promise->set_value({});
                break;
            }

            try {
                m_promise->set_value(task.value()());
            } catch (const std::exception& err) {
                LOGE("py task error: " << err.what());
                m_promise->set_exception(std::current_exception());
            }
        }

        m_py_interpreter.reset();
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
    }

    LOGD("py executor loop ended");
}

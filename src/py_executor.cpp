/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "py_executor.hpp"

#include <stdexcept>

#include "logger.hpp"
#ifdef USE_SFOS
#include "py_tools.hpp"
#endif

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

void py_executor::loop() {
    LOGD("py executor loop started");

#ifdef USE_PYTHON_MODULE
    py_tools::init_module();
#endif

    try {
        m_py_interpreter.emplace();

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

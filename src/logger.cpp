/* Copyright (C) 2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "logger.hpp"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <threads.h>

#include <chrono>
#include <cstdio>

Logger::LogType Logger::m_level = Logger::LogType::Error;
std::optional<std::ofstream> Logger::m_file = std::nullopt;

std::ostream &operator<<(std::ostream &os, Logger::LogType type) {
    switch (type) {
        case Logger::LogType::Trace:
            os << "trace";
            break;
        case Logger::LogType::Debug:
            os << "debug";
            break;
        case Logger::LogType::Info:
            os << "info";
            break;
        case Logger::LogType::Warning:
            os << "warning";
            break;
        case Logger::LogType::Error:
            os << "error";
            break;
        case Logger::LogType::Quiet:
            os << "quiet";
            break;
    }
    return os;
}

void Logger::init(LogType level, const std::string &file) {
    m_level = level;

    if (file.empty()) {
        m_file.reset();
        LOGI("logging to stderr enabled");
    } else {
        m_file.emplace(file, std::ios::trunc);
        if (!m_file->good()) {
            m_file.reset();
            LOGW("failed to create log file: " << file);
        } else {
            LOGI("logging to file enabled");
        }
    }
}

void Logger::setLevel(LogType level) {
    if (m_level != level) {
        auto old = m_level;
        m_level = level;
        LOGD("logging level changed: " << old << " => " << m_level);
    }
}

Logger::LogType Logger::level() { return m_level; }

bool Logger::match(LogType type) {
    return static_cast<int>(type) >= static_cast<int>(m_level);
}

Logger::Message::Message(LogType type, const char *file, const char *function,
                         int line)
    : m_type{type}, m_file{file}, m_fun{function}, m_line{line} {}

inline static auto typeToChar(Logger::LogType type) {
    switch (type) {
        case Logger::LogType::Trace:
            return 'T';
        case Logger::LogType::Debug:
            return 'D';
        case Logger::LogType::Info:
            return 'I';
        case Logger::LogType::Warning:
            return 'W';
        case Logger::LogType::Error:
            return 'E';
        case Logger::LogType::Quiet:
            return 'Q';
    }
    return '-';
}

Logger::Message::~Message() {
    if (!match(m_type)) return;

    auto now = std::chrono::system_clock::now();
    auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now.time_since_epoch())
                     .count() %
                 1000;

    const auto str = m_os.str();
    if (str.empty()) return;

    if (m_fun == nullptr || m_fun[0] == '\0') m_fun = m_emptyStr;

    auto fmt =
        fmt::format("[{{0}}] {{1:%H:%M:%S}}.{{2}} {{3:#10x}} {{4}}{}- {{5}}{}",
                    m_line > 0 ? ":{6} " : " ", str.back() == '\n' ? "" : "\n");
    try {
        auto line = fmt::format(fmt, typeToChar(m_type), now, msecs,
                                thrd_current(), m_fun, str, m_line);
        if (Logger::m_file) {
            *Logger::m_file << line;
            Logger::m_file->flush();
        } else {
            fmt::print(stderr, line);
            fflush(stderr);
        }
    } catch (const std::runtime_error &e) {
        fmt::print(stderr, "logger error: {}\n", e.what());
    }
}

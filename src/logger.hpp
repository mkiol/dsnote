/* Copyright (C) 2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <optional>
#include <sstream>

#ifdef USE_TRACE_LOGS
#define LOGT(msg) \
    Logger::Message(logger::LogType::Trace, __FILE__, __func__, __LINE__) << msg
#else
#define LOGT(msg)
#endif
#define LOGD(msg) \
    Logger::Message(Logger::LogType::Debug, __FILE__, __func__, __LINE__) << msg
#define LOGI(msg) \
    Logger::Message(Logger::LogType::Info, __FILE__, __func__, __LINE__) << msg
#define LOGW(msg)                                                           \
    Logger::Message(Logger::LogType::Warning, __FILE__, __func__, __LINE__) \
        << msg
#define LOGE(msg) \
    Logger::Message(Logger::LogType::Error, __FILE__, __func__, __LINE__) << msg

class Logger {
   public:
    enum class LogType {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Quiet = 5
    };
    friend std::ostream &operator<<(std::ostream &os, LogType type);

    class Message {
        std::ostringstream m_os;
        LogType m_type;
        const char *m_file;
        const char *m_fun;
        int m_line;

       public:
        Message(LogType type, const char *file, const char *function, int line);
        ~Message();

        template <typename T>
        Message &operator<<(const T &t) {
            m_os << std::boolalpha << t;
            return *this;
        }
    };

    static void init(LogType level, const std::string &file = {});
    static void setLevel(LogType level);
    static LogType level();
    static bool match(LogType type);
    Logger() = delete;

   private:
    inline static const char *m_emptyStr = "()";
    static LogType m_level;
    static std::optional<std::ofstream> m_file;
};

#endif  // LOGGER_H

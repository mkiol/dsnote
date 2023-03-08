/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "qtlogger.hpp"

#include <fmt/printf.h>

#include <QMessageLogContext>
#include <QString>
#include <QtGlobal>

#include "logger.hpp"

static void qtLog(QtMsgType qtType, const QMessageLogContext &qtContext,
                  const QString &qtMsg) {
    Logger::Message msg{[qtType] {
                            switch (qtType) {
                                case QtDebugMsg:
                                    return Logger::LogType::Debug;
                                case QtInfoMsg:
                                    return Logger::LogType::Info;
                                case QtWarningMsg:
                                    return Logger::LogType::Warning;
                                case QtCriticalMsg:
                                case QtFatalMsg:
                                    return Logger::LogType::Error;
                            }
                            return Logger::LogType::Debug;
                        }(),
                        qtContext.file ? qtContext.file : "",
                        qtContext.function ? qtContext.function : "",
                        qtContext.line};
    msg << qtMsg.toStdString();
}

void initQtLogger() { qInstallMessageHandler(qtLog); }

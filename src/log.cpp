/* Copyright (C) 2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "log.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QThread>
#include <cstdlib>

void qtLog(QtMsgType type, const QMessageLogContext &context,
           const QString &msg) {
    char t;
    switch (type) {
        case QtDebugMsg:
            t = 'D';
            break;
        case QtInfoMsg:
            t = 'I';
            break;
        case QtWarningMsg:
            t = 'W';
            break;
        case QtCriticalMsg:
            t = 'C';
            break;
        case QtFatalMsg:
            t = 'F';
            break;
        default:
            t = '?';
    }

    fprintf(stderr, "[%c] %s %p %s:%u - %s\n", t,
            QDateTime::currentDateTime()
                .toString(QStringLiteral("hh:mm:ss.zzz"))
                .toLatin1()
                .constData(),
            static_cast<void *>(QThread::currentThread()),
            context.function ? context.function : "", context.line,
            msg.toLocal8Bit().constData());
    fflush(stderr);
}

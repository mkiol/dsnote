/* Copyright (C) 2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LOGF_H
#define LOGF_H

#include <QMessageLogContext>
#include <QString>
#include <cstdio>

void qtLog(QtMsgType type, const QMessageLogContext &context,
           const QString &msg);

#endif  // LOGF_H

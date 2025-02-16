/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef QTLOGGER_HPP
#define QTLOGGER_HPP

#include <QDBusError>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <iostream>

std::ostream &operator<<(std::ostream &os, const QString &msg);
std::ostream &operator<<(std::ostream &os, const QStringList &msg);
void initQtLogger();

#endif  // QTLOGGER_HPP

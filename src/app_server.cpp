/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "app_server.hpp"

#include <QDBusConnection>
#include <QDebug>
#include <algorithm>

#include "settings.h"

app_server::app_server(QObject *parent)
    : QObject{parent}, m_dbus_service_adaptor{this} {
    // DBus
    auto con = QDBusConnection::sessionBus();
    if (!con.registerService(DBUS_SERVICE_NAME)) {
        qWarning() << "dbus app service registration failed";
        throw std::runtime_error("dbus app service registration failed");
    }
    if (!con.registerObject(DBUS_SERVICE_PATH, this)) {
        qWarning() << "dbus app object registration failed";
        throw std::runtime_error("dbus app object registration failed");
    }
}

void app_server::Activate(const QVariantMap &platform_data) {
    qDebug() << "[dbus app] Activate called:" << platform_data;

    emit activate_requested();
}

void app_server::ActivateAction(
    const QString &action_name, [[maybe_unused]] const QVariantList &parameter,
    [[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] ActivateAction called:" << action_name;
}

void app_server::Open(const QStringList &uris,
                      [[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] Open called:" << uris;

    QStringList files;
    std::transform(uris.cbegin(), uris.cend(), std::back_inserter(files),
                   [](const auto &uri) { return QUrl{uri}.toLocalFile(); });

    if (files.isEmpty()) return;

    emit files_to_open_requested(files);
}

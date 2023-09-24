/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "app_server.hpp"

#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

app_server::app_server(QObject *parent)
    : QObject{parent},
#ifdef USE_SFOS
      m_dbus_service{this}
#else
      m_dbus_service{KDBusService::Unique}
#endif
{
#ifdef USE_SFOS
    auto con = QDBusConnection::sessionBus();
    if (!con.registerService(DBUS_SERVICE_NAME)) {
        qWarning() << "dbus app service registration failed";
        throw std::runtime_error(
            "dbus app service registration failed, maybe another instance is "
            "running");
    }
    if (!con.registerObject(DBUS_SERVICE_PATH, this)) {
        qWarning() << "dbus app object registration failed, maybe another "
                      "instance is running";
        throw std::runtime_error("dbus app object registration failed");
    }
#else
    connect(&m_dbus_service, &KDBusService::activateRequested, this,
            &app_server::activateRequested, Qt::QueuedConnection);
    connect(&m_dbus_service, &KDBusService::activateActionRequested, this,
            &app_server::activateActionRequested, Qt::QueuedConnection);
    connect(&m_dbus_service, &KDBusService::openRequested, this,
            &app_server::openRequested, Qt::QueuedConnection);
#endif
}

void app_server::files_to_open(const QStringList &files) {
    if (files.isEmpty()) {
        emit activate_requested();
    } else {
        emit files_to_open_requested(files);
    }
}

#ifdef USE_SFOS
void app_server::Activate([[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] Activate called";

    emit activate_requested();
}

void app_server::ActivateAction(
    const QString &action_name, [[maybe_unused]] const QVariantList &parameter,
    [[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] ActivateAction called:" << action_name;

    emit activate_requested();
}

void app_server::Open(const QStringList &uris,
                      [[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] Open called:" << uris;

    QStringList files;
    std::transform(uris.cbegin(), uris.cend(), std::back_inserter(files),
                   [](const auto &uri) { return QUrl{uri}.toLocalFile(); });

    files_to_open(files);
}
#else
void app_server::activateRequested(const QStringList& arguments,
                                   const QString& workingDirectory) {
    qDebug() << "[dbus app] activateRequested:" << arguments
             << workingDirectory;

    if (arguments.isEmpty()) return;

    QStringList files;
    std::for_each(
        std::next(arguments.cbegin()), arguments.cend(),
        [&](const QString& argument) {
            if (argument.startsWith('-')) return;
            auto scheme = QUrl{argument}.scheme();
            if (scheme.isEmpty()) {
                files.push_back(
                    QDir{workingDirectory}.absoluteFilePath(argument));
            } else if (scheme == "file") {
                files.push_back(QUrl{argument}.toLocalFile());
            } else {
                qWarning() << "ignoring file argument:" << argument;
            }
        });

    files_to_open(files);
}
void app_server::activateActionRequested(const QString& actionName,
                                         const QVariant& parameter) {
    qDebug() << "[dbus app] activateActionRequested:" << actionName
             << parameter;

    emit activate_requested();
}
void app_server::openRequested(const QList<QUrl>& uris) {
    qDebug() << "[dbus app] openRequested:" << uris;

    QStringList files;
    std::transform(uris.cbegin(), uris.cend(), std::back_inserter(files),
                   [](const auto& uri) { return uri.toLocalFile(); });

    files_to_open(files);
}
#endif

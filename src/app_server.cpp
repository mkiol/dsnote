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

#include "text_tools.hpp"

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

    emit action_requested(action_name, {});
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

    QString action;
    QString id;
    QString text;
    QStringList files;

    enum class option_t : uint8_t { none, action, id, text };
    option_t option = option_t::none;

    std::for_each(
        std::next(arguments.cbegin()), arguments.cend(),
        [&](const QString& argument) {
            if (argument == "--action") {
                option = option_t::action;
                return;
            }
            if (argument == "--id") {
                option = option_t::id;
                return;
            }
            if (argument == "--text") {
                option = option_t::text;
                return;
            }
            if (argument.startsWith('-')) {
                option = option_t::none;
                return;
            }

            if (option == option_t::action) {
                action = argument;
                option = option_t::none;
            } else if (option == option_t::id) {
                id = argument;
                option = option_t::none;
            } else if (option == option_t::text) {
                text = argument;
                option = option_t::none;
            } else {
                auto scheme = QUrl{argument}.scheme();
                if (scheme.isEmpty()) {
                    files.push_back(
                        QDir{workingDirectory}.absoluteFilePath(argument));
                } else if (scheme == "file") {
                    files.push_back(QUrl{argument}.toLocalFile());
                } else {
                    qWarning() << "ignoring file argument:" << argument;
                }
            }
        });

    if (action.isEmpty()) {
        files_to_open(files);
    } else {
        QString extra;
        bool id_ok =
            !id.isEmpty() && text_tools::valid_model_id(id.toStdString());

        if (action.compare("set-stt-model", Qt::CaseInsensitive) == 0 ||
            action.compare("set-tts-model", Qt::CaseInsensitive) == 0) {
            if (!id_ok) {
                qWarning() << "missing or invalid language or model id";
                return;
            }
            extra = QStringLiteral("{%1}").arg(id);
        } else if (action.compare("start-reading-text", Qt::CaseInsensitive) ==
                   0) {
            if (text.isEmpty()) {
                qWarning() << "missing text to read";
                return;
            }
            if (!id_ok) {
                extra = std::move(text);
            } else {
                extra = QStringLiteral("{%1}%2").arg(id, text);
            }
        } else if (action.compare("start-reading-clipboard",
                                  Qt::CaseInsensitive) == 0) {
            if (id_ok) {
                extra = QStringLiteral("{%1}").arg(id);
            }
        }

        emit action_requested(action, extra);
    }
}

void app_server::activateActionRequested(const QString& actionName,
                                         const QVariant& parameter) {
    qDebug() << "[dbus app] activateActionRequested:" << actionName
             << parameter;

    emit action_requested(actionName, parameter.toString());
}

void app_server::openRequested(const QList<QUrl>& uris) {
    qDebug() << "[dbus app] openRequested:" << uris;

    QStringList files;
    std::transform(uris.cbegin(), uris.cend(), std::back_inserter(files),
                   [](const auto& uri) { return uri.toLocalFile(); });

    files_to_open(files);
}
#endif

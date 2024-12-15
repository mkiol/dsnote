/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "app_server.hpp"

#include <fmt/format.h>

#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

#include "dbus_application_inf.h"
#include "dbus_dsnote_inf.h"

app_server::app_server(const cmd::options &options, QObject *parent)
    : QObject{parent},
      m_dbus_application_adaptor{this},
      m_dbus_dsnote_adaptor{this} {
    auto con = QDBusConnection::sessionBus();

    if (con.registerService(DBUS_SERVICE_NAME) &&
        con.registerObject(DBUS_SERVICE_PATH, this)) {
        qDebug() << "dbus app service registration successul";

        m_pending_request_timer.setSingleShot(true);
        m_pending_request_timer.setInterval(500);  // 1s;
        connect(
            &m_pending_request_timer, &QTimer::timeout, this,
            [this, options]() {
                if (state() > 2) {
                    qDebug() << "pending request";
                    request_another_instance(options);
                } else {
                    m_pending_request_timer.start();
                }
            },
            Qt::QueuedConnection);
        m_pending_request_timer.start();
    } else {
        qDebug() << "dbus app service registration failed => maybe another "
                    "instance is running";

        request_another_instance(options);

        exit(0);
    }
}

void app_server::request_another_instance(const cmd::options &options) {
    if (!options.valid) return;

    if (options.action.isEmpty()) {
        if (!options.files.isEmpty()) {
            OrgFreedesktopApplicationInterface iface{
                DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                QDBusConnection::sessionBus()};
            iface.setTimeout(DBUS_TIMEOUT_MS);

            QStringList uris;
            std::transform(options.files.cbegin(), options.files.cend(),
                           std::back_inserter(uris), [](const auto &path) {
                               return QUrl::fromLocalFile(path).toString();
                           });

            qDebug() << "[dbus client] calling Open on another instance:"
                     << uris;
            iface.Open(uris, {}).waitForFinished();
        }
    } else {
        OrgFreedesktopApplicationInterface iface{DBUS_SERVICE_NAME,
                                                 DBUS_SERVICE_PATH,
                                                 QDBusConnection::sessionBus()};
        iface.setTimeout(DBUS_TIMEOUT_MS);

        qDebug() << "[dbus client] calling ActivateAction on another instance:"
                 << options.action;
        iface.ActivateAction(options.action, QVariantList{} << options.extra,
                             {});
    }

    if (options.model_list_types != cmd::model_type_flag::none ||
        options.active_model_types != cmd::model_type_flag::none ||
        options.print_state) {
        DsnoteDbusInterface iface(DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                                  QDBusConnection::sessionBus());
        iface.setTimeout(DBUS_TIMEOUT_MS);

        if (options.print_state) {
            fmt::print("State:\n\t{}\nTask state:\n\t{}\n", iface.state(),
                       iface.taskState());
        }

        if (options.model_list_types & cmd::model_type_flag::stt) {
            QDBusReply<QStringList> reply = iface.GetSttModelIds();
            if (reply.isValid()) {
                fmt::print("Available STT models:\n");
                for (const auto &id : reply.value()) {
                    fmt::print("\t{}\n", id.toStdString());
                }
            }
        }
        if (options.model_list_types & cmd::model_type_flag::tts) {
            QDBusReply<QStringList> reply = iface.GetTtsModelIds();
            if (reply.isValid()) {
                fmt::print("Available TTS models:\n");
                for (const auto &id : reply.value()) {
                    fmt::print("\t{}\n", id.toStdString());
                }
            }
        }
        if (options.active_model_types & cmd::model_type_flag::stt) {
            fmt::print("Active STT model:\n\t{}\n",
                       iface.activeSttModelId().toStdString());
        }
        if (options.active_model_types & cmd::model_type_flag::tts) {
            fmt::print("Active TTS model:\n\t{}\n",
                       iface.activeTtsModelId().toStdString());
        }
    }
}

void app_server::files_to_open(const QStringList &files) {
    if (files.isEmpty()) {
        emit activate_requested();
    } else {
        emit files_to_open_requested(files);
    }
}

void app_server::handle_active_stt_model_change() {
    emit ActiveSttModelIdPropertyChanged(active_stt_model_id());
}

void app_server::handle_active_tts_model_change() {
    emit ActiveTtsModelIdPropertyChanged(active_tts_model_id());
}

void app_server::handle_state_change() { emit StatePropertyChanged(state()); }

void app_server::handle_task_state_change() {
    emit TaskStatePropertyChanged(task_state());
}

void app_server::setDsnoteApp(QObject *app) {
    m_dsnote_app = app;

    if (!m_dsnote_app) return;

    connect(m_dsnote_app, SIGNAL(active_stt_model_changed()), this,
            SLOT(handle_active_stt_model_change()), Qt::QueuedConnection);
    connect(m_dsnote_app, SIGNAL(active_tts_model_changed()), this,
            SLOT(handle_active_tts_model_change()), Qt::QueuedConnection);
    connect(m_dsnote_app, SIGNAL(service_state_changed()), this,
            SLOT(handle_state_change()), Qt::QueuedConnection);
    connect(m_dsnote_app, SIGNAL(task_state_changed()), this,
            SLOT(handle_task_state_change()), Qt::QueuedConnection);
}

void app_server::InvokeAction(const QString &action_name,
                              const QString &argument) {
    qDebug() << "[dbus app] InvokeAction called:" << action_name;

    emit action_requested(action_name, argument);
}

QStringList app_server::GetSttModelIds() {
    qDebug() << "[dbus app] GetSttModelIds called";

    QStringList list;

    if (!m_dsnote_app) return list;

    QMetaObject::invokeMethod(m_dsnote_app, "available_stt_model_ids",
                              Q_RETURN_ARG(QStringList, list));

    return list;
}

QStringList app_server::GetTtsModelIds() {
    qDebug() << "[dbus app] GetTtsModelIds called";

    QStringList list;

    if (!m_dsnote_app) return list;

    QMetaObject::invokeMethod(m_dsnote_app, "available_tts_model_ids",
                              Q_RETURN_ARG(QStringList, list));

    return list;
}

void app_server::Activate([[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] Activate called";

    emit activate_requested();
}

void app_server::ActivateAction(
    const QString &action_name, [[maybe_unused]] const QVariantList &parameter,
    [[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] ActivateAction called:" << action_name;

    emit action_requested(action_name, parameter.isEmpty()
                                           ? QString{}
                                           : parameter.front().toString());
}

void app_server::Open(const QStringList &uris,
                      [[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] Open called:" << uris;

    QStringList files;
    std::transform(uris.cbegin(), uris.cend(), std::back_inserter(files),
                   [](const auto &uri) { return QUrl{uri}.toLocalFile(); });

    files_to_open(files);
}

QString app_server::active_stt_model_id() const {
    qDebug() << "[dbus app] ActiveSttModelId called";
    return m_dsnote_app ? m_dsnote_app->property("active_stt_model").toString()
                        : QString{};
}

QString app_server::active_tts_model_id() const {
    qDebug() << "[dbus app] ActiveTtsModelId called";
    return m_dsnote_app ? m_dsnote_app->property("active_tts_model").toString()
                        : QString{};
}

int app_server::state() const {
    qDebug() << "[dbus app] State called";
    return m_dsnote_app ? m_dsnote_app->property("state").toInt() : 0;
}

int app_server::task_state() const {
    qDebug() << "[dbus app] TaskState called";
    return m_dsnote_app ? m_dsnote_app->property("task_state").toInt() : 0;
}

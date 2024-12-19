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
    qRegisterMetaType<QList<QStringList>>("QList<QStringList>");
    qDBusRegisterMetaType<QList<QStringList>>();

    auto con = QDBusConnection::sessionBus();

    if (con.registerService(DBUS_SERVICE_NAME) &&
        con.registerObject(DBUS_SERVICE_PATH, this)) {
        qDebug() << "dbus app service registration successul";

        m_pending_request_timer.setSingleShot(true);
        m_pending_request_timer.setInterval(500);  // 1s;
        connect(
            &m_pending_request_timer, &QTimer::timeout, this,
            [this, options, count = 10]() mutable {
                if (state() > 2) {
                    qDebug() << "pending request";
                    request_another_instance(options);
                } else if (count > 0) {
                    --count;
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

    if (options.models_to_print_roles != cmd::role_none ||
        options.active_model_to_print_role != cmd::role_none ||
        options.state_scope_to_print_flag != cmd::scope_none) {
        DsnoteDbusInterface iface(DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                                  QDBusConnection::sessionBus());
        iface.setTimeout(DBUS_TIMEOUT_MS);

        if (options.state_scope_to_print_flag & cmd::scope_general) {
            fmt::print("General state:\n\t{}\n", iface.state());
        }
        if (options.state_scope_to_print_flag & cmd::scope_task) {
            fmt::print("Task state:\n\t{}\n", iface.taskState());
        }

        auto max_id_size = [](const QList<QStringList> &models) {
            return std::accumulate(
                models.cbegin(), models.cend(), 0,
                [](int size, const auto &model) {
                    return std::max(
                        model.size() < 2 ? size : model.at(0).size(), size);
                });
        };

        auto print_models = [](const char *name, int max_id_size,
                               const QList<QStringList> &models) {
            fmt::print("Available {} models: {}\n", name, models.size());

            for (const auto &model : models) {
                if (model.size() < 2) continue;
                fmt::print(fmt::format("\t{{:{}}} \"{{}}\"\n", max_id_size),
                           model.at(0).toStdString(),
                           model.at(1).toStdString());
            }
        };

        auto print_active_model = [](const char *name, int max_id_size,
                                     const QStringList &model) {
            fmt::print("Active {} model:\n", name);
            fmt::print(fmt::format("\t{{:{}}} \"{{}}\"\n", max_id_size),
                       model.size() > 1 ? model.at(0).toStdString() : "-",
                       model.size() > 1 ? model.at(1).toStdString() : "-");
        };

        int g_max_size = 1;

        if ((options.models_to_print_roles & cmd::role_stt) &&
            (options.models_to_print_roles & cmd::role_tts)) {
            QDBusReply<QList<QStringList>> replyStt = iface.GetSttModels();
            QDBusReply<QList<QStringList>> replyTts = iface.GetTtsModels();
            if (replyStt.isValid() && replyTts.isValid()) {
                auto listStt = replyStt.value();
                auto listTts = replyTts.value();
                g_max_size =
                    std::max(max_id_size(listStt), max_id_size(listTts));
                print_models("STT", g_max_size, listStt);
                print_models("TTS", g_max_size, listTts);
            }
        } else if (options.models_to_print_roles & cmd::role_stt) {
            QDBusReply<QList<QStringList>> replyStt = iface.GetSttModels();
            if (replyStt.isValid()) {
                auto listStt = replyStt.value();
                g_max_size = max_id_size(listStt);
                print_models("STT", g_max_size, listStt);
            }
        } else if (options.models_to_print_roles & cmd::role_tts) {
            QDBusReply<QList<QStringList>> replyTts = iface.GetTtsModels();
            if (replyTts.isValid()) {
                auto listStt = replyTts.value();
                g_max_size = max_id_size(listStt);
                print_models("STT", g_max_size, listStt);
            }
        }

        if ((options.active_model_to_print_role & cmd::role_stt) &&
            (options.active_model_to_print_role & cmd::role_tts)) {
            auto modelStt = iface.activeSttModel();
            auto modelTts = iface.activeTtsModel();
            g_max_size = std::max(
                g_max_size,
                std::max(modelStt.size() > 1 ? modelStt.at(0).size() : 1,
                         modelTts.size() > 1 ? modelTts.at(0).size() : 1));
            print_active_model("STT", g_max_size, modelStt);
            print_active_model("TTS", g_max_size, modelTts);
        } else if (options.active_model_to_print_role & cmd::role_stt) {
            auto modelStt = iface.activeSttModel();
            g_max_size = std::max(
                g_max_size, modelStt.size() > 1 ? modelStt.at(0).size() : 1);
            print_active_model("STT", g_max_size, modelStt);
        } else if (options.active_model_to_print_role & cmd::role_tts) {
            auto modelTts = iface.activeTtsModel();
            g_max_size = std::max(
                g_max_size, modelTts.size() > 1 ? modelTts.at(0).size() : 1);
            print_active_model("TTS", g_max_size, modelTts);
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
    emit ActiveSttModelPropertyChanged(active_stt_model());
}

void app_server::handle_active_tts_model_change() {
    emit ActiveTtsModelPropertyChanged(active_tts_model());
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

QList<QStringList> app_server::GetSttModels() {
    qDebug() << "[dbus app] GetSttModels called";

    QList<QStringList> list;

    if (!m_dsnote_app) return list;

    QMetaObject::invokeMethod(m_dsnote_app, "available_stt_models_info",
                              Q_RETURN_ARG(QList<QStringList>, list));

    return list;
}

QList<QStringList> app_server::GetTtsModels() {
    qDebug() << "[dbus app] GetTtsModels called";

    QList<QStringList> list;

    if (!m_dsnote_app) return list;

    QMetaObject::invokeMethod(m_dsnote_app, "available_tts_models_info",
                              Q_RETURN_ARG(QList<QStringList>, list));

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

QStringList app_server::active_stt_model() const {
    qDebug() << "[dbus app] ActiveSttModel called";
    return m_dsnote_app
               ? QStringList{}
                     << m_dsnote_app->property("active_stt_model").toString()
                     << m_dsnote_app->property("active_stt_model_name")
                            .toString()
               : QStringList{};
}

QStringList app_server::active_tts_model() const {
    qDebug() << "[dbus app] ActiveTtsModel called";
    return m_dsnote_app
               ? QStringList{}
                     << m_dsnote_app->property("active_tts_model").toString()
                     << m_dsnote_app->property("active_tts_model_name")
                            .toString()
               : QStringList{};
}

int app_server::state() const {
    qDebug() << "[dbus app] State called";
    return m_dsnote_app ? m_dsnote_app->property("state").toInt() : 0;
}

int app_server::task_state() const {
    qDebug() << "[dbus app] TaskState called";
    return m_dsnote_app ? m_dsnote_app->property("task_state").toInt() : 0;
}

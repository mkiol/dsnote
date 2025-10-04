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

        exit(request_another_instance(options));
    }
}

int app_server::request_another_instance(const cmd::options &options) {
    if (!options.valid) return 1;

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
    }

    DsnoteDbusInterface iface(DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                              QDBusConnection::sessionBus());
    iface.setTimeout(DBUS_TIMEOUT_MS);

    if (!options.action.isEmpty()) {
        qDebug() << "[dbus client] calling InvokeAction on another instance:"
                 << options.action;

        QVariantMap arguments;
        if (!options.model_id.isEmpty()) {
            arguments.insert("model-id", options.model_id);
        }
        if (!options.text.isEmpty()) {
            arguments.insert("text", options.text);
        }
        if (!options.output_file.isEmpty()) {
            arguments.insert("output-file",
                             QFileInfo{options.output_file}.absoluteFilePath());
        }

        auto result = iface.InvokeAction(options.action, arguments);
        result.waitForFinished();

        if (result.isValid()) {
            auto error = static_cast<action_error_code_t>(
                result.value().value("error").toInt());
            qDebug() << "action result error code:" << static_cast<int>(error);
            switch (error) {
                case action_error_code_t::success:
                    break;
                case action_error_code_t::not_enabled:
                    fmt::print(stderr,
                               "Action has failed. Action invocation is "
                               "not enabled in settings.\n");
                    return static_cast<int>(error);
                case action_error_code_t::unknown_name:
                    fmt::print(
                        stderr,
                        "Action has failed. Invalid name of the action.\n");
                    return static_cast<int>(error);
            }
        }
    }

    if (options.models_to_print_roles != cmd::role_none ||
        options.active_model_to_print_role != cmd::role_none ||
        options.state_scope_to_print_flag != cmd::scope_none) {

        if (options.state_scope_to_print_flag & cmd::scope_general) {
            fmt::print("General state:\n\t{}\n", iface.state());
        }
        if (options.state_scope_to_print_flag & cmd::scope_task) {
            fmt::print("Task state:\n\t{}\n", iface.taskState());
        }

        auto max_id_size = [](const QVariantList &models) {
            return std::accumulate(
                models.cbegin(), models.cend(), qsizetype{0}, [](qsizetype size, const auto &m) {
                    auto model = qdbus_cast<QVariantMap>(
                        m.template value<QDBusArgument>());
                    return std::max(model.contains("id")
                                        ? model.value("id").toString().size()
                                        : size,
                                    size);
                });
        };

        auto print_models = [](const char *name, qsizetype max_id_size,
                               const QVariantList &models) {
            fmt::print("Available {} models: {}\n", name, models.size());

            for (const auto &m : models) {
                auto model = qdbus_cast<QVariantMap>(m.value<QDBusArgument>());
                if (!model.contains("id")) continue;
                fmt::print(fmt::format("\t{{:{}}} \"{{}}\"\n",
                                       max_id_size > 0 ? max_id_size : 10),
                           model.value("id").toString().toStdString(),
                           model.value("name").toString().toStdString());
            }
        };

        auto print_active_model = [](const char *name, qsizetype max_id_size,
                                     const QVariantMap &model) {
            fmt::print("Active {} model:\n", name);
            fmt::print(fmt::format("\t{{:{}}} \"{{}}\"\n",
                                   max_id_size > 0 ? max_id_size : 10),
                       model.contains("id")
                           ? model.value("id").toString().toStdString()
                           : "-",
                       model.contains("name")
                           ? model.value("name").toString().toStdString()
                           : "-");
        };

        qsizetype g_max_size = 1;

        if ((options.models_to_print_roles & cmd::role_stt) &&
            (options.models_to_print_roles & cmd::role_tts)) {
            QDBusReply<QVariantList> replyStt = iface.GetSttModels();
            QDBusReply<QVariantList> replyTts = iface.GetTtsModels();
            if (replyStt.isValid() && replyTts.isValid()) {
                auto listStt = replyStt.value();
                auto listTts = replyTts.value();
                g_max_size =
                    std::max(max_id_size(listStt), max_id_size(listTts));
                print_models("STT", g_max_size, listStt);
                print_models("TTS", g_max_size, listTts);
            }
        } else if (options.models_to_print_roles & cmd::role_stt) {
            QDBusReply<QVariantList> replyStt = iface.GetSttModels();
            if (replyStt.isValid()) {
                auto listStt = replyStt.value();
                g_max_size = max_id_size(listStt);
                print_models("STT", g_max_size, listStt);
            }
        } else if (options.models_to_print_roles & cmd::role_tts) {
            QDBusReply<QVariantList> replyTts = iface.GetTtsModels();
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
            g_max_size =
                std::max(g_max_size,
                         std::max(modelStt.contains("id")
                                      ? modelStt.value("id").toString().size()
                                      : 1,
                                  modelTts.contains("id")
                                      ? modelTts.value("id").toString().size()
                                      : 1));
            print_active_model("STT", g_max_size, modelStt);
            print_active_model("TTS", g_max_size, modelTts);
        } else if (options.active_model_to_print_role & cmd::role_stt) {
            auto modelStt = iface.activeSttModel();
            g_max_size = std::max(g_max_size,
                                  modelStt.contains("id")
                                      ? modelStt.value("id").toString().size()
                                      : 1);
            print_active_model("STT", g_max_size, modelStt);
        } else if (options.active_model_to_print_role & cmd::role_tts) {
            auto modelTts = iface.activeTtsModel();
            g_max_size = std::max(g_max_size,
                                  modelTts.contains("id")
                                      ? modelTts.value("id").toString().size()
                                      : 1);
            print_active_model("TTS", g_max_size, modelTts);
        }
    }

    return 0;
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

QVariantMap app_server::InvokeAction(const QString &action_name,
                                     const QVariantMap &arguments) {
    qDebug() << "[dbus app] InvokeAction called:" << action_name;

    return invoke_action(action_name, arguments);
}

QVariantMap app_server::invoke_action(const QString &action_id,
                                      const QVariantMap &arguments) {
    QVariantMap map;

    QMetaObject::invokeMethod(
        m_dsnote_app, "execute_action_id", Q_RETURN_ARG(QVariantMap, map),
        Q_ARG(QString, action_id), Q_ARG(QVariantMap, arguments),
        Q_ARG(bool, false));

    return map;
}

QVariantList app_server::GetSttModels() {
    qDebug() << "[dbus app] GetSttModels called";

    QVariantList list;

    if (m_dsnote_app) {
        QMetaObject::invokeMethod(m_dsnote_app, "available_stt_models_info",
                                  Q_RETURN_ARG(QVariantList, list));
    }

    return list;
}

QVariantList app_server::GetTtsModels() {
    qDebug() << "[dbus app] GetTtsModels called";

    QVariantList list;

    if (!m_dsnote_app) return list;

    QMetaObject::invokeMethod(m_dsnote_app, "available_tts_models_info",
                              Q_RETURN_ARG(QVariantList, list));

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

    QVariantMap argumets;
    if (parameter.size() > 2) argumets.insert("output-file", parameter.at(2));
    if (parameter.size() > 1) argumets.insert("text", parameter.at(1));
    if (parameter.size() > 0) argumets.insert("model-id", parameter.at(0));

    invoke_action(action_name, argumets);
}

void app_server::Open(const QStringList &uris,
                      [[maybe_unused]] const QVariantMap &platform_data) {
    qDebug() << "[dbus app] Open called:" << uris;

    QStringList files;
    std::transform(uris.cbegin(), uris.cend(), std::back_inserter(files),
                   [](const auto &uri) { return QUrl{uri}.toLocalFile(); });

    files_to_open(files);
}

QVariantMap app_server::active_stt_model() const {
    qDebug() << "[dbus app] ActiveSttModel called";

    QVariantMap map;

    if (m_dsnote_app) {
        map.insert(QStringLiteral("id"),
                   m_dsnote_app->property("active_stt_model").toString());
        map.insert(QStringLiteral("name"),
                   m_dsnote_app->property("active_stt_model_name").toString());
    }

    return map;
}

QVariantMap app_server::active_tts_model() const {
    qDebug() << "[dbus app] ActiveTtsModel called";

    QVariantMap map;

    if (m_dsnote_app) {
        map.insert(QStringLiteral("id"),
                   m_dsnote_app->property("active_tts_model").toString());
        map.insert(QStringLiteral("name"),
                   m_dsnote_app->property("active_tts_model_name").toString());
    }

    return map;
}

int app_server::state() const {
    qDebug() << "[dbus app] State called";
    return m_dsnote_app ? m_dsnote_app->property("state").toInt() : 0;
}

int app_server::task_state() const {
    qDebug() << "[dbus app] TaskState called";
    return m_dsnote_app ? m_dsnote_app->property("task_state").toInt() : 0;
}

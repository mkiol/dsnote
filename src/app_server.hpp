/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef APP_SERVER_HPP
#define APP_SERVER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "cmd_options.hpp"
#include "config.h"
#include "dbus_application_adaptor.h"
#include "dbus_dsnote_adaptor.h"

class app_server : public QObject {
    Q_OBJECT

    // bus dsnote api
    Q_PROPERTY(QVariantMap ActiveSttModel READ active_stt_model CONSTANT)
    Q_PROPERTY(QVariantMap ActiveTtsModel READ active_tts_model CONSTANT)
    Q_PROPERTY(int State READ state CONSTANT)
    Q_PROPERTY(int TaskState READ task_state CONSTANT)

   public:
    explicit app_server(const cmd::options &options, QObject *parent = nullptr);
    Q_INVOKABLE void setDsnoteApp(QObject *app);

    // dbus application api
    Q_INVOKABLE void Activate(const QVariantMap &platform_data);
    Q_INVOKABLE void ActivateAction(const QString &action_name,
                                    const QVariantList &parameter,
                                    const QVariantMap &platform_data);
    Q_INVOKABLE void Open(const QStringList &uris,
                          const QVariantMap &platform_data);
    // dbus dsnote api
    Q_INVOKABLE QVariantMap InvokeAction(const QString &action_name,
                                         const QVariantMap &arguments);
    Q_INVOKABLE QVariantList GetSttModels();
    Q_INVOKABLE QVariantList GetTtsModels();

   signals:
    void activate_requested();
    void files_to_open_requested(const QStringList &files);
    // dbus dsnote api
    void ActiveSttModelPropertyChanged(const QVariantMap &id);
    void ActiveTtsModelPropertyChanged(const QVariantMap &id);
    void StatePropertyChanged(int state);
    void TaskStatePropertyChanged(int state);

   private:
    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_APP_SERVICE)};
    inline static const QString DBUS_SERVICE_PATH{
        QStringLiteral(APP_DBUS_APP_PATH)};
    static const int DBUS_TIMEOUT_MS = 10000;  // 10s

    enum class action_error_code_t {
        success = 0,
        not_enabled = 10,
        unknown_name = 99
    };

    ApplicationAdaptor m_dbus_application_adaptor;
    DsnoteAdaptor m_dbus_dsnote_adaptor;
    QObject* m_dsnote_app = nullptr;
    QTimer m_pending_request_timer;

    void files_to_open(const QStringList &files);
    int request_another_instance(const cmd::options &options);
    QVariantMap active_stt_model() const;
    QVariantMap active_tts_model() const;
    int state() const;
    int task_state() const;
    QVariantMap invoke_action(const QString &action_id,
                              const QVariantMap &arguments);
   private Q_SLOTS:
    void handle_active_stt_model_change();
    void handle_active_tts_model_change();
    void handle_state_change();
    void handle_task_state_change();
};

#endif  // APP_SERVER_HPP

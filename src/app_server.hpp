/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef APP_SERVER_HPP
#define APP_SERVER_HPP

#include <QList>
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
    Q_PROPERTY(QString ActiveSttModelId READ active_stt_model_id CONSTANT)
    Q_PROPERTY(QString ActiveTtsModelId READ active_tts_model_id CONSTANT)
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
    Q_INVOKABLE void InvokeAction(const QString &action_name,
                                  const QString &argument);
    Q_INVOKABLE QStringList GetSttModelIds();
    Q_INVOKABLE QStringList GetTtsModelIds();

   signals:
    void activate_requested();
    void action_requested(QString action_name, QString action_extra);
    void files_to_open_requested(const QStringList &files);
    // dbus dsnote api
    void ActiveSttModelIdPropertyChanged(const QString &id);
    void ActiveTtsModelIdPropertyChanged(const QString &id);
    void StatePropertyChanged(int state);
    void TaskStatePropertyChanged(int state);

   private:
    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_APP_SERVICE)};
    inline static const QString DBUS_SERVICE_PATH{
        QStringLiteral(APP_DBUS_APP_PATH)};
    static const int DBUS_TIMEOUT_MS = 10000;  // 10s

    ApplicationAdaptor m_dbus_application_adaptor;
    DsnoteAdaptor m_dbus_dsnote_adaptor;
    QObject* m_dsnote_app = nullptr;
    QTimer m_pending_request_timer;

    void files_to_open(const QStringList &files);
    void request_another_instance(const cmd::options &options);
    QString active_stt_model_id() const;
    QString active_tts_model_id() const;
    int state() const;
    int task_state() const;
   private Q_SLOTS:
    void handle_active_stt_model_change();
    void handle_active_tts_model_change();
    void handle_state_change();
    void handle_task_state_change();
};

#endif  // APP_SERVER_HPP

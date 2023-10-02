/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
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
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "config.h"

#ifdef USE_SFOS
#include "dbus_application_adaptor.h"
#else
#include <KDBusService>
#endif

class app_server : public QObject {
    Q_OBJECT
   public:
    explicit app_server(QObject *parent = nullptr);

   signals:
    void activate_requested();
    void action_requested(QString action_name);
    void files_to_open_requested(const QStringList &files);

   private:
    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_APP_SERVICE)};
    inline static const QString DBUS_SERVICE_PATH{
        QStringLiteral(APP_DBUS_APP_PATH)};

#ifdef USE_SFOS
    ApplicationAdaptor m_dbus_service;
#else
    KDBusService m_dbus_service;
#endif

#ifdef USE_SFOS
    Q_INVOKABLE void Activate(const QVariantMap &platform_data);
    Q_INVOKABLE void ActivateAction(const QString &action_name,
                                    const QVariantList &parameter,
                                    const QVariantMap &platform_data);
    Q_INVOKABLE void Open(const QStringList &uris,
                          const QVariantMap &platform_data);
#else
    void activateRequested(const QStringList& arguments,
                           const QString& workingDirectory);
    void activateActionRequested(const QString& actionName,
                                 const QVariant& parameter);
    void openRequested(const QList<QUrl>& uris);
#endif
    void files_to_open(const QStringList &files);
};

#endif  // APP_SERVER_HPP

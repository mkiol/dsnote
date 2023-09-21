/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
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
#include <QVariantList>
#include <QVariantMap>

#include "config.h"
#include "dbus_application_adaptor.h"

class app_server : public QObject {
    Q_OBJECT
   public:
    explicit app_server(QObject *parent = nullptr);

   signals:
    void activate_requested();
    void files_to_open_requested(const QStringList &files);

   private:
    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_APP_SERVICE)};
    inline static const QString DBUS_SERVICE_PATH{
        QStringLiteral(APP_DBUS_APP_PATH)};

    ApplicationAdaptor m_dbus_service_adaptor;

    // DBus
    Q_INVOKABLE void Activate(const QVariantMap &platform_data);
    Q_INVOKABLE void ActivateAction(const QString &action_name,
                                    const QVariantList &parameter,
                                    const QVariantMap &platform_data);
    Q_INVOKABLE void Open(const QStringList &uris,
                          const QVariantMap &platform_data);
};

#endif  // APP_SERVER_HPP

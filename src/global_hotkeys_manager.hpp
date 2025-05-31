/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GLOBAL_HOTKEYS_MANAGER_H
#define GLOBAL_HOTKEYS_MANAGER_H

#include <QDBusObjectPath>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "settings.h"

#ifdef USE_X11_FEATURES
#include <qhotkey.h>
#endif

#include "dbus_portal_globalshortcuts_inf.h"

class global_hotkeys_manager : public QObject {
    Q_OBJECT

   public:
    explicit global_hotkeys_manager(QObject* parent = nullptr);
    bool is_supported() const;
    bool is_x11_supported() const;
    bool is_portal_supported() const;
    void set_portal_bindings();
    void reset_portal_connection();

   Q_SIGNALS:
    void hotkey_activated(const QString& action_id,
                          const QVariantMap& arguments);

   private:
    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral("org.freedesktop.portal.Desktop")};
    inline static const QString DBUS_SERVICE_PATH{
        QStringLiteral("/org/freedesktop/portal/desktop")};
    static const int DBUS_TIMEOUT_MS = 10000;  // 10s

    uint m_portal_request_token_counter = 0;

    OrgFreedesktopPortalGlobalShortcutsInterface m_portal_inf;
    QDBusObjectPath m_portal_session;
    QMetaObject::Connection m_portal_activated_conn;
#ifdef USE_X11_FEATURES
    struct x11_hotkeys_t {
#define X(name, id, desc, key) QHotkey name;
        HOTKEY_TABLE
#undef X
    };
    x11_hotkeys_t m_x11_hotkeys;
    bool m_force_bind = false;

    void enable_x11();
    void disable_x11();
    void handle_x11_activated();
#endif

    void create_portal_session(bool force_bind = false);
    void fetch_portal_shortcuts();
    QString get_portal_request_token();
    void enable_or_disable();
    void enable_portal();
    void disable_portal();
    void handle_portal_activated(const QDBusObjectPath& session_handle,
                                 const QString& action_id, qulonglong timestamp,
                                 const QVariantMap& options);

   public Q_SLOTS:
    void handle_create_session_response(uint res, const QVariantMap& results);
    void handle_list_shortcuts_response(uint code, const QVariantMap& results);
    void handle_bind_shortcuts_response(uint code, const QVariantMap& results);
};

#endif  // GLOBAL_HOTKEYS_MANAGER_H

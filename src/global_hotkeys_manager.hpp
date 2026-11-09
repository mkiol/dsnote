/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GLOBAL_HOTKEYS_MANAGER_H
#define GLOBAL_HOTKEYS_MANAGER_H

#include <QDBusObjectPath>
#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <utility>

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
    enum class hotkey_t {
        none,
#define X(xname, ...) xname,
        HOTKEY_TABLE
#undef X
    };

    enum class hotkey_type_t { stt, tts, other };

    struct hotkey_info_t {
        hotkey_t hotkey = hotkey_t::none;
        QString action_name;
        hotkey_type_t type = hotkey_type_t::other;
        hotkey_t toggle = hotkey_t::none;
        bool trigger_on_deactivate = false;
    };

    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral("org.freedesktop.portal.Desktop")};
    inline static const QString DBUS_SERVICE_PATH{
        QStringLiteral("/org/freedesktop/portal/desktop")};
    static const int DBUS_TIMEOUT_MS = 10000;  // 10s

    uint m_portal_request_token_counter = 0;

    OrgFreedesktopPortalGlobalShortcutsInterface m_portal_inf;
    QDBusObjectPath m_portal_session;
    QMetaObject::Connection m_portal_activated_conn;
    QMetaObject::Connection m_portal_deactivated_conn;
    QHash<hotkey_t, hotkey_info_t> m_hotkey_map;
    QHash<QString, hotkey_t> m_action_to_hotkey_map;
#ifdef USE_X11_FEATURES
    struct x11_hotkeys_t {
#define X(xname, ...) QHotkey xname;
        HOTKEY_TABLE
#undef X
    };
    x11_hotkeys_t m_x11_hotkeys;

    void enable_x11();
    void disable_x11();
    void handle_x11_activated();
    void handle_x11_released();
#endif

    void create_portal_session();
    void fetch_portal_shortcuts();
    QString get_portal_request_token();
    void enable_or_disable();
    void enable_portal();
    void disable_portal();
    std::pair<QString, bool> map_action_to_final_action(
        const QString& action_id, bool activated) const;
    void handle_portal_action(const QString& action_id, bool activated);
    void handle_portal_activated(const QDBusObjectPath& session_handle,
                                 const QString& action_id, qulonglong timestamp,
                                 const QVariantMap& options);
    void handle_portal_deactivated(const QDBusObjectPath& session_handle,
                                   const QString& action_id,
                                   qulonglong timestamp,
                                   const QVariantMap& options);

   public Q_SLOTS:
    void handle_create_session_response(uint res, const QVariantMap& results);
    void handle_list_shortcuts_response(uint code, const QVariantMap& results);
    void handle_bind_shortcuts_response(uint code, const QVariantMap& results);
};

#endif  // GLOBAL_HOTKEYS_MANAGER_H

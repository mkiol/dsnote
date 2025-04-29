/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "global_hotkeys_manager.hpp"

#include <QDebug>
#include <QRandomGenerator>

#include "config.h"
#include "dbus_portal_request_inf.h"
#include "logger.hpp"
#include "qtlogger.hpp"
#include "settings.h"

using PortalShortcut = QPair<QString, QVariantMap>;
using PortalShortcuts = QList<PortalShortcut>;

global_hotkeys_manager::global_hotkeys_manager(QObject *parent)
    : QObject{parent},
      m_portal_inf{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                   QDBusConnection::sessionBus()} {
    qDBusRegisterMetaType<PortalShortcuts>();
    qDBusRegisterMetaType<QPair<QString, QVariantMap>>();
    m_portal_inf.setTimeout(DBUS_TIMEOUT_MS);

    connect(settings::instance(), &settings::hotkeys_enabled_changed, this,
            &global_hotkeys_manager::enable_or_disable);
    connect(settings::instance(), &settings::hotkeys_type_changed, this,
            &global_hotkeys_manager::enable_or_disable);
    connect(settings::instance(), &settings::hotkeys_changed, this,
            &global_hotkeys_manager::enable_or_disable);

    // try to create portal global shortcuts session
    create_portal_session();
}

bool global_hotkeys_manager::is_supported() const {
    return is_x11_supported() || is_portal_supported();
}

bool global_hotkeys_manager::is_x11_supported() const {
#ifdef USE_X11_FEATURES
    return settings::instance()->is_xcb();
#else
    return false;
#endif
}

bool global_hotkeys_manager::is_portal_supported() const {
    return !m_portal_session.path().isEmpty();
}

void global_hotkeys_manager::enable_or_disable() {
    disable_portal();
#ifdef USE_X11_FEATURES
    disable_x11();
    if (settings::instance()->hotkeys_enabled()) {
        switch (settings::instance()->hotkeys_type()) {
            case settings::hotkeys_type_t::HotkeysTypeX11:
                enable_x11();
                break;
            case settings::hotkeys_type_t::HotkeysTypePortal:
                enable_portal();
                break;
        }
    }
#else
    if (settings::instance()->hotkeys_enabled()) {
        switch (settings::instance()->hotkeys_type()) {
            case settings::hotkeys_type_t::HotkeysTypePortal:
                enable_portal();
                break;
        }
    }
#endif
}

void global_hotkeys_manager::enable_portal() {
    if (!is_portal_supported()) {
        LOGW("can't enable global hotkeys becuase portal is not supported");
        return;
    }

    m_portal_activated_conn = connect(
        &m_portal_inf, &OrgFreedesktopPortalGlobalShortcutsInterface::Activated,
        this, &global_hotkeys_manager::handle_portal_activated);

    fetch_portal_shortcuts();
}

void global_hotkeys_manager::disable_portal() {
    disconnect(m_portal_activated_conn);
}

void global_hotkeys_manager::handle_portal_activated(
    [[maybe_unused]] const QDBusObjectPath &session_handle,
    const QString &action_id, [[maybe_unused]] qulonglong timestamp,
    [[maybe_unused]] const QVariantMap &options) {
    LOGD("portal hotkey activated: " << action_id);
    emit hotkey_activated(action_id, {});
}

void global_hotkeys_manager::create_portal_session(bool force_bind) {
    LOGD("[dbus] call CreateSession");

    auto handle_token = QStringLiteral(APP_ID "_%1")
                            .arg(QRandomGenerator::global()->generate());
    m_force_bind = force_bind;

    auto reply = m_portal_inf.CreateSession({
        {QLatin1String("session_handle_token"), handle_token},
        {QLatin1String("handle_token"), handle_token},
    });

    reply.waitForFinished();

    if (reply.isError()) {
        LOGW("can't connect to portal global shortcuts service: "
             << reply.error().message());
        return;
    }

    LOGD("connecting to portal service: " << reply.value().path());

    QDBusConnection::sessionBus().connect(
        DBUS_SERVICE_NAME, reply.value().path(),
        OrgFreedesktopPortalRequestInterface::staticInterfaceName(),
        QLatin1String("Response"), this,
        SLOT(handle_create_session_response(uint, QVariantMap)));
}

void global_hotkeys_manager::handle_create_session_response(
    uint res, const QVariantMap &results) {
    if (res != 0) {
        LOGW("failed to create portal session: " << res);
        return;
    }

    m_portal_session = QDBusObjectPath{results["session_handle"].toString()};

    LOGD("portal session created successfully");

    enable_or_disable();
}

void global_hotkeys_manager::fetch_portal_shortcuts() {
    LOGD("[dbus] call ListShortcuts");

    auto reply = m_portal_inf.ListShortcuts(m_portal_session, {});
    reply.waitForFinished();
    if (reply.isError()) {
        LOGW("failed to call ListShortcuts: " << reply.error().message());
        return;
    }

    auto *req = new OrgFreedesktopPortalRequestInterface(
        DBUS_SERVICE_NAME, reply.value().path(), QDBusConnection::sessionBus(),
        this);

    connect(req, &OrgFreedesktopPortalRequestInterface::Response, this,
            &global_hotkeys_manager::handle_list_shortcuts_response);
    connect(req, &OrgFreedesktopPortalRequestInterface::Response, req,
            &QObject::deleteLater);
}

void global_hotkeys_manager::handle_list_shortcuts_response(
    uint code, const QVariantMap &results) {
    if (code != 0) {
        LOGW("failed to get the list of portal hotkeys: " << code);
        return;
    }

    if (!results.contains("shortcuts")) {
        LOGW("no shortcuts in portal reply");
        return;
    }

    PortalShortcuts s;
    const auto arg = results["shortcuts"].value<QDBusArgument>();
    arg >> s;

    for (auto it = s.cbegin(), it_end = s.cend(); it != it_end; ++it) {
        LOGD("portal hotkey: " << it->first << " "
                               << it->second["description"].toString() << " "
                               << it->second["trigger_description"].toString());
    }

    if (s.isEmpty()) {
        set_portal_bindings();
        m_force_bind = false;
        return;
    }

    QStringList shortcuts_id_list;
#define X(name, id, desc, key) shortcuts_id_list.push_back(id);
    HOTKEY_TABLE
#undef X

    bool all_shortcuts_configured = [&]() {
        for (auto it = s.cbegin(), it_end = s.cend(); it != it_end; ++it) {
            if (!shortcuts_id_list.contains(it->first)) {
                return false;
            }
        }
        return true;
    }();

    if (all_shortcuts_configured && !m_force_bind) {
        LOGD("portal global shortcuts already configured");
    } else {
        set_portal_bindings();
    }

    m_force_bind = false;
}

void global_hotkeys_manager::handle_bind_shortcuts_response(
    uint code, [[maybe_unused]] const QVariantMap &results) {
    if (code != 0) {
        LOGW("failed to bind portal shortcuts: " << code);
        return;
    }
}

QString global_hotkeys_manager::get_portal_request_token() {
    m_portal_request_token_counter += 1;
    return QString(APP_ID "_request_token_%1")
        .arg(m_portal_request_token_counter);
}

void global_hotkeys_manager::set_portal_bindings() {
    if (!is_portal_supported()) {
        LOGW("portal not supported");
        return;
    }

    PortalShortcuts shortcuts;

    auto key_to_xdg_key = [](QString key) {
        key.replace("shift", "SHIFT", Qt::CaseInsensitive);
        key.replace("caps", "CAPS", Qt::CaseInsensitive);
        key.replace("ctrl", "CTRL", Qt::CaseInsensitive);
        key.replace("num", "NUM", Qt::CaseInsensitive);
        key.replace("alt", "ALT", Qt::CaseInsensitive);
        key.replace("logo", "LOGO", Qt::CaseInsensitive);
        key.replace(",", "comma");
        key.replace(".", "period");
        key.replace("-", "minus");
        key.replace("=", "equal");
        key.replace(":", "colon");
        key.replace(";", "semicolon");
        key.replace("?", "question");
        key.replace("\\", "backslash");
        key.replace("/", "slash");
        return key;
    };

#define X(name, id, desc, key)                                             \
    {                                                                      \
        PortalShortcut shortcut;                                           \
        QVariantMap shortcut_options;                                      \
        shortcut.first = id;                                               \
        shortcut_options.insert("description", desc);                      \
        shortcut_options.insert("preferred_trigger", key_to_xdg_key(key)); \
        shortcut.second = shortcut_options;                                \
        shortcuts.append(shortcut);                                        \
    }
    HOTKEY_TABLE
#undef X

    LOGD("[dbus] call BindShortcuts");

    auto reply = m_portal_inf.BindShortcuts(
        m_portal_session, shortcuts, /*parent_window*/ {},
        {{"handle_token", get_portal_request_token()}});
    reply.waitForFinished();
    if (reply.isError()) {
        LOGW("failed to call BindShortcuts: " << reply.error().message());
        return;
    }

    auto req = new OrgFreedesktopPortalRequestInterface(
        DBUS_SERVICE_NAME, reply.value().path(), QDBusConnection::sessionBus(),
        this);

    connect(req, &OrgFreedesktopPortalRequestInterface::Response, this,
            &global_hotkeys_manager::handle_bind_shortcuts_response);
    connect(req, &OrgFreedesktopPortalRequestInterface::Response, req,
            &QObject::deleteLater);
}

#ifdef USE_X11_FEATURES

void global_hotkeys_manager::disable_x11() {
#define X(name, id, desc, key) QObject::disconnect(&m_x11_hotkeys.name);
    HOTKEY_TABLE
#undef X
#define X(name, id, desc, key) m_x11_hotkeys.name.setRegistered(false);
    HOTKEY_TABLE
#undef X
}

void global_hotkeys_manager::enable_x11() {
    if (!is_x11_supported()) {
        LOGW("can't enable global hotkeys becuase x11 is not supported");
        return;
    }

    auto *s = settings::instance();

#define X(name, id, desc, keyw)                                               \
    if (!s->hotkey_##name().isEmpty()) {                                      \
        if (!m_x11_hotkeys.name.setShortcut(QKeySequence{s->hotkey_##name()}, \
                                            true)) {                          \
            LOGW(                                                             \
                "failed to register global hotkey, perhaps it is "            \
                "already in use: "                                            \
                << s->hotkey_##name());                                       \
        }                                                                     \
        m_x11_hotkeys.name.setProperty("action", id);                         \
        QObject::connect(&m_x11_hotkeys.name, &QHotkey::activated, this,      \
                         &global_hotkeys_manager::handle_x11_activated);      \
    }

    HOTKEY_TABLE
#undef X
}

void global_hotkeys_manager::handle_x11_activated() {
    auto action_id = sender()->property("action").toString();
    LOGD("x11 hotkey activated: " << action_id);
    emit hotkey_activated(action_id, {});
}

void global_hotkeys_manager::reset_portal_connection() {
    if (!is_portal_supported()) {
        LOGW("portal not supported");
        return;
    }

    disable_portal();

    create_portal_session(/*force_bind=*/true);
}

#endif  // USE_X11_FEATURES

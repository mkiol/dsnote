/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "global_hotkeys_manager.hpp"

#include <QDebug>
#include <QGuiApplication>
#include <QRandomGenerator>

#include "config.h"
#include "dbus_portal_request_inf.h"
#include "logger.hpp"
#include "qtlogger.hpp"  // don't remove it
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

#define X(xname, xname_str, xdesc, xkey, xtype, xdeact, xname_toggle, ...)     \
    m_hotkey_map.insert(hotkey_t::xname, {                                     \
                                             .hotkey = hotkey_t::xname,        \
                                             .action_name = xname_str,         \
                                             .type = hotkey_type_t::xtype,     \
                                             .toggle = hotkey_t::xname_toggle, \
                                             .trigger_on_deactivate = xdeact,  \
                                         });                                   \
    m_action_to_hotkey_map.insert(QStringLiteral(xname_str), hotkey_t::xname);
    HOTKEY_TABLE
#undef X

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
        enable_portal();
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
    m_portal_deactivated_conn =
        connect(&m_portal_inf,
                &OrgFreedesktopPortalGlobalShortcutsInterface::Deactivated,
                this, &global_hotkeys_manager::handle_portal_deactivated);

    fetch_portal_shortcuts();
}

void global_hotkeys_manager::disable_portal() {
    disconnect(m_portal_activated_conn);
    disconnect(m_portal_deactivated_conn);
}

std::pair<QString, /*toggle enabled*/ bool>
global_hotkeys_manager::map_action_to_final_action(const QString &action_id,
                                                   bool activated) const {
    std::pair<QString, bool> final_action_id{{}, false};

    auto it = m_action_to_hotkey_map.find(action_id);
    if (it == m_action_to_hotkey_map.cend()) {
        LOGW("unknown action-id: " << action_id);
        return final_action_id;
    }

    auto it_info = m_hotkey_map.find(it.value());
    if (it_info == m_hotkey_map.cend()) {
        LOGE("unknown hotkey: action-id=" << action_id << ", hotkey="
                                          << static_cast<int>(it.value()));
        return final_action_id;
    }

    const auto &info = it_info.value();

    auto hotkey_toggle = hotkey_t::none;
    if (info.toggle != hotkey_t::none) {
        switch (info.type) {
            case hotkey_type_t::stt:
                if (settings::instance()->hotkey_stt_toggle_on_key_release()) {
                    hotkey_toggle = info.toggle;
                }
                break;
            case hotkey_type_t::tts:
                if (settings::instance()->hotkey_tts_toggle_on_key_release()) {
                    hotkey_toggle = info.toggle;
                }
                break;
            case hotkey_type_t::other:
                break;
        }
    }

    auto hotkey_to_trigger = hotkey_t::none;

    if (activated) {
        // trigger only if hotkey should not be triggered on deactivation
        if (!info.trigger_on_deactivate) {
            hotkey_to_trigger = info.hotkey;
        }
    } else {  // deactivate
        hotkey_to_trigger = hotkey_toggle;

        // trigger only if hotkey should be triggered on deactivation
        if (hotkey_to_trigger == hotkey_t::none && info.trigger_on_deactivate) {
            hotkey_to_trigger = info.hotkey;
        }
    }

    if (hotkey_to_trigger == hotkey_t::none) {
        // do nothing
        return final_action_id;
    }

    it_info = m_hotkey_map.find(hotkey_to_trigger);
    if (it_info == m_hotkey_map.cend()) {
        LOGE("unknown hotkey after mapping: action-id="
             << action_id
             << ", hotkey=" << static_cast<int>(hotkey_to_trigger));
        return final_action_id;
    }

    final_action_id.first = it_info.value().action_name;
    final_action_id.second = hotkey_toggle != hotkey_t::none;

    return final_action_id;
}

void global_hotkeys_manager::handle_portal_action(const QString &action_id,
                                                  bool activated) {
    auto [final_action_id, toggle_enabled] =
        map_action_to_final_action(action_id, activated);
    if (final_action_id.isEmpty()) {
        // do nothing
        return;
    }

    LOGD("portal hotkey to trigger: " << action_id << " => " << final_action_id
                                      << ", toggle-enabled=" << toggle_enabled);

    emit hotkey_activated(final_action_id,
                          {{"toggle-on-release", toggle_enabled}});
}

void global_hotkeys_manager::handle_portal_activated(
    [[maybe_unused]] const QDBusObjectPath &session_handle,
    const QString &action_id, [[maybe_unused]] qulonglong timestamp,
    [[maybe_unused]] const QVariantMap &options) {
    handle_portal_action(action_id, /*activated=*/true);
}

void global_hotkeys_manager::handle_portal_deactivated(
    [[maybe_unused]] const QDBusObjectPath &session_handle,
    const QString &action_id, [[maybe_unused]] qulonglong timestamp,
    [[maybe_unused]] const QVariantMap &options) {
    handle_portal_action(action_id, /*activated=*/false);
}

static void register_app_id_in_portal() {
    QDBusInterface registry(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.host.portal.Registry"),
        QDBusConnection::sessionBus());
    LOGD("[dbus] call Register");
    QDBusReply<void> reply =
        registry.call(QStringLiteral("Register"),
                      QGuiApplication::desktopFileName(), QVariantMap{});
    if (!reply.isValid()) {
        LOGW("portal register failed: " << reply.error().message());
    }
}

void global_hotkeys_manager::create_portal_session() {
    LOGD("[dbus] call CreateSession");

    // register app-id explicitly
    // this is needed only when you start from terminal
    register_app_id_in_portal();

    auto handle_token = QStringLiteral(APP_ID "_%1")
                            .arg(QRandomGenerator::global()->generate());
    auto reply = m_portal_inf.CreateSession({
        {QLatin1String("session_handle_token"), handle_token},
        {QLatin1String("handle_token"), handle_token},
    });

    reply.waitForFinished();

    if (reply.isError()) {
        LOGW("can't connect to portal global shortcuts service: "
             << reply.error().message());

        // enable x11 key bindings if configured
        enable_or_disable();

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
    } else {
        m_portal_session =
            QDBusObjectPath{results["session_handle"].toString()};
        LOGD("portal session created successfully");
    }

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
        return;
    }

    QStringList shortcuts_id_list;
#define X(xname, xname_str, ...) shortcuts_id_list.push_back(xname_str);
    HOTKEY_TABLE
#undef X

    bool all_shortcuts_configured = [&]() {
        // ensure every id from HOTKEY_TABLE is present
        // in portal shortcuts list.
        for (const auto &id : shortcuts_id_list) {
            bool found = false;
            for (auto it = s.cbegin(), it_end = s.cend(); it != it_end; ++it) {
                if (it->first == id) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }();

    if (all_shortcuts_configured) {
        LOGD("portal global shortcuts already configured");
    }

    set_portal_bindings();
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

#define X(xname, xname_str, xdesc, xkey, ...)                               \
    {                                                                       \
        PortalShortcut shortcut;                                            \
        QVariantMap shortcut_options;                                       \
        shortcut.first = xname_str;                                         \
        shortcut_options.insert("description", xdesc);                      \
        shortcut_options.insert("preferred_trigger", key_to_xdg_key(xkey)); \
        shortcut.second = shortcut_options;                                 \
        shortcuts.append(shortcut);                                         \
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

    auto *req = new OrgFreedesktopPortalRequestInterface(
        DBUS_SERVICE_NAME, reply.value().path(), QDBusConnection::sessionBus(),
        this);

    connect(req, &OrgFreedesktopPortalRequestInterface::Response, this,
            &global_hotkeys_manager::handle_bind_shortcuts_response);
    connect(req, &OrgFreedesktopPortalRequestInterface::Response, req,
            &QObject::deleteLater);
}

void global_hotkeys_manager::reset_portal_connection() {
    if (!is_portal_supported()) {
        LOGW("portal not supported");
        return;
    }

    disable_portal();

    create_portal_session();
}

#ifdef USE_X11_FEATURES

void global_hotkeys_manager::disable_x11() {
#define X(xname, ...) QObject::disconnect(&m_x11_hotkeys.xname);
    HOTKEY_TABLE
#undef X
#define X(xname, ...) m_x11_hotkeys.xname.setRegistered(false);
    HOTKEY_TABLE
#undef X
}

void global_hotkeys_manager::enable_x11() {
    if (!is_x11_supported()) {
        LOGW("can't enable global hotkeys becuase x11 is not supported");
        return;
    }

    auto *s = settings::instance();

#define X(xname, xname_str, ...)                                          \
    if (!s->hotkey_##xname().isEmpty()) {                                 \
        if (!m_x11_hotkeys.xname.setShortcut(                             \
                QKeySequence{s->hotkey_##xname()}, true)) {               \
            LOGW(                                                         \
                "failed to register global hotkey, perhaps it is "        \
                "already in use: "                                        \
                << s->hotkey_##xname());                                  \
        }                                                                 \
        m_x11_hotkeys.xname.setProperty("action", xname_str);             \
        QObject::connect(&m_x11_hotkeys.xname, &QHotkey::released, this,  \
                         &global_hotkeys_manager::handle_x11_activated);  \
        QObject::connect(&m_x11_hotkeys.xname, &QHotkey::activated, this, \
                         &global_hotkeys_manager::handle_x11_activated);  \
    }

    HOTKEY_TABLE
#undef X
}

void global_hotkeys_manager::handle_x11_activated() {
    auto action_id = sender()->property("action").toString();

    auto [final_action_id, toggle_enabled] =
        map_action_to_final_action(action_id, /*activated=*/true);
    if (final_action_id.isEmpty()) {
        // do nothing
        return;
    }

    LOGD("x11 hotkey activated: " << action_id << " => " << final_action_id
                                  << ", toggle-enabled=" << toggle_enabled);
    emit hotkey_activated(final_action_id,
                          {{"toggle-on-release", toggle_enabled}});
}

void global_hotkeys_manager::handle_x11_released() {
    auto action_id = sender()->property("action").toString();

    auto [final_action_id, toggle_enabled] =
        map_action_to_final_action(action_id, /*activated=*/false);
    if (final_action_id.isEmpty()) {
        // do nothing
        return;
    }

    LOGD("x11 hotkey activated on release: "
         << action_id << " => " << final_action_id
         << ", toggle-enabled=" << toggle_enabled);
    emit hotkey_activated(final_action_id,
                          {{"toggle-on-release", toggle_enabled}});
}

#endif  // USE_X11_FEATURES

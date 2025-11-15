/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "fake_keyboard.hpp"

// clang-format off
#include "settings.h"
// clang-format on

#include <fmt/format.h>
#include <linux/uinput.h>
#include <qpa/qplatformnativeinterface.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>

#include <QClipboard>
#include <QDBusConnection>
#include <QFile>
#include <QGuiApplication>
#include <QLocale>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <tuple>

#include "dbus_klipper_inf.h"
#include "logger.hpp"
#include "qtlogger.hpp"
#include "wl-clipboard.h"

using namespace std::chrono_literals;

#ifdef USE_X11_FEATURES
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xdo.h>
#include <xkbcommon/xkbcommon-x11.h>

static XKeyEvent create_key_event(Display *display, Window &win,
                                  Window &win_root,
                                  fake_keyboard::key_code_t key) {
    XKeyEvent event;
    memset(&event, 0, sizeof(XKeyEvent));
    event.display = display;
    event.window = win;
    event.root = win_root;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = True;
    event.keycode = key.code;
    event.state = key.mask | key.layout << 13;

    // layout index is in 'Group index'
    // https://www.x.org/releases/X11R7.6/doc/libX11/specs/
    // XKB/xkblib.html#xkb_state_to_core_protocol_state_transformation

    return event;
}
#endif  // USE_X11_FEATURES

fake_keyboard::fake_keyboard(QObject *parent) : QObject{parent} {
#ifdef USE_X11_FEATURES
    switch (settings::instance()->fake_keyboard_type()) {
        case settings::fake_keyboard_type_t::FakeKeyboardTypeLegacy:
            init_legacy();
            break;
        case settings::fake_keyboard_type_t::FakeKeyboardTypeXdo:
            init_xdo();
            break;
        case settings::fake_keyboard_type_t::FakeKeyboardTypeYdo:
            init_ydo();
            break;
    }
#else
    init_ydo();
#endif
}

fake_keyboard::~fake_keyboard() {
    {
        std::lock_guard lock{m_wl_mtx};
        disconnect_wayland();
    }
#ifdef USE_X11_FEATURES
    if (m_xdo_thread.joinable()) m_xdo_thread.join();
    if (m_xdo) xdo_free(m_xdo);
#endif
    if (m_xkb_compose_table) {
        xkb_compose_table_unref(m_xkb_compose_table);
        m_xkb_compose_table = nullptr;
    }
    if (m_xkb_ctx) {
        xkb_context_unref(m_xkb_ctx);
        m_xkb_ctx = nullptr;
    }
    if (m_ydo_daemon_socket >= 0) ::close(m_ydo_daemon_socket);
}

bool fake_keyboard::is_supported() {
    return is_legacy_supported() || is_xdo_supported() || is_ydo_supported();
}

bool fake_keyboard::is_xdo_supported() {
#ifdef USE_X11_FEATURES
    return settings::instance()->is_xcb();
#else
    return false;
#endif
}

bool fake_keyboard::is_ydo_supported() {
    auto ydo_daemon_socket = make_ydo_socket();

    if (ydo_daemon_socket < 0) {
        return false;
    }

    ::close(ydo_daemon_socket);
    return true;
}

bool fake_keyboard::is_legacy_supported() {
#ifdef USE_X11_FEATURES
    return settings::instance()->is_xcb();
#else
    return false;
#endif
}

// Copied from https://github.com/ReimuNotMoe/ydotool
void fake_keyboard::ydo_uinput_emit(uint16_t type, uint16_t code, int32_t val,
                                    bool syn_report) const {
    input_event ie{};
    ie.type = type;
    ie.code = code;
    ie.value = val;

    write(m_ydo_daemon_socket, &ie, sizeof(ie));

    if (syn_report) {
        ie.type = EV_SYN;
        ie.code = SYN_REPORT;
        ie.value = 0;
        write(m_ydo_daemon_socket, &ie, sizeof(ie));
    }
}

xkb_keycode_t fake_keyboard::get_l3_shift_keycode() {
    xkb_keycode_t keycode = 0;

    if (!m_xkb_keymap) {
        LOGE("xkb_keymap not initialized");
        return keycode;
    }

    auto *state = xkb_state_new(m_xkb_keymap);
    if (!state) {
        LOGE("failed to create xkb state");
        return keycode;
    }

    auto min_keycode = xkb_keymap_min_keycode(m_xkb_keymap);
    auto max_keycode = xkb_keymap_max_keycode(m_xkb_keymap);
    const xkb_keysym_t *syms_out = nullptr;
    for (xkb_keycode_t kc = min_keycode; kc <= max_keycode; ++kc) {
        auto num_keysym = xkb_state_key_get_syms(state, kc, &syms_out);
        for (int i = 0; i < num_keysym; ++i) {
            if (syms_out[i] == XKB_KEY_ISO_Level3_Shift) {
                LOGD("l3_shift is mapped to keycode: " << kc << " " << i);
                keycode = kc;
            }
        }
    }

    xkb_state_unref(state);
    return keycode;
}

std::vector<fake_keyboard::key_code_t> fake_keyboard::key_from_character(
    /*UTF-32*/ uint32_t character) {
    if (!m_xkb_keymap) {
        LOGE("xkb_keymap not initialized");
        return {};
    }

    auto in_sym = xkb_utf32_to_keysym(character);

    const size_t XKB_KEYSYM_NAME_MAX_SIZE = 28;
    char name[XKB_KEYSYM_NAME_MAX_SIZE];
    auto ret = xkb_keysym_get_name(in_sym, name, sizeof(name));
    if (ret < 0 || static_cast<size_t>(ret) >= sizeof(name)) {
        LOGE("failed to get name of keysym");
        return {};
    }

    auto find_key_layout =
        [&](xkb_keysym_t sym) -> std::optional<fake_keyboard::key_code_t> {
        fake_keyboard::key_code_t key;
        key.sym = sym;

        auto min_keycode = xkb_keymap_min_keycode(m_xkb_keymap);
        auto max_keycode = xkb_keymap_max_keycode(m_xkb_keymap);
        for (xkb_keycode_t keycode = min_keycode; keycode <= max_keycode;
             ++keycode) {
            const auto *key_name =
                xkb_keymap_key_get_name(m_xkb_keymap, keycode);
            if (!key_name) {
                continue;
            }

            auto find_key = [&](xkb_layout_index_t layout) {
                const auto *layout_name =
                    xkb_keymap_layout_get_name(m_xkb_keymap, layout);
                if (!layout_name) {
                    return;
                }

                auto num_levels = xkb_keymap_num_levels_for_key(
                    m_xkb_keymap, keycode, layout);
                for (xkb_level_index_t level = 0; level < num_levels; ++level) {
                    const xkb_keysym_t *syms = nullptr;

                    auto num_syms = xkb_keymap_key_get_syms_by_level(
                        m_xkb_keymap, keycode, layout, level, &syms);
                    for (int sym_idx = 0; sym_idx < num_syms; ++sym_idx) {
                        if (syms[sym_idx] == key.sym) {
                            key.layout = layout;
                            key.mask = level;
                            key.code = keycode;
                            return;
                        }
                    }
                }
            };

            auto num_layouts =
                xkb_keymap_num_layouts_for_key(m_xkb_keymap, keycode);
            if (m_keyboard_layout_idx > -1) {
                find_key(
                    static_cast<xkb_layout_index_t>(m_keyboard_layout_idx));
                if (key.code > 0) return key;
            } else {
                for (xkb_layout_index_t layout = 0; layout < num_layouts;
                     ++layout) {
                    find_key(layout);
                    if (key.code > 0) return key;
                }
            }
        }

        return std::nullopt;
    };

    auto find_compose_keys = [&](xkb_keysym_t sym) {
        std::vector<fake_keyboard::key_code_t> keys;

        if (!m_xkb_compose_table) return keys;

        auto *iter = xkb_compose_table_iterator_new(m_xkb_compose_table);
        if (!iter) return keys;

        xkb_compose_table_entry *entry = nullptr;
        while ((entry = xkb_compose_table_iterator_next(iter))) {
            if (xkb_compose_table_entry_keysym(entry) == sym) {
                size_t sequence_length = 0;
                const auto *sequence =
                    xkb_compose_table_entry_sequence(entry, &sequence_length);
                if (sequence && sequence_length > 0) {
                    for (size_t i = 0; i < sequence_length; ++i) {
                        auto key = find_key_layout(sequence[i]);
                        if (key) keys.push_back(*key);
                    }
                    break;
                }
            }
        }

        xkb_compose_table_iterator_free(iter);

        return keys;
    };

    // return single key code
    if (auto key = find_key_layout(in_sym)) return {*key};

    // return compose key codes
    if (auto keys = find_compose_keys(in_sym); !keys.empty()) return keys;

    return {};
}

void fake_keyboard::ydo_type_char(uint32_t c) {
    auto keycodes = key_from_character(c);
    for (const auto &keycode : keycodes) {
        bool shift = keycode.mask == 1 || keycode.mask == 3;
        bool l3_shift = keycode.mask == 2 || keycode.mask == 3;

        if (shift) ydo_uinput_emit(EV_KEY, KEY_LEFTSHIFT, 1, true);
        if (l3_shift && m_l3_shift_keycode > 8)
            ydo_uinput_emit(EV_KEY, m_l3_shift_keycode - 8, 1, true);
        ydo_uinput_emit(EV_KEY, keycode.code - 8, 1, true);

        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(
            settings::instance()->fake_keyboard_delay()));

        ydo_uinput_emit(EV_KEY, keycode.code - 8, 0, true);
        if (l3_shift && m_l3_shift_keycode > 8)
            ydo_uinput_emit(EV_KEY, m_l3_shift_keycode - 8, 0, true);
        if (shift) ydo_uinput_emit(EV_KEY, KEY_LEFTSHIFT, 0, true);
    }
}

// Set the clipboard text
QString fake_keyboard::copy_to_clipboard(const QString &text) {
    if (text.isEmpty()) {
        LOGE("Text is empty, skipping clipboard copy");
        return QString("");
    }

    QString prev_clip_text;
    bool wayland = settings::instance()->is_wayland();
    bool failed = false;

    // Try wl-clipboard
    if (wayland) {
        LOGD("Trying wl-clipboard");

        if (!wl_paste_clipboard(prev_clip_text)) {
            LOGE("Failed to paste from wl-clipboard");
            failed = true;
        }
        if (!wl_copy_to_clipboard(text)) {
            LOGE("Failed to copy to wl-clipboard");
            failed = true;
        }
    }

    // Try Klipper
    if (failed && wayland) {
        LOGD("Trying Klipper");
        OrgKdeKlipperKlipperInterface klipper("org.kde.klipper", "/klipper",
                                              QDBusConnection::sessionBus());
        prev_clip_text = klipper.getClipboardContents();
        auto reply = klipper.setClipboardContents(text);

        failed = reply.isError();
    }

    // Try QClipboard
    if (failed || !wayland) {
        LOGD("Trying QClipboard");
        QEventLoop loop;
        auto *clip = QGuiApplication::clipboard();
        prev_clip_text = clip->text();
        loop.processEvents();
        clip->setText(text);
        loop.processEvents();
    }

    return prev_clip_text;
}

// Send Ctrl+V (paste) to the active window using the configured method
void fake_keyboard::send_ctrl_v() {
    LOGD("Sending Control V");

    // Delay to allow clipboard to update
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

#ifdef USE_X11_FEATURES
    switch (m_method) {
        case method_t::ydo:
            send_ctrl_v_ydo();
            break;
        case method_t::legacy:
            send_ctrl_v_legacy();
            break;
        case method_t::xdo:
            send_ctrl_v_xdo();
            break;
    }
#else
    // Non-X11 build: use ydo path (Wayland) by default
    if (m_ydo_daemon_socket >= 0) {
        send_ctrl_v_ydo();
    } else {
        LOGW("no method available to send ctrl+v");
    }
#endif
}

// Helper implementations split to avoid code duplication for the ydo path.
// ydo implementation is usable on both X11 and Wayland builds.
void fake_keyboard::send_ctrl_v_ydo() {
    // helper closure: emit a key event and wait a small fixed duration
    auto press_and_wait = [&](uint16_t code, int32_t val) {
        ydo_uinput_emit(EV_KEY, code, val, 1);
        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(
            settings::instance()->fake_keyboard_delay()));
    };

    press_and_wait(KEY_LEFTCTRL, 1);
    press_and_wait(KEY_V, 1);

    std::this_thread::sleep_for(50ms);

    press_and_wait(KEY_V, 0);
    press_and_wait(KEY_LEFTCTRL, 0);
}

#ifdef USE_X11_FEATURES
void fake_keyboard::send_ctrl_v_legacy() {
    if (!m_x11_display) {
        LOGW("no x11 display for legacy send_ctrl_v");
        return;
    }
    // Resolve keycodes
    KeyCode ctrl_code = XKeysymToKeycode(m_x11_display, XK_Control_L);
    KeyCode v_code = XKeysymToKeycode(m_x11_display, XK_V);
    if (ctrl_code == 0 || v_code == 0) {
        LOGW("failed to get keycodes for ctrl or v");
        return;
    }
    // Create and send events: press ctrl, press v, release v, release ctrl
    XKeyEvent event;
    memset(&event, 0, sizeof(XKeyEvent));
    event.display = m_x11_display;
    event.window = m_focus_window;
    event.root = m_root_window;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = 1;
    event.y = 1;
    event.x_root = 1;
    event.y_root = 1;
    event.same_screen = True;

    // helper closure: send event (press/release) and wait briefly
    auto send_and_wait = [&](int type, KeyCode keycode) {
        event.type = type;
        event.keycode = keycode;
        XSendEvent(event.display, event.window, True,
                   (type == KeyPress) ? KeyPressMask : KeyReleaseMask,
                   reinterpret_cast<XEvent *>(&event));
        XSync(event.display, False);
        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(
            settings::instance()->fake_keyboard_delay()));
    };

    send_and_wait(KeyPress, ctrl_code);
    send_and_wait(KeyPress, v_code);

    std::this_thread::sleep_for(50ms);

    send_and_wait(KeyRelease, v_code);
    send_and_wait(KeyRelease, ctrl_code);
}

void fake_keyboard::send_ctrl_v_xdo() {
    if (m_xdo) {
        // Use xdo to send ctrl+v sequence to current window
        xdo_send_keysequence_window(
            m_xdo, CURRENTWINDOW, "ctrl+v",
            settings::instance()->fake_keyboard_delay());
    } else {
        LOGW("XDO not available");
    }
}
#endif

int fake_keyboard::make_ydo_socket() {
    auto ydo_daemon_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (ydo_daemon_socket < 0) {
        LOGW("failed to create ydo socket");
        return -1;
    }

    auto connect_socket = [&](const std::string &file) {
        sockaddr_un sa{};
        sa.sun_family = AF_UNIX;

        snprintf(sa.sun_path, sizeof(sa.sun_path) - 1, "%s", file.c_str());

        if (::connect(ydo_daemon_socket, (const struct sockaddr *)&sa,
                      sizeof(sa))) {
            int err = errno;
            LOGD("failed to connect ydo socket: " << sa.sun_path << " "
                                                  << strerror(err));
            return false;
        }

        LOGD("connected ydo socket: " << sa.sun_path);
        return true;
    };

    auto *env_ys = getenv("YDOTOOL_SOCKET");
    auto *env_xrd = getenv("XDG_RUNTIME_DIR");

    if (env_ys) {
        if (!connect_socket(env_ys)) {
            close(ydo_daemon_socket);
            return -1;
        }
    } else if (env_xrd) {
        if (!connect_socket(std::string{env_xrd} + "/.ydotool_socket")) {
            if (!connect_socket("/tmp/.ydotool_socket")) {
                close(ydo_daemon_socket);
                return -1;
            }
        }
    } else {
        if (!connect_socket("/tmp/.ydotool_socket")) {
            close(ydo_daemon_socket);
            return -1;
        }
    }

    return ydo_daemon_socket;
}

void fake_keyboard::make_compose_table(const char *compose_file) {
    if (m_xkb_compose_table) return;  // already created

    LOGD("trying compose file: " << compose_file);

    if (auto *file = fopen(compose_file, "r")) {
        m_xkb_compose_table = xkb_compose_table_new_from_file(
            m_xkb_ctx, file, "C", XKB_COMPOSE_FORMAT_TEXT_V1,
            XKB_COMPOSE_COMPILE_NO_FLAGS);
        std::ignore = fclose(file);
    } else {
        LOGW("can't open compose file: " << compose_file);
    }
};

void fake_keyboard::init_ydo() {
    LOGD("using ydo fake-keyboard");

    m_ydo_daemon_socket = make_ydo_socket();
    if (m_ydo_daemon_socket < 0) {
        throw std::runtime_error{"failed to connect to ydo daemon"};
    }

    m_xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!m_xkb_ctx) throw std::runtime_error{"no xkb context"};

#ifdef USE_X11_FEATURES
    auto *xcb_conn = QX11Info::connection();
    if (xcb_conn) {
        auto device_id = xkb_x11_get_core_keyboard_device_id(xcb_conn);
        if (device_id == -1) throw std::runtime_error{"no xkb keyboard"};

        // get keymap from X11 server
        m_xkb_keymap = xkb_x11_keymap_new_from_device(
            m_xkb_ctx, xcb_conn, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (!m_xkb_keymap) {
            xkb_context_unref(m_xkb_ctx);
            m_xkb_ctx = nullptr;
            throw std::runtime_error{"no xkb keymap"};
        }
    }
#endif

    if (!m_keymap.empty()) {
        // get cached keymap
        m_xkb_keymap = xkb_keymap_new_from_buffer(
            m_xkb_ctx, m_keymap.data(), m_keymap.size(),
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    }

    if (!m_xkb_keymap) {
        // get keymap from wayland composer
        connect_wayland();
    }

    if (!m_xkb_keymap) {
        LOGD("fallback, create standard US keymap");

        xkb_rule_names names{};
        names.rules = nullptr;
        names.model = "pc104";
        names.layout = "us";

        m_xkb_keymap = xkb_keymap_new_from_names(m_xkb_ctx, &names,
                                                 XKB_KEYMAP_COMPILE_NO_FLAGS);
    }

    if (m_xkb_keymap) {
        m_num_layouts = xkb_keymap_num_layouts(m_xkb_keymap);
        if (m_num_layouts == 0) {
            xkb_context_unref(m_xkb_ctx);
            m_xkb_ctx = nullptr;
            LOGF("no xkb layouts");
        }

        LOGD("keyboard layouts: ");
        auto layout_str = settings::instance()->fake_keyboard_layout();
        int layout_idx = -1;
        if (!layout_str.isEmpty()) {
            bool ok = false;
            auto idx = layout_str.toInt(&ok);
            if (ok) layout_idx = idx;
        }
        for (unsigned int i = 0; i < m_num_layouts; ++i) {
            const auto *name = xkb_keymap_layout_get_name(m_xkb_keymap, i);
            LOGD(" " << i << ":" << name);

            if (m_keyboard_layout_idx < 0 &&
                (layout_idx == static_cast<int>(i) ||
                 (!layout_str.isEmpty() &&
                  QString{name}.contains(layout_str, Qt::CaseInsensitive)))) {
                m_keyboard_layout_idx = static_cast<int>(i);
            }
        }
        if (m_keyboard_layout_idx < 0) {
            m_keyboard_layout_idx = 0;  // default
        }
        LOGD("keyboard layout to use: " << m_keyboard_layout_idx);

        m_l3_shift_keycode = get_l3_shift_keycode();
    }

    if (m_xkb_ctx) {
        if (auto compose_file = settings::instance()->x11_compose_file();
            !compose_file.isEmpty()) {
            make_compose_table(compose_file.toStdString().c_str());
        }
        make_compose_table(fmt::format("/usr/share/X11/locale/{}.UTF-8/Compose",
                                       QLocale::system().name().toStdString())
                               .c_str());
        make_compose_table("/usr/share/X11/locale/C/Compose");
        if (!m_xkb_compose_table) {
            m_xkb_compose_table = xkb_compose_table_new_from_locale(
                m_xkb_ctx, "C", XKB_COMPOSE_COMPILE_NO_FLAGS);
        }
        if (!m_xkb_compose_table) LOGW("can't compile xkb compose table");
    }

    m_method = method_t::ydo;

    connect(this, &fake_keyboard::send_keyevent_request, this,
            &fake_keyboard::send_keyevent, Qt::QueuedConnection);

    m_delay_timer.setSingleShot(false);
    m_delay_timer.setInterval(settings::instance()->fake_keyboard_delay());

    connect(&m_delay_timer, &QTimer::timeout, this,
            &fake_keyboard::send_keyevent, Qt::QueuedConnection);
}

void fake_keyboard::send_text(const QString &text) {
    if (text.isEmpty()) return;

#ifdef USE_X11_FEATURES
    switch (m_method) {
        case method_t::legacy:
            send_text_legacy(text);
            break;
        case method_t::ydo:
            send_text_ydo(text);
            break;
        case method_t::xdo:
            send_text_xdo(text);
            break;
    }
#else
    send_text_ydo(text);
#endif
}

void fake_keyboard::send_text_ydo(const QString &text) {
    m_text = text.toStdU32String();

    m_text_cursor = 0;

    m_delay_timer.start();
}

void fake_keyboard::send_keyevent() {
    if (m_text_cursor >= m_text.size() && m_keys_to_send_queue.empty()) {
        m_delay_timer.stop();

        emit text_sending_completed();
        return;
    }

#ifdef USE_X11_FEATURES
    switch (m_method) {
        case method_t::legacy:
            send_keyevent_legacy();
            break;
        case method_t::ydo:
            send_keyevent_ydo();
            break;
        case method_t::xdo:
            LOGF("invalid fake-keyboard type");
    }
#else
    send_keyevent_ydo();
#endif
}

void fake_keyboard::send_keyevent_ydo() {
    ydo_type_char(m_text.at(m_text_cursor));

    ++m_text_cursor;
}

#ifdef USE_X11_FEATURES
void fake_keyboard::send_text_legacy(const QString &text) {
    auto num_layouts = xkb_keymap_num_layouts(m_xkb_keymap);
    if (num_layouts < 1) {
        LOGW("no xkb layouts");
        return;
    }

    m_text = text.toStdU32String();

    m_text_cursor = 0;

    m_delay_timer.start();
}

void fake_keyboard::send_text_xdo(const QString &text) {
    if (m_xdo_thread.joinable()) m_xdo_thread.join();

    m_xdo_thread = std::thread([this, text]() {
        xdo_enter_text_window(
            m_xdo, CURRENTWINDOW, text.toStdString().c_str(),
            settings::instance()->fake_keyboard_delay() * 100);
    });
}

void fake_keyboard::init_legacy() {
    LOGD("using legacy fake-keyboard");

    m_x11_display = QX11Info::display();
    if (!m_x11_display) throw std::runtime_error{"no x11 display"};

    m_xcb_conn = QX11Info::connection();
    if (!m_xcb_conn) throw std::runtime_error{"no xcb connection"};

    auto device_id = xkb_x11_get_core_keyboard_device_id(m_xcb_conn);
    if (device_id == -1) throw std::runtime_error{"no xkb keyboard"};

    m_xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!m_xkb_ctx) throw std::runtime_error{"no xkb context"};

    m_xkb_keymap = xkb_x11_keymap_new_from_device(
        m_xkb_ctx, m_xcb_conn, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!m_xkb_keymap) {
        xkb_context_unref(m_xkb_ctx);
        m_xkb_ctx = nullptr;
        LOGF("no xkb keymap");
    }

    m_num_layouts = xkb_keymap_num_layouts(m_xkb_keymap);
    if (m_num_layouts == 0) {
        xkb_context_unref(m_xkb_ctx);
        m_xkb_ctx = nullptr;
        LOGF("no xkb layouts");
    }
    if (m_num_layouts > XkbGroup4Index + 1) m_num_layouts = XkbGroup4Index + 1;

    LOGD("keyboard layouts: ");
    for (unsigned int i = 0; i < m_num_layouts; ++i) {
        const auto *name = xkb_keymap_layout_get_name(m_xkb_keymap, i);
        LOGD(" " << i << ":" << name);
    }

    if (auto compose_file = settings::instance()->x11_compose_file();
        !compose_file.isEmpty()) {
        make_compose_table(compose_file.toStdString().c_str());
    }
    if (!m_xkb_compose_table)
        m_xkb_compose_table = xkb_compose_table_new_from_locale(
            m_xkb_ctx, "C", XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (!m_xkb_compose_table) LOGW("can't compile xkb compose table");

    m_method = method_t::legacy;

    m_root_window = XDefaultRootWindow(m_x11_display);
    int revert;
    XGetInputFocus(m_x11_display, &m_focus_window, &revert);

    connect(this, &fake_keyboard::send_keyevent_request, this,
            &fake_keyboard::send_keyevent, Qt::QueuedConnection);

    m_delay_timer.setSingleShot(false);
    m_delay_timer.setInterval(settings::instance()->fake_keyboard_delay());

    connect(&m_delay_timer, &QTimer::timeout, this,
            &fake_keyboard::send_keyevent, Qt::QueuedConnection);
}

void fake_keyboard::init_xdo() {
    LOGD("using xdo fake-keyboard");

    if (!QX11Info::display()) {
        LOGF("no x11 display");
    }

    m_xdo = xdo_new_with_opened_display(QX11Info::display(), nullptr, 0);
    if (!m_xdo) {
        LOGF("can't create xdo");
    }

    m_method = method_t::xdo;
}

std::vector<fake_keyboard::key_code_t> fake_keyboard::key_from_character_x11(
    /*UTF-32*/ uint32_t character) {
    auto in_sym = xkb_utf32_to_keysym(character);

    auto find_key_layout =
        [&](xkb_keysym_t sym) -> std::optional<fake_keyboard::key_code_t> {
        fake_keyboard::key_code_t key;
        key.sym = sym;
        key.code = XKeysymToKeycode(m_x11_display, key.sym);

        for (unsigned int l = 0; l < m_num_layouts; ++l) {
            auto num_levels_for_key =
                xkb_keymap_num_levels_for_key(m_xkb_keymap, key.code, l);

            for (unsigned int i = 0; i < num_levels_for_key; ++i) {
                const xkb_keysym_t *syms_out = nullptr;
                auto sym_size = xkb_keymap_key_get_syms_by_level(
                    m_xkb_keymap, key.code, l, i, &syms_out);
                for (int ii = 0; ii < sym_size; ++ii) {
                    if (syms_out[ii] == key.sym) {
                        key.layout = l;

                        if (i == 1)
                            key.mask = ShiftMask;
                        else if (i == 2)
                            key.mask = XkbKeysymToModifiers(
                                m_x11_display, XK_ISO_Level3_Shift);
                        else if (i == 3)
                            key.mask = XkbKeysymToModifiers(
                                           m_x11_display, XK_ISO_Level3_Shift) |
                                       ShiftMask;
                        return key;
                    }
                }
            }
        }

        return std::nullopt;
    };

    auto find_compose_keys = [&](xkb_keysym_t sym) {
        std::vector<fake_keyboard::key_code_t> keys;

        if (!m_xkb_compose_table) return keys;

        auto *iter = xkb_compose_table_iterator_new(m_xkb_compose_table);
        if (!iter) return keys;

        xkb_compose_table_entry *entry = nullptr;
        while ((entry = xkb_compose_table_iterator_next(iter))) {
            if (xkb_compose_table_entry_keysym(entry) == sym) {
                size_t sequence_length = 0;
                const auto *sequence =
                    xkb_compose_table_entry_sequence(entry, &sequence_length);
                if (sequence && sequence_length > 0) {
                    for (size_t i = 0; i < sequence_length; ++i) {
                        auto key = find_key_layout(sequence[i]);
                        if (key) keys.push_back(*key);
                    }
                    break;
                }
            }
        }

        xkb_compose_table_iterator_free(iter);

        return keys;
    };

    // return single key code
    if (auto key = find_key_layout(in_sym)) return {*key};

    // return compose key codes
    if (auto keys = find_compose_keys(in_sym); !keys.empty()) return keys;

    // fallback, just return key code without layout
    fake_keyboard::key_code_t key;
    key.sym = in_sym;
    key.code = XKeysymToKeycode(m_x11_display, key.sym);
    return {key};
}

void fake_keyboard::send_keyevent_legacy() {
    if (m_keys_to_send_queue.empty()) {
        for (auto key : key_from_character_x11(m_text.at(m_text_cursor)))
            m_keys_to_send_queue.push(key);
        ++m_text_cursor;
    }

    auto &key = m_keys_to_send_queue.front();

    auto event =
        create_key_event(m_x11_display, m_focus_window, m_root_window, key);
    event.type = KeyPress;
    XSendEvent(event.display, event.window, True, KeyPressMask,
               reinterpret_cast<XEvent *>(&event));

    XSync(event.display, False);

    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(
        settings::instance()->fake_keyboard_delay()));

    event.type = KeyRelease;
    XSendEvent(event.display, event.window, True, KeyReleaseMask,
               reinterpret_cast<XEvent *>(&event));

    XSync(event.display, False);

    m_keys_to_send_queue.pop();
}
#endif

void fake_keyboard::connect_wayland() {
    LOGD("connect wayland");

    std::lock_guard lock{m_wl_mtx};

    auto *native = QGuiApplication::platformNativeInterface();
    if (!native) {
        LOGW("can't get native interface");
        return;
    }

    m_wl_display = static_cast<wl_display *>(
        native->nativeResourceForIntegration("display"));
    if (!m_wl_display) {
        LOGW("can't get wl display interface");
        return;
    }

    m_wl_registry = wl_display_get_registry(m_wl_display);
    wl_registry_add_listener(m_wl_registry, &wly_global_listener, this);

    auto *cb = wl_display_sync(m_wl_display);
    wl_callback_add_listener(cb, &wly_callback, nullptr);

    wl_display_roundtrip(m_wl_display);

    LOGD("wayland roundtrip done");

    disconnect_wayland();
}

void fake_keyboard::disconnect_wayland() {
    if (m_wl_keyboard) {
        wl_keyboard_release(m_wl_keyboard);
        m_wl_keyboard = nullptr;
    }
    if (m_wl_seat) {
        wl_seat_release(m_wl_seat);
        m_wl_seat = nullptr;
    }
    if (m_wl_registry) {
        wl_registry_destroy(m_wl_registry);
        m_wl_registry = nullptr;
    }
}

void fake_keyboard::wly_global_callback(void *data, wl_registry *registry,
                                        uint32_t id, const char *interface,
                                        uint32_t version) {
    LOGD("wl global: interface=" << interface << " version=" << version);

    const uint32_t required_seat_interface_ver = 7;

    auto *self = static_cast<fake_keyboard *>(data);

    if (strcmp(interface, wl_seat_interface.name) == 0 &&
        version >= required_seat_interface_ver) {
        self->m_wl_seat = static_cast<wl_seat *>(wl_registry_bind(
            registry, id, &wl_seat_interface, required_seat_interface_ver));
        wl_seat_add_listener(self->m_wl_seat, &wly_seat_listener, self);
    }

    wl_display_roundtrip(self->m_wl_display);
}

void fake_keyboard::wly_global_remove_callback(
    [[maybe_unused]] void *data, [[maybe_unused]] wl_registry *registry,
    [[maybe_unused]] uint32_t id) {}

void fake_keyboard::wly_callback_callback([[maybe_unused]] void *data,
                                          wl_callback *cb,
                                          [[maybe_unused]] uint32_t time) {
    wl_callback_destroy(cb);
}

void fake_keyboard::wly_seat_capabilities(void *data, wl_seat *wl_seat,
                                          uint32_t capabilities) {
    auto *self = static_cast<fake_keyboard *>(data);

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !self->m_wl_keyboard) {
        self->m_wl_keyboard = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_add_listener(self->m_wl_keyboard, &wly_keyboard_listener,
                                 self);
        wl_display_roundtrip(self->m_wl_display);
    } else {
        LOGW("wayland seat doesn't have keyboard capa");
    }
}

void fake_keyboard::wly_seat_name([[maybe_unused]] void *data,
                                  [[maybe_unused]] wl_seat *wl_seat,
                                  [[maybe_unused]] const char *name) {}

void fake_keyboard::wly_keyboard_keymap(
    void *data, [[maybe_unused]] wl_keyboard *wl_keyboard, uint32_t format,
    int32_t fd, uint32_t size) {
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        LOGE(
            "failed to get xkb_keymap from wayland: unsupported keymap format");
        return;
    }

    if (size == 0) {
        LOGE("failed to get xkb_keymap from wayland: keymap size is zero");
        return;
    }

    auto *self = static_cast<fake_keyboard *>(data);

    // try mmap
    auto *map_shm =
        static_cast<char *>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
    if (map_shm == MAP_FAILED) {
        LOGW("map shm failed: " << errno);

        // try read file
        m_keymap = std::string(size, '\0');
        read(fd, m_keymap.data(), size);
        auto *self = static_cast<fake_keyboard *>(data);
        self->m_xkb_keymap = xkb_keymap_new_from_string(
            self->m_xkb_ctx, m_keymap.c_str(), XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS);
    } else {
        self->m_xkb_keymap = xkb_keymap_new_from_string(
            self->m_xkb_ctx, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS);
        m_keymap.assign(map_shm, size);
        munmap(map_shm, size);
    }

    close(fd);

    if (!self->m_xkb_keymap) {
        LOGE("failed to get xkb_keymap from wayland");
        m_keymap.clear();
    }
}

void fake_keyboard::wly_keyboard_enter(
    [[maybe_unused]] void *data, [[maybe_unused]] wl_keyboard *wl_keyboard,
    [[maybe_unused]] uint32_t serial, [[maybe_unused]] wl_surface *surface,
    [[maybe_unused]] wl_array *keys) {}
void fake_keyboard::wly_keyboard_leave(
    [[maybe_unused]] void *data, [[maybe_unused]] wl_keyboard *wl_keyboard,
    [[maybe_unused]] uint32_t serial, [[maybe_unused]] wl_surface *surface) {}
void fake_keyboard::wly_keyboard_key([[maybe_unused]] void *data,
                                     [[maybe_unused]] wl_keyboard *wl_keyboard,
                                     [[maybe_unused]] uint32_t serial,
                                     [[maybe_unused]] uint32_t time,
                                     [[maybe_unused]] uint32_t key,
                                     [[maybe_unused]] uint32_t state) {}
void fake_keyboard::wly_keyboard_modifiers(
    [[maybe_unused]] void *data, [[maybe_unused]] wl_keyboard *wl_keyboard,
    [[maybe_unused]] uint32_t serial, [[maybe_unused]] uint32_t mods_depressed,
    [[maybe_unused]] uint32_t mods_latched,
    [[maybe_unused]] uint32_t mods_locked, [[maybe_unused]] uint32_t group) {}
void fake_keyboard::wly_keyboard_repeat_info(
    [[maybe_unused]] void *data, [[maybe_unused]] wl_keyboard *wl_keyboard,
    [[maybe_unused]] int32_t rate, [[maybe_unused]] int32_t delay) {}

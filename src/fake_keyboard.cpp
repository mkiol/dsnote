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

#include <linux/uinput.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <QFile>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifdef USE_X11_FEATURES
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xdo.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include <cstdio>
#include <optional>
#include <string>

#include "logger.hpp"
#include "qtlogger.hpp"

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

// Copied from https://github.com/ReimuNotMoe/ydotool
#define FLAG_UPPERCASE 0x8000000
static const int32_t ascii2keycode_map[128] = {
    // 00 - 0f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, KEY_TAB, KEY_ENTER, -1, -1, -1, -1, -1,

    // 10 - 1f
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    // 20 - 2f
    KEY_SPACE, KEY_1 | FLAG_UPPERCASE, KEY_APOSTROPHE | FLAG_UPPERCASE,
    KEY_3 | FLAG_UPPERCASE, KEY_4 | FLAG_UPPERCASE, KEY_5 | FLAG_UPPERCASE,
    KEY_7 | FLAG_UPPERCASE, KEY_APOSTROPHE, KEY_9 | FLAG_UPPERCASE,
    KEY_0 | FLAG_UPPERCASE, KEY_8 | FLAG_UPPERCASE, KEY_EQUAL | FLAG_UPPERCASE,
    KEY_COMMA, KEY_MINUS, KEY_DOT, KEY_SLASH,

    // 30 - 3f
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_SEMICOLON | FLAG_UPPERCASE, KEY_SEMICOLON, KEY_COMMA | FLAG_UPPERCASE,
    KEY_EQUAL, KEY_DOT | FLAG_UPPERCASE, KEY_SLASH | FLAG_UPPERCASE,

    // 40 - 4f
    KEY_2 | FLAG_UPPERCASE, KEY_A | FLAG_UPPERCASE, KEY_B | FLAG_UPPERCASE,
    KEY_C | FLAG_UPPERCASE, KEY_D | FLAG_UPPERCASE, KEY_E | FLAG_UPPERCASE,
    KEY_F | FLAG_UPPERCASE, KEY_G | FLAG_UPPERCASE, KEY_H | FLAG_UPPERCASE,
    KEY_I | FLAG_UPPERCASE, KEY_J | FLAG_UPPERCASE, KEY_K | FLAG_UPPERCASE,
    KEY_L | FLAG_UPPERCASE, KEY_M | FLAG_UPPERCASE, KEY_N | FLAG_UPPERCASE,
    KEY_O | FLAG_UPPERCASE,

    // 50 - 5f
    KEY_P | FLAG_UPPERCASE, KEY_Q | FLAG_UPPERCASE, KEY_R | FLAG_UPPERCASE,
    KEY_S | FLAG_UPPERCASE, KEY_T | FLAG_UPPERCASE, KEY_U | FLAG_UPPERCASE,
    KEY_V | FLAG_UPPERCASE, KEY_W | FLAG_UPPERCASE, KEY_X | FLAG_UPPERCASE,
    KEY_Y | FLAG_UPPERCASE, KEY_Z | FLAG_UPPERCASE, KEY_LEFTBRACE,
    KEY_BACKSLASH, KEY_RIGHTBRACE, KEY_6 | FLAG_UPPERCASE,
    KEY_MINUS | FLAG_UPPERCASE,

    // 60 - 6f
    KEY_GRAVE, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
    KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,

    // 70 - 7f
    KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_LEFTBRACE | FLAG_UPPERCASE, KEY_BACKSLASH | FLAG_UPPERCASE,
    KEY_RIGHTBRACE | FLAG_UPPERCASE, KEY_GRAVE | FLAG_UPPERCASE, -1};

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
#ifdef USE_X11_FEATURES
    if (m_xdo_thread.joinable()) m_xdo_thread.join();
    if (m_xdo) xdo_free(m_xdo);
    if (m_xkb_compose_table) xkb_compose_table_unref(m_xkb_compose_table);
    if (m_xkb_ctx) xkb_context_unref(m_xkb_ctx);
#endif
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
                                    bool syn_report) {
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

// Copied from https://github.com/ReimuNotMoe/ydotool
void fake_keyboard::ydo_type_char(char c) {
    int kdef = ascii2keycode_map[static_cast<unsigned char>(c)];
    if (kdef == -1) {
        return;
    }

    uint16_t kc = kdef & 0xffff;

    if (kdef & FLAG_UPPERCASE) {
        ydo_uinput_emit(EV_KEY, KEY_LEFTSHIFT, 1, 1);
    }
    ydo_uinput_emit(EV_KEY, kc, 1, 1);

    usleep(20000);

    ydo_uinput_emit(EV_KEY, kc, 0, 1);
    if (kdef & FLAG_UPPERCASE) {
        ydo_uinput_emit(EV_KEY, KEY_LEFTSHIFT, 0, 1);
    }
}

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

void fake_keyboard::init_ydo() {
    LOGD("using ydo fake-keyboard");

    m_ydo_daemon_socket = make_ydo_socket();
    if (m_ydo_daemon_socket < 0) {
        throw std::runtime_error{"failed to connect to ydo daemon"};
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
    m_text = text;

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
            throw std::runtime_error{"invalid fake-keyboard type"};
    }
#else
    send_keyevent_ydo();
#endif
}

void fake_keyboard::send_keyevent_ydo() {
    ydo_type_char(m_text.at(m_text_cursor).toLatin1());

    ++m_text_cursor;
}

#ifdef USE_X11_FEATURES
void fake_keyboard::send_text_legacy(const QString &text) {
    auto num_layouts = xkb_keymap_num_layouts(m_xkb_keymap);
    if (num_layouts < 1) {
        LOGW("no xkb layouts");
        return;
    }

    m_text = text;

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
        throw std::runtime_error{"no xkb keymap"};
    }

    m_num_layouts = xkb_keymap_num_layouts(m_xkb_keymap);
    if (m_num_layouts == 0) {
        xkb_context_unref(m_xkb_ctx);
        m_xkb_ctx = nullptr;
        throw std::runtime_error{"no xkb layouts"};
    }
    if (m_num_layouts > XkbGroup4Index + 1) m_num_layouts = XkbGroup4Index + 1;

    LOGD("keyboard layouts: ");
    for (unsigned int i = 0; i < m_num_layouts; ++i) {
        const auto *name = xkb_keymap_layout_get_name(m_xkb_keymap, i);
        LOGD(" " << i << ":" << name);
    }

    if (auto compose_file = settings::instance()->x11_compose_file();
        !compose_file.isEmpty()) {
        LOGD("using compose file: " << compose_file);
        if (auto *file = fopen(compose_file.toStdString().c_str(), "r")) {
            m_xkb_compose_table = xkb_compose_table_new_from_file(
                m_xkb_ctx, file, "C", XKB_COMPOSE_FORMAT_TEXT_V1,
                XKB_COMPOSE_COMPILE_NO_FLAGS);
            fclose(file);
        } else {
            LOGW("can't open compose file");
        }
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

    if (!QX11Info::display()) throw std::runtime_error{"no x11 display"};

    m_xdo = xdo_new_with_opened_display(QX11Info::display(), nullptr, 0);
    if (!m_xdo) throw std::runtime_error{"can't create xdo"};

    m_method = method_t::xdo;
}

std::vector<fake_keyboard::key_code_t> fake_keyboard::key_from_character(
    /*UTF-32*/ uint32_t character) {
    auto sym = xkb_utf32_to_keysym(character);

    auto find_key_layout =
        [&](xkb_keysym_t sym) -> std::optional<fake_keyboard::key_code_t> {
        fake_keyboard::key_code_t key;
        key.sym = sym;
        key.code = XKeysymToKeycode(m_x11_display, key.sym);

        for (unsigned int l = 0; l < m_num_layouts; ++l) {
            auto num_levels_for_key =
                xkb_keymap_num_levels_for_key(m_xkb_keymap, key.code, l);

            for (unsigned int i = 0; i < num_levels_for_key; ++i) {
                const xkb_keysym_t *syms_out;
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

        xkb_compose_table_entry *entry;
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
    if (auto key = find_key_layout(sym)) return {*key};

    // return compose key codes
    if (auto keys = find_compose_keys(sym); !keys.empty()) return keys;

    // fallback, just return key code without layout
    fake_keyboard::key_code_t key;
    key.sym = sym;
    key.code = XKeysymToKeycode(m_x11_display, key.sym);
    return {key};
}

void fake_keyboard::send_keyevent_legacy() {
    if (m_keys_to_send_queue.empty()) {
        for (auto key : key_from_character(m_text.at(m_text_cursor).unicode()))
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

    event.type = KeyRelease;
    XSendEvent(event.display, event.window, True, KeyReleaseMask,
               reinterpret_cast<XEvent *>(&event));

    XSync(event.display, False);

    m_keys_to_send_queue.pop();
}
#endif

/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "fake_keyboard.hpp"

// clang-format off
#include "settings.h"
// clang-format on

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include <QFile>
#include <QX11Info>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <thread>

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

fake_keyboard::fake_keyboard(QObject *parent)
    : QObject{parent},
      m_x11_display{QX11Info::display()},
      m_xcb_conn{QX11Info::connection()} {
    if (!m_x11_display) throw std::runtime_error{"no x11 display"};

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

    qDebug() << "keyboard layouts:";
    for (unsigned int i = 0; i < m_num_layouts; ++i) {
        const auto *name = xkb_keymap_layout_get_name(m_xkb_keymap, i);
        qDebug() << i << ":" << name;
    }

    if (auto compose_file = settings::instance()->x11_compose_file();
        !compose_file.isEmpty()) {
        qDebug() << "using compose file:" << compose_file;
        if (auto *file = fopen(compose_file.toStdString().c_str(), "r")) {
            m_xkb_compose_table = xkb_compose_table_new_from_file(
                m_xkb_ctx, file, "C", XKB_COMPOSE_FORMAT_TEXT_V1,
                XKB_COMPOSE_COMPILE_NO_FLAGS);
            fclose(file);
        } else {
            qWarning() << "can't open compose file";
        }
    }
    if (!m_xkb_compose_table)
        m_xkb_compose_table = xkb_compose_table_new_from_locale(
            m_xkb_ctx, "C", XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (!m_xkb_compose_table) qWarning() << "can't compile xkb compose table";

    m_root_window = XDefaultRootWindow(m_x11_display);
    int revert;
    XGetInputFocus(m_x11_display, &m_focus_window, &revert);

    connect(this, &fake_keyboard::send_keyevent_request, this,
            &fake_keyboard::send_keyevent, Qt::QueuedConnection);

    m_delay_timer.setSingleShot(false);
    m_delay_timer.setInterval(10);  // send key event every 10ms

    connect(&m_delay_timer, &QTimer::timeout, this,
            &fake_keyboard::send_keyevent, Qt::QueuedConnection);
}

fake_keyboard::~fake_keyboard() {
    if (m_xkb_compose_table) xkb_compose_table_unref(m_xkb_compose_table);
    if (m_xkb_ctx) xkb_context_unref(m_xkb_ctx);
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

void fake_keyboard::send_text(const QString &text) {
    auto num_layouts = xkb_keymap_num_layouts(m_xkb_keymap);
    if (num_layouts < 1) {
        qWarning() << "no xkb layouts";
        return;
    }

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

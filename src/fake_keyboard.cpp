/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "fake_keyboard.hpp"

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include <QX11Info>
#include <algorithm>
#include <chrono>
#include <thread>

struct key_code_t {
    unsigned int sym = 0;
    unsigned int code = 0;
    unsigned int mask = 0;
    unsigned int layout = 0;
};

static XKeyEvent create_key_event(Display *display, Window &win,
                                  Window &win_root, key_code_t key) {
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

static key_code_t key_from_character(Display *display, xkb_keymap *keymap,
                                     unsigned int num_layouts,
                                     /*UTF-32*/ uint32_t character) {
    key_code_t key;
    key.sym = xkb_utf32_to_keysym(character);
    key.code = XKeysymToKeycode(display, key.sym);

    for (unsigned int l = 0; l < num_layouts; ++l) {
        auto num_levels_for_key =
            xkb_keymap_num_levels_for_key(keymap, key.code, l);

        bool found = false;

        for (unsigned int i = 0; i < num_levels_for_key; ++i) {
            const xkb_keysym_t *syms_out;
            auto sym_size = xkb_keymap_key_get_syms_by_level(keymap, key.code,
                                                             l, i, &syms_out);
            for (int ii = 0; ii < sym_size; ++ii) {
                if (syms_out[ii] == key.sym) {
                    if (i == 1)
                        key.mask = ShiftMask;
                    else if (i == 2)
                        key.mask =
                            XkbKeysymToModifiers(display, XK_ISO_Level3_Shift);
                    else if (i == 3)
                        key.mask =
                            XkbKeysymToModifiers(display, XK_ISO_Level3_Shift) |
                            ShiftMask;

                    found = true;
                    break;
                }
            }

            if (found) break;
        }

        if (found) {
            key.layout = l;
            break;
        }
    }

    return key;
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
    if (m_xkb_ctx) xkb_context_unref(m_xkb_ctx);
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
    if (m_text_cursor >= m_text.size()) {
        m_delay_timer.stop();

        emit text_sending_completed();
        return;
    }

    auto event = create_key_event(
        m_x11_display, m_focus_window, m_root_window,
        key_from_character(m_x11_display, m_xkb_keymap, m_num_layouts,
                           m_text.at(m_text_cursor).unicode()));
    event.type = KeyPress;
    XSendEvent(event.display, event.window, True, KeyPressMask,
               reinterpret_cast<XEvent *>(&event));

    XSync(event.display, False);

    event.type = KeyRelease;
    XSendEvent(event.display, event.window, True, KeyReleaseMask,
               reinterpret_cast<XEvent *>(&event));

    XSync(event.display, False);

    ++m_text_cursor;
}

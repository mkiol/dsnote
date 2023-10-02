/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "keyboard_tools.hpp"

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include <QX11Info>
#include <algorithm>

namespace keyboard_tools {

struct key_t {
    unsigned int sym = 0;
    unsigned int code = 0;
    unsigned int mask = 0;
};

static XKeyEvent create_key_event(Display *display, Window &win,
                                  Window &win_root, key_t key) {
    XKeyEvent event;
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
    event.state = key.mask;

    return event;
}

static key_t key_from_character(Display *display, xkb_keymap *keymap,
                                xkb_layout_index_t layout,
                                /*UTF-32*/ uint32_t character) {
    key_t key;
    key.sym = xkb_utf32_to_keysym(character);
    key.code = XKeysymToKeycode(display, key.sym);

    auto num_levels_for_key =
        xkb_keymap_num_levels_for_key(keymap, key.code, layout);

    for (unsigned int i = 0; i < num_levels_for_key; ++i) {
        const xkb_keysym_t *syms_out;
        auto sym_size =
            xkb_keymap_key_get_syms_by_level(keymap, key.code, 0, i, &syms_out);
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
                break;
            }
        }
    }

    return key;
}

void send_text(const QString &text) {
    auto *display = QX11Info::display();
    if (!display) {
        qWarning() << "no x11 display";
        return;
    }

    auto *conn = QX11Info::connection();

    auto device_id = xkb_x11_get_core_keyboard_device_id(conn);
    if (device_id == -1) {
        qWarning() << "xkb_x11_get_core_keyboard_device_id error";
        return;
    }

    auto *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        qWarning() << "xkb_context_new error";
        return;
    }

    auto *keymap = xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                                  XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        qWarning() << "xkb_x11_keymap_new_from_device error";
        xkb_context_unref(ctx);
        return;
    }

    auto num_layouts = xkb_keymap_num_layouts(keymap);
    if (num_layouts < 1) {
        qWarning() << "xkb_keymap_num_layouts error";
        xkb_context_unref(ctx);
        return;
    }

    qDebug() << "keyboard layouts:";
    for (unsigned int i = 0; i < num_layouts; ++i) {
        qDebug() << i << ":" << xkb_keymap_layout_get_name(keymap, i);
    }

    auto win_root = XDefaultRootWindow(display);
    Window win_focus;
    int revert;
    XGetInputFocus(display, &win_focus, &revert);

    std::for_each(text.cbegin(), text.cend(), [&](QChar c) {
        auto event = create_key_event(
            display, win_focus, win_root,
            key_from_character(display, keymap, /*layout=*/0, c.unicode()));

        event.type = KeyPress;
        XSendEvent(event.display, event.window, True, KeyPressMask,
                   reinterpret_cast<XEvent *>(&event));

        event.type = KeyRelease;
        XSendEvent(event.display, event.window, True, KeyPressMask,
                   reinterpret_cast<XEvent *>(&event));
    });

    XFlush(display);

    xkb_context_unref(ctx);
}
}  // namespace keyboard_tools

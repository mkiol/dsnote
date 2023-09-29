/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "keyboard_tools.hpp"

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <QX11Info>
#include <algorithm>

namespace keyboard_tools {

static XKeyEvent create_key_event(Display *display, Window &win,
                                  Window &win_root, int keycode,
                                  int modifiers) {
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
    event.keycode = XKeysymToKeycode(display, keycode);
    event.state = modifiers;

    return event;
}

void send_text(const QString &text) {
    auto *display = QX11Info::display();
    if (!display) {
        qWarning() << "no x11 display";
        return;
    }

    auto win_root = XDefaultRootWindow(display);
    Window win_focus;
    int revert;
    XGetInputFocus(display, &win_focus, &revert);

    std::for_each(text.cbegin(), text.cend(), [&](QChar c) {
        unsigned int key_mask = 0;
        auto keysym = XK_space;

        if (c.isLetterOrNumber()) {
            char str[] = " ";
            str[0] = c.toLatin1();
            keysym = XStringToKeysym(str);
            if (c.isUpper()) key_mask = ShiftMask;
        } else if (c.isPunct()) {
            if (c == '.') {
                keysym = XK_period;
            } else if (c == ',') {
                keysym = XK_comma;
            } else if (c == '!') {
                keysym = XK_exclam;
                key_mask = ShiftMask;
            } else if (c == '?') {
                keysym = XK_question;
                key_mask = ShiftMask;
            } else if (c == ';') {
                keysym = XK_semicolon;
            } else {
                qWarning() << "unknown punct character";
            }
        } else if (c.isSpace()) {
        } else {
            qWarning() << "unknown character";
        }

        auto event =
            create_key_event(display, win_focus, win_root, keysym, key_mask);

        event.type = KeyPress;
        XSendEvent(event.display, event.window, True, KeyPressMask,
                   reinterpret_cast<XEvent *>(&event));

        event.type = KeyRelease;
        XSendEvent(event.display, event.window, True, KeyPressMask,
                   reinterpret_cast<XEvent *>(&event));
    });

    XFlush(display);
}
}  // namespace keyboard_tools

/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef FAKE_KEYBOARD_HPP
#define FAKE_KEYBOARD_HPP

#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QX11Info>
#include <queue>

struct xcb_connection_t;
struct xkb_context;
struct xkb_keymap;

class fake_keyboard : public QObject {
    Q_OBJECT
   public:
    struct key_code_t {
        unsigned int sym = 0;
        unsigned int code = 0;
        unsigned int mask = 0;
        unsigned int layout = 0;
    };

    fake_keyboard(QObject* parent = nullptr);
    ~fake_keyboard();
    void send_text(const QString& text);

   signals:
    void text_sending_completed();
    void send_keyevent_request();

   private:
    Display* m_x11_display = nullptr;
    xcb_connection_t* m_xcb_conn = nullptr;
    xkb_context* m_xkb_ctx = nullptr;
    xkb_keymap* m_xkb_keymap = nullptr;
    unsigned long m_root_window = 0;
    unsigned long m_focus_window = 0;
    QString m_text;
    int m_text_cursor = 0;
    QTimer m_delay_timer;
    unsigned int m_num_layouts = 0;
    std::queue<key_code_t> m_keys_to_send_queue;

    void send_keyevent();
};

#endif  // FAKE_KEYBOARD_HPP

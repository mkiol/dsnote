/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
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
#include <cstdint>
#include <queue>

#ifdef USE_X11_FEATURES
#include <QX11Info>
#include <thread>

struct xcb_connection_t;
struct xkb_context;
struct xkb_keymap;
struct xkb_compose_table;
struct xdo;
#endif

class fake_keyboard : public QObject {
    Q_OBJECT
   public:
    struct key_code_t {
        unsigned int sym = 0;
        unsigned int code = 0;
        unsigned int mask = 0;
        unsigned int layout = 0;
    };

    static bool is_supported();
    static bool is_xdo_supported();
    static bool is_ydo_supported();
    static bool is_legacy_supported();

    fake_keyboard(QObject* parent = nullptr);
    ~fake_keyboard();
    void send_text(const QString& text);

   signals:
    void text_sending_completed();
    void send_keyevent_request();

   private:
    enum class method_t { legacy, xdo, ydo };
    method_t m_method = method_t::legacy;
    std::queue<key_code_t> m_keys_to_send_queue;
    QString m_text;
    int m_text_cursor = 0;
    QTimer m_delay_timer;
    int m_ydo_daemon_socket = -1;
#ifdef USE_X11_FEATURES
    Display* m_x11_display = nullptr;
    xdo* m_xdo = nullptr;
    xcb_connection_t* m_xcb_conn = nullptr;
    xkb_context* m_xkb_ctx = nullptr;
    xkb_keymap* m_xkb_keymap = nullptr;
    xkb_compose_table* m_xkb_compose_table = nullptr;
    std::thread m_xdo_thread;
    unsigned long m_root_window = 0;
    unsigned long m_focus_window = 0;
    unsigned int m_num_layouts = 0;
#endif

    static int make_ydo_socket();

    void init_ydo();
    void send_text_ydo(const QString& text);
    void send_keyevent();
    void send_keyevent_ydo();
    void ydo_uinput_emit(uint16_t type, uint16_t code, int32_t val,
                         bool syn_report);
    void ydo_type_char(char c);
#ifdef USE_X11_FEATURES
    void init_xdo();
    void init_legacy();
    void send_text_xdo(const QString& text);
    void send_text_legacy(const QString& text);
    void send_keyevent_legacy();
    std::vector<fake_keyboard::key_code_t> key_from_character(
        /*UTF-32*/ uint32_t character);
#endif
};

#endif  // FAKE_KEYBOARD_HPP

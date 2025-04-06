/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef FAKE_KEYBOARD_HPP
#define FAKE_KEYBOARD_HPP

#include <wayland-client.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#ifdef USE_X11_FEATURES
#include <QX11Info>

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

    explicit fake_keyboard(QObject* parent = nullptr);
    ~fake_keyboard() override;
    void send_text(const QString& text);

   signals:
    void text_sending_completed();
    void send_keyevent_request();

   private:
    enum class method_t { legacy, xdo, ydo };
    inline static std::string m_keymap{};
    method_t m_method = method_t::legacy;
    std::queue<key_code_t> m_keys_to_send_queue;
    std::u32string m_text;
    size_t m_text_cursor = 0;
    QTimer m_delay_timer;
    int m_ydo_daemon_socket = -1;
    xkb_context* m_xkb_ctx = nullptr;
    xkb_keymap* m_xkb_keymap = nullptr;
    xkb_compose_table* m_xkb_compose_table = nullptr;
    uint32_t m_l3_shift_keycode = 0;
    wl_display* m_wl_display = nullptr;
    wl_registry* m_wl_registry = nullptr;
    wl_seat* m_wl_seat = nullptr;
    wl_keyboard* m_wl_keyboard = nullptr;
    std::mutex m_wl_mtx;
    int m_keyboard_layout_idx = -1;
#ifdef USE_X11_FEATURES
    Display* m_x11_display = nullptr;
    xdo* m_xdo = nullptr;
    xcb_connection_t* m_xcb_conn = nullptr;
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
                         bool syn_report) const;
    void ydo_type_char(uint32_t c);
    std::vector<fake_keyboard::key_code_t> key_from_character(
        uint32_t character);
    uint32_t get_l3_shift_keycode();
    void connect_wayland();
    void disconnect_wayland();
    void make_compose_table(const char* compose_file);
#ifdef USE_X11_FEATURES
    void init_xdo();
    void init_legacy();
    void send_text_xdo(const QString& text);
    void send_text_legacy(const QString& text);
    void send_keyevent_legacy();
    std::vector<fake_keyboard::key_code_t> key_from_character_x11(
        /*UTF-32*/ uint32_t character);
#endif
    static void wly_global_callback(void* data, wl_registry* registry,
                                    uint32_t id, const char* interface,
                                    uint32_t version);
    static void wly_global_remove_callback(void* data, wl_registry* registry,
                                           uint32_t id);
    static void wly_callback_callback(void* data, wl_callback* cb,
                                      uint32_t time);
    static void wly_seat_capabilities(void* data, wl_seat* wl_seat,
                                      uint32_t capabilities);
    static void wly_seat_name(void* data, wl_seat* wl_seat, const char* name);
    static void wly_keyboard_keymap(void* data, wl_keyboard* wl_keyboard,
                                    uint32_t format, int32_t fd, uint32_t size);
    static void wly_keyboard_enter(void* data, wl_keyboard* wl_keyboard,
                                   uint32_t serial, wl_surface* surface,
                                   wl_array* keys);
    static void wly_keyboard_leave(void* data, wl_keyboard* wl_keyboard,
                                   uint32_t serial, wl_surface* surface);
    static void wly_keyboard_key(void* data, wl_keyboard* wl_keyboard,
                                 uint32_t serial, uint32_t time, uint32_t key,
                                 uint32_t state);
    static void wly_keyboard_modifiers(void* data, wl_keyboard* wl_keyboard,
                                       uint32_t serial, uint32_t mods_depressed,
                                       uint32_t mods_latched,
                                       uint32_t mods_locked, uint32_t group);
    static void wly_keyboard_repeat_info(void* data, wl_keyboard* wl_keyboard,
                                         int32_t rate, int32_t delay);
    inline static const wl_callback_listener wly_callback{
        wly_callback_callback};
    inline static const wl_registry_listener wly_global_listener{
        wly_global_callback, wly_global_remove_callback};
    inline static const wl_seat_listener wly_seat_listener{
        wly_seat_capabilities, wly_seat_name};
    inline static const wl_keyboard_listener wly_keyboard_listener{
        wly_keyboard_keymap, wly_keyboard_enter,     wly_keyboard_leave,
        wly_keyboard_key,    wly_keyboard_modifiers, wly_keyboard_repeat_info};
};

#endif  // FAKE_KEYBOARD_HPP

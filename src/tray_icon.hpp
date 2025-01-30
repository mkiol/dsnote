/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef TRAYICON_H
#define TRAYICON_H

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVariantMap>
#include <utility>
#include <vector>

class tray_icon : public QSystemTrayIcon {
    Q_OBJECT
   public:
    enum class action_t {
        start_listening,
        start_listening_translate,
        start_listening_active_window,
        start_listening_translate_active_window,
        start_listening_clipboard,
        start_listening_translate_clipboard,
        stop_listening,
        start_reading,
        start_reading_clipboard,
        pause_resume_reading,
        cancel,
        quit,
        toggle_app_window,
        change_stt_model,
        change_tts_model
    };
    enum class state_t { idle, busy, stt, stt_file, tts, tts_file, mnt };
    enum class task_state_t {
        idle = 0,
        processing = 1,
        initializing = 2,
        playing = 3,
        paused = 4,
        cancelling = 5
    };

    explicit tray_icon(QObject *parent = nullptr);
    void set_state(state_t state);
    void set_task_state(task_state_t task_state);
    void set_stt_models(QVariantList&& stt_models);
    void set_active_stt_model(QString&& stt_model, bool translate_supported);
    void set_tts_models(QVariantList&& tts_models);
    void set_active_tts_model(QString&& tts_model);
    void set_fake_keyboard_supported(bool supported);

   signals:
    void action_triggered(action_t action, int value);

   private:
    inline static const auto app_icon = QStringLiteral(":/app_icon.png");
    QMenu m_menu;
    QMenu m_menu_translate;
    state_t m_state = state_t::busy;
    task_state_t m_task_state = task_state_t::idle;
    std::vector<std::pair<action_t, QObject*>> m_actions;
    QVariantList m_stt_models;
    QString m_active_stt_model;
    QVariantList m_tts_models;
    QString m_active_tts_model;
    QTimer m_animated_icon_timer;
    uint8_t m_icon_idx = 0;
    bool m_stt_translate_supported = false;
    bool m_fake_keyboard_supported = false;

    void make_menu();
    void update_menu();
    void update_icon();
    void update_animated_icon();
};

#endif  // TRAYICON_H

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
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
#include <QVariantMap>
#include <unordered_map>

class tray_icon : public QSystemTrayIcon {
    Q_OBJECT
   public:
    enum class action_t {
        start_listening,
        start_listening_active_window,
        start_listening_clipboard,
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
    void set_active_stt_model(QString&& stt_model);
    void set_tts_models(QVariantList&& tts_models);
    void set_active_tts_model(QString&& tts_model);

   signals:
    void action_triggered(action_t action, int value);

   private:
    QMenu m_menu;
    state_t m_state = state_t::busy;
    task_state_t m_task_state = task_state_t::idle;
    std::unordered_map<action_t, QObject*> m_actions;
    QVariantList m_stt_models;
    QString m_active_stt_model;
    QVariantList m_tts_models;
    QString m_active_tts_model;

    void make_menu();
    void update_menu();
};

#endif  // TRAYICON_H

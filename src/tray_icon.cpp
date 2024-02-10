/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tray_icon.hpp"

#include <QDebug>
#include <algorithm>

#include "config.h"
#include "settings.h"

tray_icon::tray_icon(QObject* parent)
    : QSystemTrayIcon{QIcon{QStringLiteral(":/app_icon.png")}, parent} {
    setToolTip(APP_NAME);

    make_menu();

    auto* s = settings::instance();

    connect(
        s, &settings::use_tray_changed, this,
        [this]() {
            if (settings::instance()->use_tray())
                show();
            else
                hide();
        },
        Qt::QueuedConnection);

    if (s->use_tray()) show();
}

void tray_icon::set_state(state_t state) {
    if (m_state != state) {
        m_state = state;
        update_menu();
    }
}

void tray_icon::set_task_state(task_state_t task_state) {
    if (m_task_state != task_state) {
        m_task_state = task_state;
        update_menu();
    }
}

void tray_icon::update_menu() {
    std::for_each(m_actions.cbegin(), m_actions.cend(), [this](auto& p) {
        switch (p.first) {
            case action_t::start_listening:
            case action_t::start_listening_active_window:
            case action_t::start_listening_clipboard:
            case action_t::start_reading:
            case action_t::start_reading_clipboard:
                p.second->setEnabled(m_state == state_t::idle);
                break;
            case action_t::stop_listening:
                p.second->setProperty("enabled", m_state == state_t::stt);
                break;
            case action_t::pause_resume_reading:
                p.second->setEnabled(m_state == state_t::tts);
                p.second->setIcon(QIcon::fromTheme(
                    m_task_state == task_state_t::paused
                        ? QStringLiteral("media-playback-start-symbolic")
                        : QStringLiteral("media-playback-pause-symbolic")));
                p.second->setText(m_task_state == task_state_t::paused
                                      ? tr("Resume reading")
                                      : tr("Pause reading"));
                break;
            case action_t::cancel:
                p.second->setProperty("enabled", m_state != state_t::idle &&
                                                     m_state != state_t::busy);
                break;
            case action_t::quit:
                break;
        }
    });
}

void tray_icon::make_menu() {
    m_actions.clear();

    m_actions.emplace(
        action_t::start_listening,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("audio-input-microphone-symbolic")),
            tr("Start listening")));
#ifdef USE_X11_FEATURES
    if (settings::instance()->is_xcb()) {
        m_actions.emplace(
            action_t::start_listening_active_window,
            m_menu.addAction(QIcon::fromTheme(QStringLiteral(
                                 "audio-input-microphone-symbolic")),
                             tr("Start listening, text to active window")));
    }
#endif
    m_actions.emplace(
        action_t::start_listening_clipboard,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("audio-input-microphone-symbolic")),
            tr("Start listening, text to clipboard")));
    m_actions.emplace(
        action_t::stop_listening,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("media-playback-stop-symbolic")),
            tr("Stop listening")));
    m_menu.addSeparator();
    m_actions.emplace(
        action_t::start_reading,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("audio-speakers-symbolic")),
            tr("Start reading")));
    m_actions.emplace(
        action_t::start_reading_clipboard,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("audio-speakers-symbolic")),
            tr("Start reading text from clipboard")));
    m_actions.emplace(
        action_t::pause_resume_reading,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("media-playback-pause-symbolic")),
            tr("Pause/Resume reading")));
    m_menu.addSeparator();
    m_actions.emplace(
        action_t::cancel,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("action-unavailable-symbolic")),
            tr("Cancel")));
    m_menu.addSeparator();
    m_actions.emplace(
        action_t::quit,
        m_menu.addAction(
            QIcon::fromTheme(QStringLiteral("application-exit-symbolic")),
            tr("Quit")));

    std::for_each(m_actions.cbegin(), m_actions.cend(), [this](const auto& p) {
        connect(p.second, &QAction::triggered, this,
                [this, action = p.first]() { emit action_triggered(action); });
    });

    update_menu();

    setContextMenu(&m_menu);
}

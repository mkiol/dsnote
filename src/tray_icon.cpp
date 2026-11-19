/* Copyright (C) 2023-2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tray_icon.hpp"

#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <algorithm>

#include "config.h"
#include "settings.h"

tray_icon::tray_icon(QObject* parent)
    : QSystemTrayIcon{QIcon{app_icon}, parent} {
    update_tooltip();
    make_menu();

    connect(
        settings::instance(), &settings::use_tray_changed, this,
        [this]() {
            setVisible(settings::instance()->use_tray());
            update_icon();
        },
        Qt::QueuedConnection);

    // timer for animated icon
    m_animated_icon_timer.setSingleShot(false);
    m_animated_icon_timer.setInterval(250);  // 0.5s
    connect(&m_animated_icon_timer, &QTimer::timeout, this,
            &tray_icon::update_animated_icon, Qt::QueuedConnection);

    setVisible(settings::instance()->use_tray());
}

void tray_icon::set_state(state_t state) {
    if (m_state != state) {
        m_state = state;
        update_menu();
        update_icon();
        update_tooltip();
    }
}

void tray_icon::set_task_state(task_state_t task_state) {
    if (m_task_state != task_state) {
        m_task_state = task_state;
        update_menu();
        update_icon();
        update_tooltip();
    }
}

void tray_icon::set_listening_state(bool listening_state) {
    if (m_listening_state != listening_state) {
        m_listening_state = listening_state;
        update_menu();
        update_icon();
        update_tooltip();
    }
}

void tray_icon::update_icon() {
    if (isVisible()) {
        switch (m_state) {
            case state_t::idle:
                m_animated_icon_timer.stop();
                break;
            case state_t::busy:
            case state_t::stt:
            case state_t::stt_file:
            case state_t::tts:
            case state_t::tts_file:
            case state_t::mnt:
                m_animated_icon_timer.start();
                break;
        }
    } else {
        m_animated_icon_timer.stop();
    }

    update_animated_icon();
}

void tray_icon::update_tooltip() {
    QString text{APP_NAME};

    QString info;
    switch (m_task_state) {
        case task_state_t::idle:
        case task_state_t::speech_detected:
        case task_state_t::playing:
            break;
        case task_state_t::paused:
            info = tr("Reading is paused.");
            break;
        case task_state_t::processing:
            switch (m_state) {
                case state_t::idle:
                case state_t::busy:
                case state_t::stt_file:
                    break;
                case state_t::stt:
                    if (m_listening_state) {
                        info =
                            tr("Keep talking... (processing your last sentence "
                               "while you talk)");
                        break;
                    }
                    [[fallthrough]];
                case state_t::tts:
                case state_t::tts_file:
                case state_t::mnt:
                    info = tr("Processing, please wait...");
                    break;
            }
            break;
        case task_state_t::initializing:
            info = tr("Getting ready, please wait...");
            break;
        case task_state_t::cancelling:
            info = tr("Cancelling, please wait...");
            break;
    }

    if (info.isEmpty()) {
        switch (m_state) {
            case state_t::idle:
                break;
            case state_t::busy:
                info = tr("Busy...");
                break;
            case state_t::stt:
                info = tr("Say something...");
                break;
            case state_t::stt_file:
                info = tr("Transcribing audio file...");
                break;
            case state_t::tts:
            case state_t::tts_file:
                info = tr("Reading a note...");
                break;
            case state_t::mnt:
                info = tr("Translating...");
                break;
        }
    }

    if (!info.isEmpty()) {
        text += " — " + info;
    }

    setToolTip(text);
}

void tray_icon::make_icon_cache() {
    if (!m_icon_cache.empty()) {
        return;
    }

    auto add_border = [](QIcon& icon) {
        auto px = icon.pixmap(icon.actualSize({84, 91}));
        const int border_size = px.width() * 0.2;
        QPainter painter{&px};
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen{Qt::red, static_cast<qreal>(border_size)});
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.drawRect(0, 0, px.width(), px.height());
        icon = QIcon{px};
    };

    auto make_icon_set = [&](const QString& key, int idx) {
        QIcon icon{idx < 0 ? QStringLiteral(":/%1.png").arg(key)
                           : QStringLiteral(":/%1_%2.png").arg(key).arg(idx)};

        m_icon_cache.insert(
            QStringLiteral("%1_%2_0").arg(key).arg(idx < 0 ? 0 : idx), icon);

        // add red border to indicate active listening state
        add_border(icon);
        m_icon_cache.insert(
            QStringLiteral("%1_%2_1").arg(key).arg(idx < 0 ? 0 : idx), icon);
    };

    make_icon_set("app_icon", -1);
    for (auto idx : {0, 1}) {
        make_icon_set("busy_icon", idx);
        make_icon_set("pause_icon", idx);
        make_icon_set("speech_icon", idx);
    }
}

void tray_icon::update_animated_icon() {
    m_icon_idx = (m_icon_idx + 1) % 2;

    icon_action_t action = icon_action_t::none;

    if (isVisible()) {
        switch (m_state) {
            case state_t::idle:
                action = icon_action_t::app;
                break;
            case state_t::busy:
            case state_t::tts_file:
            case state_t::stt_file:
            case state_t::mnt:
                action = icon_action_t::busy;
                break;
            case state_t::tts:
            case state_t::stt:
                switch (m_task_state) {
                    case task_state_t::idle:
                        action = icon_action_t::app;
                        break;
                    case task_state_t::initializing:
                    case task_state_t::cancelling:
                        action = icon_action_t::busy;
                        break;
                    case task_state_t::paused:
                        action = icon_action_t::pause;
                        break;
                    case task_state_t::processing:
                        action = icon_action_t::busy;
                        break;
                    case task_state_t::playing:
                    case task_state_t::speech_detected:
                        action = icon_action_t::speech;
                        break;
                }
                break;
        }
    } else {
        action = icon_action_t::app;
    }

    make_icon_cache();

    QString icon_key;
    switch (action) {
        case icon_action_t::none:
            break;
        case icon_action_t::app:
            icon_key =
                QStringLiteral("app_icon_0_%1").arg(m_listening_state ? 1 : 0);
            break;
        case icon_action_t::busy:
            icon_key = QStringLiteral("busy_icon_%1_%2")
                           .arg(m_icon_idx)
                           .arg(m_listening_state ? 1 : 0);
            break;
        case icon_action_t::pause:
            icon_key = QStringLiteral("pause_icon_%1_%2")
                           .arg(m_icon_idx)
                           .arg(m_listening_state ? 1 : 0);
            break;
        case icon_action_t::speech:
            icon_key = QStringLiteral("speech_icon_%1_%2")
                           .arg(m_icon_idx)
                           .arg(m_listening_state ? 1 : 0);
            break;
    }

    if (!icon_key.isEmpty()) {
        setIcon(m_icon_cache.value(icon_key));
    }

    if (!isVisible()) {
        m_animated_icon_timer.stop();
    }
}

void tray_icon::update_menu() {
    std::for_each(m_actions.cbegin(), m_actions.cend(), [this](auto& p) {
        auto stt_configured = !m_active_stt_model.isEmpty();
        auto tts_configured = !m_active_tts_model.isEmpty();

        switch (p.first) {
            case action_t::start_listening_translate_active_window:
                p.second->setProperty(
                    "enabled", stt_configured && m_state == state_t::idle &&
                                   m_stt_translate_supported &&
                                   m_fake_keyboard_supported);
                break;
            case action_t::start_listening_translate:
            case action_t::start_listening_translate_clipboard:
                p.second->setProperty("enabled", stt_configured &&
                                                     m_state == state_t::idle &&
                                                     m_stt_translate_supported);
                break;
            case action_t::start_listening_active_window:
                p.second->setProperty("enabled", stt_configured &&
                                                     m_state == state_t::idle &&
                                                     m_fake_keyboard_supported);
                break;
            case action_t::start_listening:
            case action_t::start_listening_clipboard:
                p.second->setProperty(
                    "enabled", stt_configured && m_state == state_t::idle);
                break;
            case action_t::stop_listening:
                p.second->setProperty(
                    "enabled", stt_configured && m_state == state_t::stt);

                break;
            case action_t::start_reading:
            case action_t::start_reading_clipboard:
                p.second->setProperty(
                    "enabled", tts_configured && m_state == state_t::idle);
                break;
            case action_t::pause_resume_reading:
                p.second->setProperty(
                    "enabled",
                    tts_configured && (m_state == state_t::tts ||
                                       m_state == state_t::tts_file));
                qobject_cast<QAction*>(p.second)->setIcon(QIcon::fromTheme(
                    m_task_state == task_state_t::paused
                        ? QStringLiteral("media-playback-start-symbolic")
                        : QStringLiteral("media-playback-pause-symbolic")));
                qobject_cast<QAction*>(p.second)->setText(
                    m_task_state == task_state_t::paused ? tr("Resume reading")
                                                         : tr("Pause reading"));
                break;
            case action_t::cancel:
                p.second->setProperty(
                    "enabled", m_state != state_t::idle &&
                                   m_state != state_t::busy &&
                                   m_task_state != task_state_t::cancelling);
                break;
            case action_t::change_stt_model: {
                auto* menu = qobject_cast<QMenu*>(p.second);
                menu->setProperty("enabled",
                                  stt_configured && m_state == state_t::idle);
                menu->setTitle(!stt_configured ? tr("No Speech to Text model")
                                               : m_active_stt_model);
                menu->clear();
                for (auto it = m_stt_models.cbegin(); it != m_stt_models.cend();
                     ++it) {
                    auto* action = menu->addAction(it->toString());
                    action->setProperty("idx", static_cast<int>(std::distance(
                                                   m_stt_models.cbegin(), it)));
                    connect(action, &QAction::triggered, this, [this]() {
                        emit action_triggered(
                            tray_icon::action_t::change_stt_model,
                            sender()->property("idx").toInt());
                    });
                }
                break;
            }
            case action_t::change_tts_model: {
                auto* menu = qobject_cast<QMenu*>(p.second);
                menu->setProperty("enabled",
                                  tts_configured && m_state == state_t::idle);
                menu->setTitle(!tts_configured ? tr("No Text to Speech model")
                                               : m_active_tts_model);
                menu->clear();
                for (auto it = m_tts_models.cbegin(); it != m_tts_models.cend();
                     ++it) {
                    auto* action = menu->addAction(it->toString());
                    action->setProperty("idx", static_cast<int>(std::distance(
                                                   m_tts_models.cbegin(), it)));
                    connect(action, &QAction::triggered, this, [this]() {
                        emit action_triggered(
                            tray_icon::action_t::change_tts_model,
                            sender()->property("idx").toInt());
                    });
                }
                break;
            }
            case action_t::quit:
            case action_t::toggle_app_window:
                break;
        }
    });

    QMenu* new_menu = m_stt_translate_supported ? &m_menu_translate : &m_menu;
    if (contextMenu() != new_menu) {
        setContextMenu(new_menu);
    }
}

void tray_icon::make_menu() {
    m_actions.clear();

    auto add_action_to_menu = [this](action_t action_type, const char* icon,
                                     const QString& text, QMenu& menu) {
        auto* action = menu.addAction(QIcon::fromTheme(icon), text);
        m_actions.emplace_back(action_type, action);
    };
    auto add_menu_to_menu = [this](action_t action_type, const char* icon,
                                   QMenu& menu) {
        auto* action = menu.addMenu(QIcon::fromTheme(icon), QString{});
        m_actions.emplace_back(action_type, action);
    };

    add_action_to_menu(action_t::start_listening,
                       "audio-input-microphone-symbolic", tr("Start listening"),
                       m_menu);
    add_action_to_menu(action_t::start_listening,
                       "audio-input-microphone-symbolic", tr("Start listening"),
                       m_menu_translate);
    add_action_to_menu(
        action_t::start_listening_translate, "audio-input-microphone-symbolic",
        tr("Start listening, always translate"), m_menu_translate);
    add_action_to_menu(action_t::start_listening_active_window,
                       "audio-input-microphone-symbolic",
                       tr("Start listening, text to active window"), m_menu);
    add_action_to_menu(action_t::start_listening_active_window,
                       "audio-input-microphone-symbolic",
                       tr("Start listening, text to active window"),
                       m_menu_translate);
    add_action_to_menu(
        action_t::start_listening_translate_active_window,
        "audio-input-microphone-symbolic",
        tr("Start listening, always translate, text to active window"),
        m_menu_translate);
    add_action_to_menu(action_t::start_listening_clipboard,
                       "audio-input-microphone-symbolic",
                       tr("Start listening, text to clipboard"), m_menu);
    add_action_to_menu(
        action_t::start_listening_clipboard, "audio-input-microphone-symbolic",
        tr("Start listening, text to clipboard"), m_menu_translate);
    add_action_to_menu(
        action_t::start_listening_translate_clipboard,
        "audio-input-microphone-symbolic",
        tr("Start listening, always translate, text to clipboard"),
        m_menu_translate);
    add_action_to_menu(action_t::stop_listening, "media-playback-stop-symbolic",
                       tr("Stop listening"), m_menu);
    add_action_to_menu(action_t::stop_listening, "media-playback-stop-symbolic",
                       tr("Stop listening"), m_menu_translate);
    add_menu_to_menu(action_t::change_stt_model,
                     "audio-input-microphone-symbolic", m_menu);
    add_menu_to_menu(action_t::change_stt_model,
                     "audio-input-microphone-symbolic", m_menu_translate);
    m_menu.addSeparator();
    m_menu_translate.addSeparator();
    add_action_to_menu(action_t::start_reading, "text-speak-symbolic",
                       tr("Start reading"), m_menu);
    add_action_to_menu(action_t::start_reading, "text-speak-symbolic",
                       tr("Start reading"), m_menu_translate);
    add_action_to_menu(action_t::start_reading_clipboard, "text-speak-symbolic",
                       tr("Start reading text from clipboard"), m_menu);
    add_action_to_menu(action_t::start_reading_clipboard, "text-speak-symbolic",
                       tr("Start reading text from clipboard"),
                       m_menu_translate);
    add_action_to_menu(action_t::pause_resume_reading,
                       "media-playback-pause-symbolic",
                       tr("Pause/Resume reading"), m_menu);
    add_action_to_menu(action_t::pause_resume_reading,
                       "media-playback-pause-symbolic",
                       tr("Pause/Resume reading"), m_menu_translate);
    add_menu_to_menu(action_t::change_tts_model, "text-speak-symbolic", m_menu);
    add_menu_to_menu(action_t::change_tts_model, "text-speak-symbolic",
                     m_menu_translate);
    m_menu.addSeparator();
    m_menu_translate.addSeparator();
    add_action_to_menu(action_t::cancel, "action-unavailable-symbolic",
                       tr("Cancel"), m_menu);
    add_action_to_menu(action_t::cancel, "action-unavailable-symbolic",
                       tr("Cancel"), m_menu_translate);
    m_menu.addSeparator();
    m_menu_translate.addSeparator();
    add_action_to_menu(action_t::toggle_app_window, "view-restore-symbolic",
                       tr("Show/Hide"), m_menu);
    add_action_to_menu(action_t::toggle_app_window, "view-restore-symbolic",
                       tr("Show/Hide"), m_menu_translate);
    add_action_to_menu(action_t::quit, "application-exit-symbolic", tr("Quit"),
                       m_menu);
    add_action_to_menu(action_t::quit, "application-exit-symbolic", tr("Quit"),
                       m_menu_translate);

    std::for_each(m_actions.cbegin(), m_actions.cend(), [this](const auto& p) {
        if (typeid(*p.second) == typeid(QAction)) {
            connect(qobject_cast<QAction*>(p.second), &QAction::triggered, this,
                    [this, action = p.first]() {
                        emit action_triggered(action, 0);
                    });
        }
    });

    update_menu();
}

void tray_icon::set_stt_models(QVariantList&& stt_models) {
    m_stt_models = std::move(stt_models);
    update_menu();
}

void tray_icon::set_active_stt_model(QString&& stt_model,
                                     bool translate_supported) {
    m_active_stt_model = std::move(stt_model);
    m_stt_translate_supported = translate_supported;
    update_menu();
}

void tray_icon::set_tts_models(QVariantList&& tts_models) {
    m_tts_models = std::move(tts_models);
    update_menu();
}

void tray_icon::set_active_tts_model(QString&& tts_model) {
    m_active_tts_model = std::move(tts_model);
    update_menu();
}

void tray_icon::set_fake_keyboard_supported(bool supported) {
    if (m_fake_keyboard_supported != supported) {
        m_fake_keyboard_supported = supported;
        update_menu();
    }
}

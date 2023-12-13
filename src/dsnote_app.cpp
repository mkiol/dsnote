/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dsnote_app.h"

#include <QClipboard>
#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QRegExp>
#include <QTextStream>
#include <algorithm>
#include <iterator>
#include <utility>

#include "downloader.hpp"
#include "media_compressor.hpp"
#include "mtag_tools.hpp"
#include "speech_service.h"

QDebug operator<<(QDebug d, dsnote_app::service_state_t state) {
    switch (state) {
        case dsnote_app::service_state_t::StateBusy:
            d << "busy";
            break;
        case dsnote_app::service_state_t::StateIdle:
            d << "idle";
            break;
        case dsnote_app::service_state_t::StateListeningManual:
            d << "listening-manual";
            break;
        case dsnote_app::service_state_t::StateListeningAuto:
            d << "listening-auto";
            break;
        case dsnote_app::service_state_t::StateListeningSingleSentence:
            d << "listening-single-sentence";
            break;
        case dsnote_app::service_state_t::StateNotConfigured:
            d << "not-configured";
            break;
        case dsnote_app::service_state_t::StateTranscribingFile:
            d << "transcribing-file";
            break;
        case dsnote_app::service_state_t::StatePlayingSpeech:
            d << "playing-speech";
            break;
        case dsnote_app::service_state_t::StateWritingSpeechToFile:
            d << "writing-speech-to-file";
            break;
        case dsnote_app::service_state_t::StateTranslating:
            d << "translating";
            break;
        case dsnote_app::service_state_t::StateUnknown:
            d << "unknown";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::service_task_state_t type) {
    switch (type) {
        case dsnote_app::service_task_state_t::TaskStateIdle:
            d << "idle";
            break;
        case dsnote_app::service_task_state_t::TaskStateProcessing:
            d << "processing";
            break;
        case dsnote_app::service_task_state_t::TaskStateInitializing:
            d << "initializing";
            break;
        case dsnote_app::service_task_state_t::TaskStateSpeechDetected:
            d << "speech-detected";
            break;
        case dsnote_app::service_task_state_t::TaskStateSpeechPlaying:
            d << "speech-playing";
            break;
        case dsnote_app::service_task_state_t::TaskStateSpeechPaused:
            d << "speech-paused";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::error_t type) {
    switch (type) {
        case dsnote_app::error_t::ErrorGeneric:
            d << "generic-error";
            break;
        case dsnote_app::error_t::ErrorFileSource:
            d << "file-source-error";
            break;
        case dsnote_app::error_t::ErrorMicSource:
            d << "mic-source-error";
            break;
        case dsnote_app::error_t::ErrorSttEngine:
            d << "stt-engine-error";
            break;
        case dsnote_app::error_t::ErrorTtsEngine:
            d << "tts-engine-error";
            break;
        case dsnote_app::error_t::ErrorMntEngine:
            d << "mnt-engine-error";
            break;
        case dsnote_app::error_t::ErrorNoService:
            d << "no-service-error";
            break;
        case dsnote_app::error_t::ErrorSaveNoteToFile:
            d << "save-note-to-file-error";
            break;
        case dsnote_app::error_t::ErrorLoadNoteFromFile:
            d << "load-note-from-file-error";
            break;
        case dsnote_app::error_t::ErrorContentDownload:
            d << "content-download-error";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::action_t action) {
    switch (action) {
        case dsnote_app::action_t::start_listening:
            d << "start-listening";
            break;
        case dsnote_app::action_t::start_listening_active_window:
            d << "start-listening-active-window";
            break;
        case dsnote_app::action_t::start_listening_clipboard:
            d << "start-listening-clipboard";
            break;
        case dsnote_app::action_t::stop_listening:
            d << "stop-listening";
            break;
        case dsnote_app::action_t::start_reading:
            d << "start-reading";
            break;
        case dsnote_app::action_t::start_reading_clipboard:
            d << "start-reading-clipboard";
            break;
        case dsnote_app::action_t::pause_resume_reading:
            d << "pause-resume-reading";
            break;
        case dsnote_app::action_t::cancel:
            d << "cancel";
            break;
    }

    return d;
}

dsnote_app::dsnote_app(QObject *parent)
    : QObject{parent},
      m_dbus_service{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                     QDBusConnection::sessionBus()},
      m_dbus_notifications{"org.freedesktop.Notifications",
                           "/org/freedesktop/Notifications",
                           QDBusConnection::sessionBus()} {
    qDebug() << "starting app:" << settings::instance()->launch_mode();

    QDir{cache_dir()}.canonicalPath();

    connect(settings::instance(), &settings::note_changed, this,
            &dsnote_app::handle_note_changed);
    connect(settings::instance(), &settings::speech_mode_changed, this,
            &dsnote_app::update_listen);
    connect(settings::instance(), &settings::mode_changed, this,
            &dsnote_app::update_listen);
    connect(settings::instance(), &settings::translator_mode_changed, this,
            &dsnote_app::handle_translator_settings_changed,
            Qt::QueuedConnection);
    connect(settings::instance(), &settings::translate_when_typing_changed,
            this, &dsnote_app::handle_translator_settings_changed,
            Qt::QueuedConnection);
    connect(settings::instance(), &settings::hotkeys_enabled_changed, this,
            &dsnote_app::register_hotkeys);
    connect(settings::instance(), &settings::hotkeys_changed, this,
            &dsnote_app::register_hotkeys);
    connect(this, &dsnote_app::service_state_changed, this, [this] {
        auto reset_progress = [this]() {
            if (m_transcribe_progress != -1.0) {
                m_transcribe_progress = -1.0;
                emit transcribe_progress_changed();
            }
            if (m_speech_to_file_progress != -1.0) {
                m_speech_to_file_progress = -1.0;
                emit speech_to_file_progress_changed();
            }
        };
        if (service_state() != StateUnknown && service_state() != StateBusy) {
            update_available_stt_models();
            update_active_stt_model();
            update_available_tts_models();
            update_active_tts_model();
            update_available_mnt_langs();
            update_active_mnt_lang();
            update_available_mnt_out_langs();
            update_active_mnt_out_lang();
            update_available_tts_models_for_in_mnt();
            update_available_tts_models_for_out_mnt();
            update_active_tts_model_for_in_mnt();
            update_active_tts_model_for_out_mnt();
            update_current_task();
            update_task_state();
            if (service_state() == StateTranscribingFile ||
                service_state() == StateWritingSpeechToFile) {
                update_progress();
            } else {
                reset_progress();
            }
        } else {
            m_side_task.reset();
            m_primary_task.reset();
            m_current_task = INVALID_TASK;
            reset_progress();
        }
    });
    connect(this, &dsnote_app::active_stt_model_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::available_stt_models_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::active_tts_model_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::available_tts_models_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::available_mnt_langs_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::active_mnt_lang_changed, this,
            &dsnote_app::handle_translator_settings_changed);
    connect(this, &dsnote_app::active_mnt_out_lang_changed, this,
            &dsnote_app::handle_translator_settings_changed);
    connect(this, &dsnote_app::can_open_next_file, this,
            &dsnote_app::open_next_file, Qt::QueuedConnection);
    connect(
        this, &dsnote_app::busy_changed, this,
        [this] {
            if (!m_files_to_open.empty() && !busy())
                m_open_files_delay_timer.start();
        },
        Qt::QueuedConnection);

    m_translator_delay_timer.setSingleShot(true);
    m_translator_delay_timer.setInterval(500);
    connect(&m_translator_delay_timer, &QTimer::timeout, this,
            &dsnote_app::handle_translate_delayed, Qt::QueuedConnection);

    m_open_files_delay_timer.setSingleShot(true);
    m_open_files_delay_timer.setInterval(500);
    connect(&m_open_files_delay_timer, &QTimer::timeout, this,
            &dsnote_app::open_next_file, Qt::QueuedConnection);

    m_action_delay_timer.setSingleShot(true);
    m_action_delay_timer.setInterval(250);
    connect(&m_action_delay_timer, &QTimer::timeout, this,
            &dsnote_app::execute_pending_action, Qt::QueuedConnection);

    m_desktop_notification_delay_timer.setSingleShot(true);
    m_desktop_notification_delay_timer.setInterval(250);
    connect(&m_desktop_notification_delay_timer, &QTimer::timeout, this,
            &dsnote_app::process_pending_desktop_notification,
            Qt::QueuedConnection);
    connect(&m_dbus_notifications,
            &OrgFreedesktopNotificationsInterface::NotificationClosed, this,
            &dsnote_app::handle_desktop_notification_closed);
    connect(&m_dbus_notifications,
            &OrgFreedesktopNotificationsInterface::ActionInvoked, this,
            &dsnote_app::handle_desktop_notification_action_invoked);

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        connect_service_signals();
        update_service_state();
    } else {
        connect(models_manager::instance(), &models_manager::models_changed,
                this, &dsnote_app::request_reload, Qt::QueuedConnection);
        connect(settings::instance(), &settings::models_dir_changed, this,
                &dsnote_app::request_reload, Qt::QueuedConnection);
        connect(settings::instance(), &settings::restore_punctuation_changed,
                this, &dsnote_app::request_reload, Qt::QueuedConnection);

        m_keepalive_timer.setSingleShot(true);
        m_keepalive_timer.setTimerType(Qt::VeryCoarseTimer);
        connect(&m_keepalive_timer, &QTimer::timeout, this,
                &dsnote_app::do_keepalive);
        m_keepalive_timer.start(KEEPALIVE_TIME);

        m_keepalive_current_task_timer.setSingleShot(true);
        m_keepalive_current_task_timer.setTimerType(Qt::VeryCoarseTimer);
        connect(&m_keepalive_current_task_timer, &QTimer::timeout, this,
                &dsnote_app::handle_keepalive_task_timeout);
        connect_service_signals();
        do_keepalive();
    }

    update_available_tts_ref_voices();
    register_hotkeys();

#ifdef USE_DESKTOP
    connect(&m_tray, &QSystemTrayIcon::activated, this,
            [this]([[maybe_unused]] QSystemTrayIcon::ActivationReason reason) {
                emit tray_activated();
            });
    connect(&m_tray, &tray_icon::action_triggered, this,
            &dsnote_app::execute_tray_action);
#endif
}

void dsnote_app::create_player() {
    m_player = std::make_unique<QMediaPlayer>(QObject::parent(),
                                              QMediaPlayer::LowLatency);
    m_player->setNotifyInterval(100);

    connect(
        m_player.get(), &QMediaPlayer::stateChanged, this,
        [this](QMediaPlayer::State state) {
            qDebug() << "player state changed:" << state;
            emit player_playing_changed();
        },
        Qt::QueuedConnection);
    connect(
        m_player.get(), &QMediaPlayer::mediaStatusChanged, this,
        [this](QMediaPlayer::MediaStatus status) {
            qDebug() << "player media status changed:" << status;
            emit player_ready_changed();
        },
        Qt::QueuedConnection);
    connect(
        m_player.get(), &QMediaPlayer::durationChanged, this,
        [this](long long duration) {
            qDebug() << "player duration changed:" << duration;
            emit player_duration_changed();
        },
        Qt::QueuedConnection);
    connect(
        m_player.get(), &QMediaPlayer::positionChanged, this,
        [this](long long position) {
            // qDebug() << "player position changed:" << position;
            emit player_position_changed();

            if (m_player_stop_position != -1 &&
                position >= m_player_stop_position) {
                m_player->pause();
            } else if (m_player_requested_play_position != -1 &&
                       position == 10) {
                m_player->setPosition(m_player_requested_play_position);
            } else if (m_player_requested_play_position == position) {
                m_player->play();
                m_player_requested_play_position = -1;
            }
        },
        Qt::QueuedConnection);
}

void dsnote_app::request_reload() {
    if (settings::instance()->launch_mode() != settings::launch_mode_t::app)
        return;

    if (!m_features_availability.isEmpty()) {
        qDebug() << "[app => dbus] Reload after fau";
        m_service_reload_called = true;
    } else {
        qDebug() << "[app => dbus] Reload";
    }

    auto reply = m_dbus_service.Reload();
    reply.waitForFinished();

    if (reply.argumentAt<0>() != SUCCESS) {
        qWarning() << "failed to reload service, error was returned";
    }
}

void dsnote_app::update_listen() {
    qDebug() << "update listen";
    if (m_files_to_open.empty()) cancel();
}

void dsnote_app::handle_stt_intermediate_text(const QString &text,
                                              const QString &lang, int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
#ifdef DEBUG
        qDebug() << "stt intermediate text decoded:" << text << lang << task;
#else
        qDebug() << "stt intermediate text decoded: ***" << lang << task;
#endif
    } else {
#ifdef DEBUG
        qDebug() << "[dbus => app] signal SttIntermediateTextDecoded:" << text
                 << lang << task;
#else
        qDebug() << "[dbus => app] signal SttIntermediateTextDecoded: ***"
                 << lang << task;
#endif
    }

    if (m_primary_task != task && m_side_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    if (m_intermediate_text != text) {
        m_intermediate_text = text;
        emit intermediate_text_changed();
    }
}

static std::pair<QChar, QString> full_stop(const QString &lang) {
    if (lang.startsWith("zh") || lang.startsWith("ja"))
        return {u'\U00003002', ""};
    return {'.', " "};
}

QString dsnote_app::insert_to_note(QString note, QString new_text,
                                   const QString &lang,
                                   settings::insert_mode_t mode) {
    if (new_text.isEmpty()) return note;

    QTextStream ss{&note, QIODevice::WriteOnly};

    auto [dot, space] = full_stop(lang);

    switch (mode) {
        case settings::insert_mode_t::InsertInLine:
            if (!note.isEmpty()) {
                auto last_char = note.at(note.size() - 1);
                if (last_char.isLetterOrNumber())
                    ss << dot << space;
                else if (last_char == dot)
                    ss << space;
                else if (!last_char.isSpace())
                    ss << ' ';
            }
            break;
        case settings::insert_mode_t::InsertNewLine:
            if (!note.isEmpty()) {
                auto last_char = note.at(note.size() - 1);
                if (last_char.isLetterOrNumber())
                    ss << dot << '\n';
                else
                    ss << '\n';
            }
            break;
    }

    new_text[0] = new_text[0].toUpper();

    ss << new_text;

    if (new_text.at(new_text.size() - 1).isLetterOrNumber()) ss << dot;

    return note;
}

void dsnote_app::handle_stt_text_decoded(const QString &text,
                                         const QString &lang, int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
#ifdef DEBUG
        qDebug() << "stt text decoded:" << text << lang << task;
#else
        qDebug() << "stt text decoded: ***" << lang << task;
#endif
    } else {
#ifdef DEBUG
        qDebug() << "[dbus => app] signal SttTextDecoded:" << text << lang
                 << task;
#else
        qDebug() << "[dbus => app] signal SttTextDecoded: ***" << lang << task;
#endif
    }

    if (m_primary_task != task && m_side_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    switch (m_stt_text_destination) {
        case stt_text_destination_t::note_add:
            make_undo();
            set_note(insert_to_note(settings::instance()->note(), text, lang,
                                    settings::instance()->insert_mode()));
            this->m_intermediate_text.clear();
            emit text_changed();
            emit intermediate_text_changed();
            break;
        case stt_text_destination_t::note_replace:
            make_undo();
            set_note(insert_to_note(QString{}, text, lang,
                                    settings::instance()->insert_mode()));
            m_stt_text_destination = stt_text_destination_t::note_add;
            this->m_intermediate_text.clear();
            emit text_changed();
            emit intermediate_text_changed();
            break;
        case stt_text_destination_t::active_window:
#ifdef USE_DESKTOP
            m_fake_keyboard.emplace();
            m_fake_keyboard->send_text(text);
            emit text_decoded_to_active_window();
#else
            qWarning() << "send to keyboard is not implemented";
#endif
            break;
        case stt_text_destination_t::clipboard:
            QGuiApplication::clipboard()->setText(text);
            emit text_decoded_to_clipboard();
            break;
    }
}

void dsnote_app::handle_tts_partial_speech(const QString &text,
                                         int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
#ifdef DEBUG
        qDebug() << "tts partial speech:" << text << task;
#else
        qDebug() << "tts partial speech: ***" << task;
#endif
    } else {
#ifdef DEBUG
        qDebug() << "[dbus => app] signal TtsPartialSpeechPlaying:" << text
                 << task;
#else
        qDebug() << "[dbus => app] signal TtsPartialSpeechPlaying: ***" << task;
#endif
    }

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    m_intermediate_text = text;

    emit intermediate_text_changed();
}

void dsnote_app::connect_service_signals() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        connect(
            speech_service::instance(), &speech_service::models_changed, this,
            [this] {
                handle_stt_models_changed(
                    speech_service::instance()->available_stt_models());
                handle_tts_models_changed(
                    speech_service::instance()->available_tts_models());
                handle_ttt_models_changed(
                    speech_service::instance()->available_ttt_models());
                handle_mnt_langs_changed(
                    speech_service::instance()->available_mnt_langs());
            },
            Qt::QueuedConnection);
        connect(
            speech_service::instance(), &speech_service::state_changed, this,
            [this] {
                handle_state_changed(
                    static_cast<int>(speech_service::instance()->state()));
            },
            Qt::QueuedConnection);
        connect(
            speech_service::instance(), &speech_service::task_state_changed,
            this,
            [this] {
                handle_task_state_changed(
                    static_cast<int>(speech_service::instance()->task_state()));
            },
            Qt::QueuedConnection);
        connect(
            speech_service::instance(), &speech_service::current_task_changed,
            this,
            [this] {
                handle_current_task_changed(
                    speech_service::instance()->current_task_id());
            },
            Qt::QueuedConnection);
        connect(
            speech_service::instance(),
            &speech_service::default_stt_model_changed, this,
            [this] {
                handle_stt_default_model_changed(
                    speech_service::instance()->default_stt_model());
            },
            Qt::QueuedConnection);
        connect(
            speech_service::instance(),
            &speech_service::default_tts_model_changed, this,
            [this] {
                handle_tts_default_model_changed(
                    speech_service::instance()->default_tts_model());
            },
            Qt::QueuedConnection);
        connect(
            speech_service::instance(),
            &speech_service::default_mnt_lang_changed, this,
            [this] {
                handle_mnt_default_lang_changed(
                    speech_service::instance()->default_mnt_lang());
            },
            Qt::QueuedConnection);
        connect(
            speech_service::instance(),
            &speech_service::default_mnt_out_lang_changed, this,
            [this] {
                handle_mnt_default_out_lang_changed(
                    speech_service::instance()->default_mnt_out_lang());
            },
            Qt::QueuedConnection);
        connect(speech_service::instance(),
                &speech_service::stt_transcribe_file_progress_changed, this,
                &dsnote_app::handle_stt_file_transcribe_progress,
                Qt::QueuedConnection);
        connect(speech_service::instance(),
                &speech_service::stt_file_transcribe_finished, this,
                &dsnote_app::handle_stt_file_transcribe_finished,
                Qt::QueuedConnection);
        connect(speech_service::instance(),
                &speech_service::tts_play_speech_finished, this,
                &dsnote_app::handle_tts_play_speech_finished,
                Qt::QueuedConnection);
        connect(speech_service::instance(),
                &speech_service::tts_speech_to_file_finished, this,
                &dsnote_app::handle_tts_speech_to_file_finished,
                Qt::QueuedConnection);
        connect(speech_service::instance(),
                &speech_service::tts_speech_to_file_progress_changed, this,
                &dsnote_app::handle_tts_speech_to_file_progress,
                Qt::QueuedConnection);
        connect(
            speech_service::instance(), &speech_service::error, this,
            [this](speech_service::error_t code) {
                handle_service_error(static_cast<int>(code));
            },
            Qt::QueuedConnection);
        connect(speech_service::instance(),
                &speech_service::stt_intermediate_text_decoded, this,
                &dsnote_app::handle_stt_intermediate_text,
                Qt::QueuedConnection);
        connect(speech_service::instance(), &speech_service::stt_text_decoded,
                this, &dsnote_app::handle_stt_text_decoded,
                Qt::QueuedConnection);
        connect(speech_service::instance(), &speech_service::tts_partial_speech_playing,
                this, &dsnote_app::handle_tts_partial_speech,
                Qt::QueuedConnection);
        connect(speech_service::instance(),
                &speech_service::mnt_translate_finished, this,
                &dsnote_app::handle_mnt_translate_finished,
                Qt::QueuedConnection);
        connect(
            speech_service::instance(),
            &speech_service::features_availability_updated, this,
            [this] { emit features_availability_updated(); },
            Qt::QueuedConnection);

    } else {
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::SttModelsPropertyChanged, this,
                &dsnote_app::handle_stt_models_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::TtsModelsPropertyChanged, this,
                &dsnote_app::handle_tts_models_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::TttModelsPropertyChanged, this,
                &dsnote_app::handle_ttt_models_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::MntLangsPropertyChanged, this,
                &dsnote_app::handle_mnt_langs_changed);
        connect(&m_dbus_service, &OrgMkiolSpeechInterface::StatePropertyChanged,
                this, &dsnote_app::handle_state_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::TaskStatePropertyChanged, this,
                &dsnote_app::handle_task_state_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::CurrentTaskPropertyChanged, this,
                &dsnote_app::handle_current_task_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::DefaultSttModelPropertyChanged, this,
                &dsnote_app::handle_stt_default_model_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::DefaultTtsModelPropertyChanged, this,
                &dsnote_app::handle_tts_default_model_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::DefaultMntLangPropertyChanged, this,
                &dsnote_app::handle_mnt_default_lang_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::DefaultMntOutLangPropertyChanged,
                this, &dsnote_app::handle_mnt_default_out_lang_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::SttFileTranscribeProgress, this,
                &dsnote_app::handle_stt_file_transcribe_progress);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::SttFileTranscribeFinished, this,
                &dsnote_app::handle_stt_file_transcribe_finished);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::TtsPlaySpeechFinished, this,
                &dsnote_app::handle_tts_play_speech_finished);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::TtsSpeechToFileFinished, this,
                &dsnote_app::handle_tts_speech_to_file_finished);
        connect(&m_dbus_service, &OrgMkiolSpeechInterface::ErrorOccured, this,
                &dsnote_app::handle_service_error);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::SttIntermediateTextDecoded, this,
                &dsnote_app::handle_stt_intermediate_text);
        connect(&m_dbus_service, &OrgMkiolSpeechInterface::SttTextDecoded, this,
                &dsnote_app::handle_stt_text_decoded);
        connect(&m_dbus_service, &OrgMkiolSpeechInterface::TtsPartialSpeechPlaying, this,
                &dsnote_app::handle_tts_partial_speech);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::TtsSpeechToFileProgress, this,
                &dsnote_app::handle_tts_speech_to_file_progress);
        connect(&m_dbus_service, &OrgMkiolSpeechInterface::MntTranslateFinished,
                this, &dsnote_app::handle_mnt_translate_finished);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::FeaturesAvailabilityUpdated, this,
                [this] {
                    features_availability();
                    emit features_availability_updated();
                });
    }
}

void dsnote_app::handle_stt_models_changed(const QVariantMap &models) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        qDebug() << "stt models changed";
    } else {
        qDebug() << "[dbus => app] signal SttModelsPropertyChanged";
    }

    m_available_stt_models_map = models;
    emit available_stt_models_changed();

    update_configured_state();
    update_active_stt_model();
}

void dsnote_app::handle_tts_models_changed(const QVariantMap &models) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        qDebug() << "tts models changed";
    } else {
        qDebug() << "[dbus => app] signal TtsModelsPropertyChanged";
    }

    m_available_tts_models_map = models;
    emit available_tts_models_changed();

    update_configured_state();
    update_active_tts_model();
    update_available_tts_models_for_in_mnt();
    update_available_tts_models_for_out_mnt();
    update_active_tts_model_for_in_mnt();
    update_active_tts_model_for_out_mnt();
}

void dsnote_app::handle_mnt_langs_changed(const QVariantMap &langs) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        qDebug() << "mnt langs changed";
    } else {
        qDebug() << "[dbus => app] signal MntLangsPropertyChanged";
    }

    m_available_mnt_langs_map = langs;
    emit available_mnt_langs_changed();

    update_available_mnt_out_langs();
    update_configured_state();
    update_active_mnt_lang();
    update_active_mnt_out_lang();
}

void dsnote_app::handle_ttt_models_changed(const QVariantMap &models) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        qDebug() << "ttt models changed";
    } else {
        qDebug() << "[dbus => app] signal TttModelsPropertyChanged";
    }

    m_available_ttt_models_map = models;
    emit available_ttt_models_changed();

    update_configured_state();
}

void dsnote_app::handle_state_changed(int status) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal StatusPropertyChanged:" << status;
    }

    auto old_busy = busy();
    auto old_connected = connected();

    auto new_service_state = static_cast<service_state_t>(status);

    if (m_service_state != new_service_state) {
        qDebug() << "app service state:" << m_service_state << "=>"
                 << new_service_state;
        m_service_state = new_service_state;

        if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::app &&
            m_service_state == service_state_t::StateIdle &&
            m_service_reload_called) {
            m_service_reload_update_done = true;
        }

        if (m_service_state == service_state_t::StateIdle) {
            features_availability();
            emit features_availability_updated();
        }

        emit service_state_changed();

        update_configured_state();

        update_tray_state();
    }

    if (old_busy != busy()) {
        qDebug() << "app busy:" << old_busy << "=>" << busy();
        emit busy_changed();
    }

    if (old_connected != connected()) {
        qDebug() << "app connected:" << old_connected << " = > " << connected();
        emit connected_changed();
    }
}

void dsnote_app::handle_task_state_changed(int state) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TaskStatePropertyChanged:" << state;
    }

    if (m_primary_task != m_current_task && m_side_task != m_current_task) {
        qWarning() << "ignore TaskStatePropertyChanged signal";
        return;
    }

    auto new_task_state = [state] {
        switch (state) {
            case 0:
                break;
            case 1:
                return service_task_state_t::TaskStateSpeechDetected;
            case 2:
                return service_task_state_t::TaskStateProcessing;
            case 3:
                return service_task_state_t::TaskStateInitializing;
            case 4:
                return service_task_state_t::TaskStateSpeechPlaying;
            case 5:
                return service_task_state_t::TaskStateSpeechPaused;
        }
        return service_task_state_t::TaskStateIdle;
    }();

    if (m_task_state != new_task_state) {
        qDebug() << "app task state:" << m_task_state << "=>" << new_task_state;
        m_task_state = new_task_state;
        emit task_state_changed();

        update_tray_task_state();
    }
}

void dsnote_app::handle_current_task_changed(int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal CurrentTaskPropertyChanged:" << task;
    }

    auto old = another_app_connected();
    auto old_busy = busy();

    if (m_current_task != task) {
        qDebug() << "app current task:" << m_current_task << "=>" << task;
        m_current_task = task;
    }

    if (old != another_app_connected()) {
        qDebug() << "app another app connected:" << old << "=>"
                 << another_app_connected();
        emit another_app_connected_changed();
    }

    if (old_busy != busy()) {
        qDebug() << "app busy:" << old_busy << "=>" << busy();
        emit busy_changed();
    }

    check_transcribe_taks();
    start_keepalive();
    update_task_state();
}

void dsnote_app::handle_tts_play_speech_finished(int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsPlaySpeechFinished:" << task;
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::handle_tts_speech_to_file_finished(const QString &file,
                                                    int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsSpeechToFileFinished:" << file
                 << task;
    }

    if (m_dest_file.isEmpty()) return;

    if (QFile::exists(m_dest_file)) QFile::remove(m_dest_file);

    if (QFile::copy(file, m_dest_file)) {
        if (settings::instance()->mtag() &&
            settings::audio_format_from_filename(m_dest_file) !=
                settings::audio_format_t::AudioFormatWav) {
            mtag_tools::write(
                /*path=*/m_dest_file.toStdString(),
                /*mtag=*/{
                    /*title=*/m_dest_file_title_tag.toStdString(),
                    /*artist=*/
                    settings::instance()->mtag_artist_name().toStdString(),
                    /*album=*/
                    settings::instance()->mtag_album_name().toStdString(),
                    /*track=*/m_dest_file_track_tag.toInt()});
        }

        QFile::remove(file);
        emit speech_to_file_done();
    }
}

void dsnote_app::handle_tts_speech_to_file_progress(double new_progress,
                                                    int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsSpeechToFileProgress:"
                 << new_progress << task;
    }

    if (m_side_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    m_speech_to_file_progress = new_progress;
    emit speech_to_file_progress_changed();
}

void dsnote_app::handle_mnt_translate_finished(
    [[maybe_unused]] const QString &in_text,
    [[maybe_unused]] const QString &in_lang, const QString &out_text,
    [[maybe_unused]] const QString &out_lang, int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal MntTranslateFinished:" << task;
    }

#ifdef DEBUG
    qDebug() << "translated text:" << out_text;
#endif

    set_translated_text(out_text);
}

void dsnote_app::handle_stt_default_model_changed(const QString &model) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal SttDefaultModelPropertyChanged:"
                 << model;
    }

    set_active_stt_model(model);
}

void dsnote_app::set_active_stt_model(const QString &model) {
    if (m_active_stt_model != model) {
        qDebug() << "app active stt model:" << m_active_stt_model << "=>"
                 << model;
        m_active_stt_model = model;
        emit active_stt_model_changed();
    }
}

void dsnote_app::set_active_mnt_lang(const QString &lang) {
    if (m_active_mnt_lang != lang) {
        qDebug() << "app active mnt lang:" << m_active_mnt_lang << "=>" << lang;
        m_active_mnt_lang = lang;

        update_available_mnt_out_langs();
        update_available_tts_models_for_in_mnt();
        update_active_tts_model_for_in_mnt();

        emit active_mnt_lang_changed();
    }
}

void dsnote_app::set_active_mnt_out_lang(const QString &lang) {
    if (m_active_mnt_out_lang != lang) {
        qDebug() << "app active mnt out lang:" << m_active_mnt_out_lang << "=>"
                 << lang;
        m_active_mnt_out_lang = lang;

        update_available_tts_models_for_out_mnt();
        update_active_tts_model_for_out_mnt();

        emit active_mnt_out_lang_changed();
    }
}

void dsnote_app::handle_tts_default_model_changed(const QString &model) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsDefaultModelPropertyChanged:"
                 << model;
    }

    set_active_tts_model(model);
}

void dsnote_app::handle_mnt_default_lang_changed(const QString &lang) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal MntDefaultLangPropertyChanged:"
                 << lang;
    }

    set_active_mnt_lang(lang);
}

void dsnote_app::handle_mnt_default_out_lang_changed(const QString &lang) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal MntDefaultOutLangPropertyChanged:"
                 << lang;
    }

    set_active_mnt_out_lang(lang);
}

void dsnote_app::set_active_tts_model(const QString &model) {
    if (m_active_tts_model != model) {
        qDebug() << "app active tts model:" << m_active_tts_model << "=>"
                 << model;
        m_active_tts_model = model;
        emit active_tts_model_changed();
    }
}

void dsnote_app::set_active_tts_model_for_in_mnt(const QString &model) {
    if (m_active_tts_model_for_in_mnt != model) {
        qDebug() << "app active tts model for in mnt:"
                 << m_active_tts_model_for_in_mnt << "=>" << model;
        m_active_tts_model_for_in_mnt = model;
        emit active_tts_model_for_in_mnt_changed();
    }
}

void dsnote_app::set_active_tts_model_for_out_mnt(const QString &model) {
    if (m_active_tts_model_for_out_mnt != model) {
        qDebug() << "app active tts model for out mnt:"
                 << m_active_tts_model_for_out_mnt << "=>" << model;
        m_active_tts_model_for_out_mnt = model;
        emit active_tts_model_for_out_mnt_changed();
    }
}

void dsnote_app::handle_stt_file_transcribe_progress(double new_progress,
                                                     int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal SttFileTranscribeProgress:"
                 << new_progress << task;
    }

    if (m_side_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    m_transcribe_progress = new_progress;
    emit transcribe_progress_changed();
}

void dsnote_app::handle_stt_file_transcribe_finished(int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal SttFileTranscribeFinished:" << task;
    }

    if (m_side_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    emit transcribe_done();

    emit can_open_next_file();
}

void dsnote_app::handle_service_error(int code) {
    auto error_code = static_cast<error_t>(code);

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        qDebug() << "service error:" << error_code;
    } else {
        qDebug() << "[dbus => app] signal ErrorOccured:" << code << error_code;
    }

    if (error_code == error_t::ErrorMicSource &&
        settings::instance()->speech_mode() ==
            settings::speech_mode_t::SpeechAutomatic) {
        qWarning() << "switching to manual mode due to error";
        settings::instance()->set_speech_mode(
            settings::speech_mode_t::SpeechManual);
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
    emit error(error_code);

    reset_files_queue();
}

void dsnote_app::check_transcribe_taks() {
    if (m_side_task && m_side_task.current != m_current_task) {
        qDebug() << "transcribe task has finished";
        m_side_task.reset();
    }
}

void dsnote_app::update_progress() {
    double new_stt_progress = 0.0;
    double new_tts_progress = 0.0;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_stt_progress = m_current_task =
            speech_service::instance()->stt_transcribe_file_progress(
                m_side_task.current);
        new_tts_progress = m_current_task =
            speech_service::instance()->tts_speech_to_file_progress(
                m_side_task.current);
    } else {
        qDebug() << "[app => dbus] call SttGetFileTranscribeProgress";
        new_stt_progress =
            m_dbus_service.SttGetFileTranscribeProgress(m_side_task.current);
        qDebug() << "[app => dbus] call TtsGetSpeechToFileProgress";
        new_tts_progress =
            m_dbus_service.TtsGetSpeechToFileProgress(m_side_task.current);
    }

    if (m_transcribe_progress != new_stt_progress) {
        m_transcribe_progress = new_stt_progress;
        emit transcribe_progress_changed();
    }

    if (m_speech_to_file_progress != new_tts_progress) {
        m_speech_to_file_progress = new_tts_progress;
        emit speech_to_file_progress_changed();
    }
}

void dsnote_app::update_current_task() {
    const auto old = another_app_connected();

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        auto new_current_task = speech_service::instance()->current_task_id();

        if (m_current_task != new_current_task) {
            qDebug() << "app current task:" << m_current_task << "=>"
                     << new_current_task;
            m_current_task = new_current_task;
        }
    } else {
        qDebug() << "[app => dbus] get CurrentTask";
        m_current_task = m_dbus_service.currentTask();
    }

    if (old != another_app_connected()) {
        qDebug() << "app another app connected:" << old << "=>"
                 << another_app_connected();
        emit another_app_connected_changed();
    }

    start_keepalive();
}

void dsnote_app::update_service_state() {
    service_state_t new_state;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_state =
            static_cast<service_state_t>(speech_service::instance()->state());
    } else {
        qDebug() << "[app => dbus] get Status";
        new_state = static_cast<service_state_t>(m_dbus_service.state());
    }

    if (m_service_state != new_state) {
        const auto old_busy = busy();
        const auto old_connected = connected();

        qDebug() << "app service state:" << m_service_state << "=>"
                 << new_state;

        m_service_state = new_state;
        emit service_state_changed();

        update_configured_state();

        if (old_busy != busy()) emit busy_changed();
        if (old_connected != connected()) emit connected_changed();

        update_tray_state();
    }
}

void dsnote_app::update_active_stt_lang_idx() {
    auto it = m_available_stt_models_map.find(active_stt_model());

    if (it == m_available_stt_models_map.end()) {
        set_active_stt_model_idx(-1);
    } else {
        set_active_stt_model_idx(
            std::distance(m_available_stt_models_map.begin(), it));
    }
}

void dsnote_app::update_active_tts_lang_idx() {
    auto it = m_available_tts_models_map.find(active_tts_model());

    if (it == m_available_tts_models_map.end()) {
        set_active_tts_model_idx(-1);
    } else {
        set_active_tts_model_idx(
            std::distance(m_available_tts_models_map.begin(), it));
    }
}

void dsnote_app::update_active_mnt_lang_idx() {
    auto it = m_available_mnt_langs_map.find(active_mnt_lang());

    if (it == m_available_mnt_langs_map.end()) {
        set_active_mnt_lang_idx(-1);
    } else {
        set_active_mnt_lang_idx(
            std::distance(m_available_mnt_langs_map.begin(), it));
    }
}

void dsnote_app::update_active_mnt_out_lang_idx() {
    auto it = m_available_mnt_out_langs_map.find(active_mnt_out_lang());

    if (it == m_available_mnt_out_langs_map.end()) {
        set_active_mnt_out_lang_idx(-1);
    } else {
        set_active_mnt_out_lang_idx(
            std::distance(m_available_mnt_out_langs_map.begin(), it));
    }
}

void dsnote_app::update_available_stt_models() {
    QVariantMap new_available_stt_models_map{};

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_available_stt_models_map =
            speech_service::instance()->available_stt_models();
    } else {
        qDebug() << "[app => dbus] get SttModels";
        new_available_stt_models_map = m_dbus_service.sttModels();
    }

    if (m_available_stt_models_map != new_available_stt_models_map) {
        qDebug() << "app stt available models:"
                 << m_available_stt_models_map.size() << "=>"
                 << new_available_stt_models_map.size();
        m_available_stt_models_map = std::move(new_available_stt_models_map);
        emit available_stt_models_changed();
    }
}

void dsnote_app::update_available_tts_models() {
    QVariantMap new_available_tts_models_map{};

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_available_tts_models_map =
            speech_service::instance()->available_tts_models();
    } else {
        qDebug() << "[app => dbus] get TtsModels";
        new_available_tts_models_map = m_dbus_service.ttsModels();
    }

    if (m_available_tts_models_map != new_available_tts_models_map) {
        qDebug() << "app tts available models:"
                 << m_available_tts_models_map.size() << "=>"
                 << new_available_tts_models_map.size();
        m_available_tts_models_map = std::move(new_available_tts_models_map);
        emit available_tts_models_changed();
    }
}

void dsnote_app::update_available_tts_ref_voices() {
    QVariantMap new_available_tts_ref_voices_map{};

    const auto ref_voices_dir =
        QDir{QStandardPaths::writableLocation(QStandardPaths::DataLocation)}
            .filePath(s_ref_voices_dir_name);

    QDir dir{ref_voices_dir};

    if (!dir.exists()) QDir::root().mkpath(dir.absolutePath());

    dir.setNameFilters(QStringList{} << "*.mp3"
                                     << "*.ogg"
                                     << "*.opus"
                                     << "*.flac");
    dir.setFilter(QDir::Files);

    for (const auto &file : std::as_const(dir).entryList()) {
        auto path = dir.absoluteFilePath(file);
        auto mtag = mtag_tools::read(path.toStdString());
        auto name = mtag && !mtag->title.empty()
                        ? QString::fromStdString(mtag->title)
                        : QFileInfo{file}.baseName();
        new_available_tts_ref_voices_map.insert(
            file, QStringList{std::move(name), std::move(path)});
    }

    if (new_available_tts_ref_voices_map.contains(
            settings::instance()->active_tts_ref_voice())) {
        m_active_tts_ref_voice = settings::instance()->active_tts_ref_voice();
    } else {
        if (new_available_tts_ref_voices_map.isEmpty()) {
            m_active_tts_ref_voice.clear();
        } else {
            m_active_tts_ref_voice =
                new_available_tts_ref_voices_map.firstKey();
        }

        settings::instance()->set_active_tts_ref_voice(m_active_tts_ref_voice);
    }

    if (new_available_tts_ref_voices_map.contains(
            settings::instance()->active_tts_for_in_mnt_ref_voice())) {
        m_active_tts_for_in_mnt_ref_voice =
            settings::instance()->active_tts_for_in_mnt_ref_voice();
    } else {
        if (new_available_tts_ref_voices_map.isEmpty()) {
            m_active_tts_for_in_mnt_ref_voice.clear();
        } else {
            m_active_tts_for_in_mnt_ref_voice =
                new_available_tts_ref_voices_map.firstKey();
        }

        settings::instance()->set_active_tts_for_in_mnt_ref_voice(
            m_active_tts_for_in_mnt_ref_voice);
    }

    if (new_available_tts_ref_voices_map.contains(
            settings::instance()->active_tts_for_out_mnt_ref_voice())) {
        m_active_tts_for_out_mnt_ref_voice =
            settings::instance()->active_tts_for_out_mnt_ref_voice();
    } else {
        if (new_available_tts_ref_voices_map.isEmpty()) {
            m_active_tts_for_out_mnt_ref_voice.clear();
        } else {
            m_active_tts_for_out_mnt_ref_voice =
                new_available_tts_ref_voices_map.firstKey();
        }

        settings::instance()->set_active_tts_for_out_mnt_ref_voice(
            m_active_tts_for_out_mnt_ref_voice);
    }

    m_available_tts_ref_voices_map =
        std::move(new_available_tts_ref_voices_map);

    emit available_tts_ref_voices_changed();
    emit active_tts_ref_voice_changed();
    emit active_tts_for_in_mnt_ref_voice_changed();
    emit active_tts_for_out_mnt_ref_voice_changed();
}

void dsnote_app::update_available_tts_models_for_in_mnt() {
    QVariantMap new_available_tts_models_for_in_mnt_map{};

    auto lpp = m_active_mnt_lang + "_";
    for (auto it = m_available_tts_models_map.cbegin();
         it != m_available_tts_models_map.cend(); ++it) {
        if (it.key().startsWith(lpp))
            new_available_tts_models_for_in_mnt_map.insert(it.key(),
                                                           it.value());
    }

    if (m_available_tts_models_for_in_mnt_map !=
        new_available_tts_models_for_in_mnt_map) {
        qDebug() << "app tts available models for in mnt:"
                 << m_available_tts_models_for_in_mnt_map.size() << "=>"
                 << new_available_tts_models_for_in_mnt_map.size();
        m_available_tts_models_for_in_mnt_map =
            std::move(new_available_tts_models_for_in_mnt_map);
        emit available_tts_models_for_in_mnt_changed();
    }
}

void dsnote_app::update_available_tts_models_for_out_mnt() {
    QVariantMap new_available_tts_models_for_out_mnt_map{};

    auto lpp = m_active_mnt_out_lang + "_";
    for (auto it = m_available_tts_models_map.cbegin();
         it != m_available_tts_models_map.cend(); ++it) {
        if (it.key().startsWith(lpp))
            new_available_tts_models_for_out_mnt_map.insert(it.key(),
                                                            it.value());
    }

    if (m_available_tts_models_for_out_mnt_map !=
        new_available_tts_models_for_out_mnt_map) {
        qDebug() << "app tts available models for out mnt:"
                 << m_available_tts_models_for_out_mnt_map.size() << "=>"
                 << new_available_tts_models_for_out_mnt_map.size();
        m_available_tts_models_for_out_mnt_map =
            std::move(new_available_tts_models_for_out_mnt_map);
        emit available_tts_models_for_out_mnt_changed();
    }
}

void dsnote_app::update_available_mnt_langs() {
    QVariantMap new_available_mnt_langs_map{};

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_available_mnt_langs_map =
            speech_service::instance()->available_mnt_langs();
    } else {
        qDebug() << "[app => dbus] get MntLangs";
        new_available_mnt_langs_map = m_dbus_service.mntLangs();
    }

    if (m_available_mnt_langs_map != new_available_mnt_langs_map) {
        qDebug() << "app mnt available langs:"
                 << m_available_mnt_langs_map.size() << "=>"
                 << new_available_mnt_langs_map.size();
        m_available_mnt_langs_map = std::move(new_available_mnt_langs_map);
        emit available_mnt_langs_changed();
        update_available_mnt_out_langs();
    }
}

void dsnote_app::update_available_mnt_out_langs() {
    QVariantMap new_available_mnt_out_langs_map{};

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_available_mnt_out_langs_map =
            speech_service::instance()->mnt_out_langs(m_active_mnt_lang);
    } else {
        qDebug() << "[app => dbus] call MntGetOutLangs";
        new_available_mnt_out_langs_map =
            m_dbus_service.MntGetOutLangs(m_active_mnt_lang);
    }

    if (m_available_mnt_out_langs_map != new_available_mnt_out_langs_map) {
        qDebug() << "app mnt available out langs:"
                 << m_available_mnt_out_langs_map.size() << "=>"
                 << new_available_mnt_out_langs_map.size();
        m_available_mnt_out_langs_map =
            std::move(new_available_mnt_out_langs_map);
        emit available_mnt_out_langs_changed();
    }
}

QVariantList dsnote_app::available_stt_models() const {
    QVariantList list;

    for (auto it = m_available_stt_models_map.constBegin();
         it != m_available_stt_models_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

QVariantList dsnote_app::available_tts_models() const {
    QVariantList list;

    for (auto it = m_available_tts_models_map.constBegin();
         it != m_available_tts_models_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

QVariantList dsnote_app::available_tts_ref_voices() const {
    QVariantList list;

    for (auto it = m_available_tts_ref_voices_map.constBegin();
         it != m_available_tts_ref_voices_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (!v.isEmpty()) list.push_back(v.front());
    }

    return list;
}

void dsnote_app::delete_tts_ref_voice(int idx) {
    auto item = std::next(m_available_tts_ref_voices_map.cbegin(), idx).value();

    auto list = item.toStringList();
    if (list.size() < 2) return;

    QFile{list.at(1)}.remove();

    update_available_tts_ref_voices();
}

void dsnote_app::rename_tts_ref_voice(int idx, const QString &new_name) {
    auto &item = std::next(m_available_tts_ref_voices_map.begin(), idx).value();

    auto list = item.toStringList();
    if (list.size() < 2) return;

    auto file_path_std = list.at(1).toStdString();
    auto mtag = mtag_tools::read(file_path_std);

    if (!mtag) return;

    if (mtag->title == new_name.toStdString()) return;

    mtag->title.assign(new_name.toStdString());

    if (!mtag_tools::write(file_path_std, *mtag)) return;

    list[0] = new_name;
    item = list;

    emit available_tts_ref_voices_changed();
    emit active_tts_ref_voice_changed();
    emit active_tts_for_in_mnt_ref_voice_changed();
    emit active_tts_for_out_mnt_ref_voice_changed();
}

QVariantList dsnote_app::available_tts_models_for_in_mnt() const {
    QVariantList list;

    for (auto it = m_available_tts_models_for_in_mnt_map.constBegin();
         it != m_available_tts_models_for_in_mnt_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

QVariantList dsnote_app::available_tts_models_for_out_mnt() const {
    QVariantList list;

    for (auto it = m_available_tts_models_for_out_mnt_map.constBegin();
         it != m_available_tts_models_for_out_mnt_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

QVariantList dsnote_app::available_mnt_langs() const {
    QVariantList list;

    for (auto it = m_available_mnt_langs_map.constBegin();
         it != m_available_mnt_langs_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

QVariantList dsnote_app::available_mnt_out_langs() const {
    QVariantList list;

    for (auto it = m_available_mnt_out_langs_map.constBegin();
         it != m_available_mnt_out_langs_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

void dsnote_app::set_active_stt_model_idx(int idx) {
    if (active_stt_model_idx() != idx && idx > -1 &&
        idx < m_available_stt_models_map.size()) {
        auto id = std::next(m_available_stt_models_map.cbegin(), idx).key();

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->set_default_stt_model(id);
        } else {
            qDebug() << "[app => dbus] set DefaultSttModel:" << idx << id;
            m_dbus_service.setDefaultSttModel(id);
        }
    }
}

void dsnote_app::set_active_tts_model_idx(int idx) {
    if (active_tts_model_idx() != idx && idx > -1 &&
        idx < m_available_tts_models_map.size()) {
        auto id = std::next(m_available_tts_models_map.cbegin(), idx).key();

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->set_default_tts_model(id);
        } else {
            qDebug() << "[app => dbus] set DefaultTtsModel:" << idx << id;
            m_dbus_service.setDefaultTtsModel(id);
        }
    }
}

void dsnote_app::set_active_tts_ref_voice_idx(int idx) {
    if (active_tts_ref_voice_idx() != idx && idx > -1 &&
        idx < m_available_tts_ref_voices_map.size()) {
        auto id = std::next(m_available_tts_ref_voices_map.cbegin(), idx).key();
        settings::instance()->set_active_tts_ref_voice(id);
        m_active_tts_ref_voice = id;
        emit active_tts_ref_voice_changed();
    }
}

void dsnote_app::set_active_tts_for_in_mnt_ref_voice_idx(int idx) {
    if (active_tts_for_in_mnt_ref_voice_idx() != idx && idx > -1 &&
        idx < m_available_tts_ref_voices_map.size()) {
        auto id = std::next(m_available_tts_ref_voices_map.cbegin(), idx).key();
        settings::instance()->set_active_tts_for_in_mnt_ref_voice(id);
        m_active_tts_for_in_mnt_ref_voice = id;
        emit active_tts_for_in_mnt_ref_voice_changed();
    }
}

void dsnote_app::set_active_tts_for_out_mnt_ref_voice_idx(int idx) {
    if (active_tts_for_out_mnt_ref_voice_idx() != idx && idx > -1 &&
        idx < m_available_tts_ref_voices_map.size()) {
        auto id = std::next(m_available_tts_ref_voices_map.cbegin(), idx).key();
        settings::instance()->set_active_tts_for_out_mnt_ref_voice(id);
        m_active_tts_for_out_mnt_ref_voice = id;
        emit active_tts_for_out_mnt_ref_voice_changed();
    }
}

void dsnote_app::set_active_tts_model_for_in_mnt_idx(int idx) {
    if (m_active_mnt_lang.isEmpty()) {
        qWarning() << "invalid active mnt lang";
        return;
    }

    if (active_tts_model_for_in_mnt_idx() != idx && idx > -1 &&
        idx < m_available_tts_models_for_in_mnt_map.size()) {
        auto id = std::next(m_available_tts_models_for_in_mnt_map.cbegin(), idx)
                      .key();

        qDebug() << "new active tts model for in mnt:" << id;

        settings::instance()->set_default_tts_model_for_mnt_lang(
            m_active_mnt_lang, id);

        update_active_tts_model_for_in_mnt();
    }
}

void dsnote_app::set_active_tts_model_for_out_mnt_idx(int idx) {
    if (m_active_mnt_out_lang.isEmpty()) {
        qWarning() << "invalid active mnt out lang";
        return;
    }

    if (active_tts_model_for_out_mnt_idx() != idx && idx > -1 &&
        idx < m_available_tts_models_for_out_mnt_map.size()) {
        auto id =
            std::next(m_available_tts_models_for_out_mnt_map.cbegin(), idx)
                .key();

        qDebug() << "new active tts model for out mnt:" << id;

        settings::instance()->set_default_tts_model_for_mnt_lang(
            m_active_mnt_out_lang, id);

        update_active_tts_model_for_out_mnt();
    }
}

void dsnote_app::set_active_mnt_lang_idx(int idx) {
    if (active_mnt_lang_idx() != idx && idx > -1 &&
        idx < m_available_mnt_langs_map.size()) {
        auto id = std::next(m_available_mnt_langs_map.cbegin(), idx).key();

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->set_default_mnt_lang(id);
        } else {
            qDebug() << "[app => dbus] set DefaultMntLang:" << idx << id;
            m_dbus_service.setDefaultMntLang(id);
        }
    }
}

void dsnote_app::set_active_mnt_out_lang_idx(int idx) {
    if (active_mnt_out_lang_idx() != idx && idx > -1 &&
        idx < m_available_mnt_out_langs_map.size()) {
        auto id = std::next(m_available_mnt_out_langs_map.cbegin(), idx).key();

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->set_default_mnt_out_lang(id);
        } else {
            qDebug() << "[app => dbus] set DefaultMntOutLang:" << idx << id;
            m_dbus_service.setDefaultMntOutLang(id);
        }
    }
}

void dsnote_app::cancel() {
    if (busy()) return;

    if (!m_open_files_delay_timer.isActive()) reset_files_queue();

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        if (m_side_task) {
            speech_service::instance()->cancel(m_side_task.current);
            m_side_task.reset();
        } else if (m_primary_task) {
            if (m_primary_task.current != INVALID_TASK) {
                speech_service::instance()->cancel(m_primary_task.current);
                m_primary_task.reset();
            } else {
                speech_service::instance()->cancel(m_primary_task.previous);
            }
        }
    } else {
        qDebug() << "[app => dbus] call Cancel";

        if (m_side_task) {
            m_dbus_service.Cancel(m_side_task.current);
            m_side_task.reset();
        } else if (m_primary_task) {
            if (m_primary_task.current != INVALID_TASK) {
                m_dbus_service.Cancel(m_primary_task.current);
                m_primary_task.reset();
            } else {
                m_dbus_service.Cancel(m_primary_task.previous);
            }
        }
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::transcribe_file(const QString &source_file, bool replace) {
    m_stt_text_destination = replace ? stt_text_destination_t::note_replace
                                     : stt_text_destination_t::note_add;

    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_transcribe_file(
            source_file, {},
            s->whisper_translate() ? QStringLiteral("en") : QString{});
    } else {
        qDebug() << "[app => dbus] call SttTranscribeFile";

        new_task = m_dbus_service.SttTranscribeFile(
            source_file, {},
            s->whisper_translate() ? QStringLiteral("en") : QString{});
    }

    m_side_task.set(new_task);
}

void dsnote_app::transcribe_file_url(const QUrl &source_file, bool replace) {
    transcribe_file(source_file.toLocalFile(), replace);
}

void dsnote_app::listen() {
    m_stt_text_destination = stt_text_destination_t::note_add;
    listen_internal();
}

void dsnote_app::listen_to_active_window() {
    m_stt_text_destination = stt_text_destination_t::active_window;
    listen_internal();
}

void dsnote_app::listen_to_clipboard() {
    m_stt_text_destination = stt_text_destination_t::clipboard;
    listen_internal();
}

void dsnote_app::listen_internal() {
    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_start_listen(
            static_cast<speech_service::speech_mode_t>(s->speech_mode()), {},
            s->whisper_translate() ? QStringLiteral("en") : QString{});
    } else {
        qDebug() << "[app => dbus] call SttStartListen:" << s->speech_mode();
        new_task = m_dbus_service.SttStartListen(
            static_cast<int>(s->speech_mode()), {},
            s->whisper_translate() ? QStringLiteral("en") : QString{});
    }

    m_primary_task.set(new_task);

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::stop_listen() {
    if (m_primary_task) {
        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->stt_stop_listen(m_primary_task.current);
        } else {
            qDebug() << "[app => dbus] call SttStopListen";
            m_dbus_service.SttStopListen(m_primary_task.current);
        }

        m_primary_task.reset();
    }
}

void dsnote_app::pause_speech() {
    if (!m_primary_task) return;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        if (m_primary_task.current != INVALID_TASK) {
            speech_service::instance()->tts_pause_speech(
                m_primary_task.current);
        } else {
            speech_service::instance()->tts_pause_speech(
                m_primary_task.previous);
        }
    } else {
        qDebug() << "[app => dbus] call TtsPauseSpeech";

        if (m_primary_task.current != INVALID_TASK) {
            m_dbus_service.TtsPauseSpeech(m_primary_task.current);
        } else {
            m_dbus_service.TtsPauseSpeech(m_primary_task.previous);
        }
    }
}

void dsnote_app::resume_speech() {
    if (!m_primary_task) return;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        if (m_primary_task.current != INVALID_TASK) {
            speech_service::instance()->tts_resume_speech(
                m_primary_task.current);
        } else {
            speech_service::instance()->tts_resume_speech(
                m_primary_task.previous);
        }
    } else {
        qDebug() << "[app => dbus] call TtsResumeSpeech";

        if (m_primary_task.current != INVALID_TASK) {
            m_dbus_service.TtsResumeSpeech(m_primary_task.current);
        } else {
            m_dbus_service.TtsResumeSpeech(m_primary_task.previous);
        }
    }
}

void dsnote_app::play_speech_internal(const QString &text,
                                      const QString &model_id,
                                      const QString &ref_voice) {
    if (text.isEmpty()) {
        qWarning() << "text is empty";
        return;
    }

    int new_task = 0;

    QVariantMap options;
    options.insert("speech_speed", settings::instance()->speech_speed());

    if (m_available_tts_ref_voices_map.contains(ref_voice)) {
        auto l = m_available_tts_ref_voices_map.value(ref_voice).toStringList();
        if (l.size() > 1) options.insert("ref_voice_file", l.at(1));
    }

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->tts_play_speech(text, model_id,
                                                               options);
    } else {
        qDebug() << "[app => dbus] call TtsPlaySpeech";

        new_task = m_dbus_service.TtsPlaySpeech2(text, model_id, options);
    }

    m_primary_task.set(new_task);

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::play_speech() {
    play_speech_internal(
        note(), {},
        tts_ref_voice_needed() ? active_tts_ref_voice() : QString{});
}

void dsnote_app::play_speech_from_clipboard() {
    play_speech_internal(
        QGuiApplication::clipboard()->text(), {},
        tts_ref_voice_needed() ? active_tts_ref_voice() : QString{});
}

void dsnote_app::play_speech_translator(bool transtalated) {
    if (!transtalated && m_active_tts_model_for_in_mnt.isEmpty()) {
        qWarning() << "no active tts model for in mnt";
        return;
    }

    if (transtalated && m_active_tts_model_for_out_mnt.isEmpty()) {
        qWarning() << "no active tts model for out mnt";
        return;
    }

    play_speech_internal(transtalated ? m_translated_text : note(),
                         transtalated ? m_active_tts_model_for_out_mnt
                                      : m_active_tts_model_for_in_mnt,
                         transtalated ? tts_for_out_mnt_ref_voice_needed()
                                            ? active_tts_for_out_mnt_ref_voice()
                                            : QString{}
                         : tts_for_in_mnt_ref_voice_needed()
                             ? active_tts_for_in_mnt_ref_voice()
                             : QString{});
}

void dsnote_app::translate_delayed() {
    if (settings::instance()->translator_mode())
        m_translator_delay_timer.start();
}

void dsnote_app::handle_translate_delayed() {
    if (service_state() == service_state_t::StateIdle)
        translate();
    else
        translate_delayed();
}

void dsnote_app::translate() {
    if (m_active_mnt_lang.isEmpty() || m_active_mnt_out_lang.isEmpty()) {
        qWarning() << "invalid active mnt lang";
        return;
    }

    if (note().isEmpty()) {
        set_translated_text({});
    } else {
        int new_task = 0;

        QVariantMap options;
        options.insert("clean_text", settings::instance()->mnt_clean_text());

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            new_task = speech_service::instance()->mnt_translate(
                note(), m_active_mnt_lang, m_active_mnt_out_lang, options);
        } else {
            qDebug() << "[app => dbus] call MntTranslate";

            new_task = m_dbus_service.MntTranslate2(
                note(), m_active_mnt_lang, m_active_mnt_out_lang, options);
        }

        m_primary_task.set(new_task);
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::speech_to_file_url(const QUrl &dest_file,
                                    const QString &title_tag,
                                    const QString &track_tag) {
    speech_to_file(dest_file.toLocalFile(), title_tag, track_tag);
}

void dsnote_app::speech_to_file(const QString &dest_file,
                                const QString &title_tag,
                                const QString &track_tag) {
    speech_to_file_internal(
        note(), {}, dest_file, title_tag, track_tag,
        tts_ref_voice_needed() ? active_tts_ref_voice() : QString{});
}

void dsnote_app::speech_to_file_translator_url(bool transtalated,
                                               const QUrl &dest_file,
                                               const QString &title_tag,
                                               const QString &track_tag) {
    speech_to_file_translator(transtalated, dest_file.toLocalFile(), title_tag,
                              track_tag);
}

void dsnote_app::speech_to_file_translator(bool transtalated,
                                           const QString &dest_file,
                                           const QString &title_tag,
                                           const QString &track_tag) {
    if (!transtalated && m_active_tts_model_for_in_mnt.isEmpty()) {
        qWarning() << "no active tts model for in mnt";
        return;
    }

    if (transtalated && m_active_tts_model_for_out_mnt.isEmpty()) {
        qWarning() << "no active tts model for out mnt";
        return;
    }

    speech_to_file_internal(
        transtalated ? m_translated_text : note(),
        transtalated ? m_active_tts_model_for_out_mnt
                     : m_active_tts_model_for_in_mnt,
        dest_file, title_tag, track_tag,
        transtalated                        ? tts_for_out_mnt_ref_voice_needed()
                                                  ? active_tts_for_out_mnt_ref_voice()
                                                  : QString{}
                               : tts_for_in_mnt_ref_voice_needed() ? active_tts_for_in_mnt_ref_voice()
                                            : QString{});
}

static QString audio_quality_to_str(settings::audio_quality_t quality) {
    switch (quality) {
        case settings::audio_quality_t::AudioQualityVbrHigh:
            return QStringLiteral("vbr_high");
        case settings::audio_quality_t::AudioQualityVbrMedium:
            return QStringLiteral("vbr_medium");
        case settings::audio_quality_t::AudioQualityVbrLow:
            return QStringLiteral("vbr_low");
    }

    return QStringLiteral("vbr_medium");
}

void dsnote_app::speech_to_file_internal(const QString &text,
                                         const QString &model_id,
                                         const QString &dest_file,
                                         const QString &title_tag,
                                         const QString &track_tag,
                                         const QString &ref_voice) {
    if (text.isEmpty()) {
        qWarning() << "text is empty";
        return;
    }

    if (dest_file.isEmpty()) {
        qWarning() << "dest file is empty";
        return;
    }

    int new_task = 0;

    QVariantMap options;
    options.insert("speech_speed", settings::instance()->speech_speed());

    if (m_available_tts_ref_voices_map.contains(ref_voice)) {
        auto l = m_available_tts_ref_voices_map.value(ref_voice).toStringList();
        if (l.size() > 1) options.insert("ref_voice_file", l.at(1));
    }

    auto audio_format_str = settings::audio_format_str_from_filename(dest_file);
    auto audio_ext = settings::audio_ext_from_filename(dest_file);

    options.insert("audio_format", audio_format_str);
    options.insert("audio_quality",
                   audio_quality_to_str(settings::instance()->audio_quality()));

    if (QFileInfo{dest_file}.suffix().toLower() != audio_ext) {
        qDebug() << "file name doesn't have proper extension for audio format";
        if (dest_file.endsWith('.'))
            m_dest_file = dest_file + audio_ext;
        else
            m_dest_file = dest_file + '.' + audio_ext;
    } else {
        m_dest_file = dest_file;
    }

    m_dest_file_title_tag = title_tag;
    m_dest_file_track_tag = track_tag;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->tts_speech_to_file(
            text, model_id, options);
    } else {
        qDebug() << "[app => dbus] call TtsSpeechToFile";

        new_task = m_dbus_service.TtsSpeechToFile(text, model_id, options);
    }

    m_side_task.set(new_task);
}

void dsnote_app::stop_play_speech() {}

void dsnote_app::set_task_state(service_task_state_t new_state) {
    if (m_task_state != new_state) {
        qDebug() << "app speech state:" << m_task_state << "=>" << new_state;
        m_task_state = new_state;
        emit task_state_changed();

        update_tray_task_state();
    }
}

void dsnote_app::update_task_state() {
    if (m_primary_task != m_current_task) {
        qWarning() << "invalid task, reseting task state";
        set_task_state(service_task_state_t::TaskStateIdle);
        return;
    }

    auto new_value = [&] {
        int task_state = 0;
        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            task_state = speech_service::instance()->task_state();
        } else {
            qDebug() << "[app => dbus] get TaskState";
            task_state = m_dbus_service.taskState();
        }

        switch (task_state) {
            case 0:
                return service_task_state_t::TaskStateIdle;
            case 1:
                return service_task_state_t::TaskStateSpeechDetected;
            case 2:
                return service_task_state_t::TaskStateProcessing;
            case 3:
                return service_task_state_t::TaskStateInitializing;
            case 4:
                return service_task_state_t::TaskStateSpeechPlaying;
        }
        return service_task_state_t::TaskStateIdle;
    }();

    set_task_state(new_value);
}

int dsnote_app::active_stt_model_idx() const {
    auto it = m_available_stt_models_map.find(m_active_stt_model);
    if (it == m_available_stt_models_map.end()) {
        return -1;
    }
    return std::distance(m_available_stt_models_map.begin(), it);
}

QString dsnote_app::active_stt_model_name() const {
    auto it = m_available_stt_models_map.find(m_active_stt_model);
    if (it == m_available_stt_models_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.size() > 1 ? l.at(1) : QString{};
}

QString dsnote_app::active_tts_model_name() const {
    auto it = m_available_tts_models_map.find(m_active_tts_model);
    if (it == m_available_tts_models_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.size() > 1 ? l.at(1) : QString{};
}

QString dsnote_app::active_tts_ref_voice_name() const {
    auto it = m_available_tts_ref_voices_map.find(m_active_tts_ref_voice);
    if (it == m_available_tts_ref_voices_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.isEmpty() ? QString{} : l.front();
}

QString dsnote_app::active_tts_for_in_mnt_ref_voice_name() const {
    auto it =
        m_available_tts_ref_voices_map.find(m_active_tts_for_in_mnt_ref_voice);
    if (it == m_available_tts_ref_voices_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.isEmpty() ? QString{} : l.front();
}

QString dsnote_app::active_tts_for_out_mnt_ref_voice_name() const {
    auto it =
        m_available_tts_ref_voices_map.find(m_active_tts_for_out_mnt_ref_voice);
    if (it == m_available_tts_ref_voices_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.isEmpty() ? QString{} : l.front();
}

QString dsnote_app::active_tts_model_for_in_mnt_name() const {
    auto it = m_available_tts_models_for_in_mnt_map.find(
        m_active_tts_model_for_in_mnt);
    if (it == m_available_tts_models_for_in_mnt_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.size() > 1 ? l.at(1) : QString{};
}

QString dsnote_app::active_tts_model_for_out_mnt_name() const {
    auto it = m_available_tts_models_for_out_mnt_map.find(
        m_active_tts_model_for_out_mnt);
    if (it == m_available_tts_models_for_out_mnt_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.size() > 1 ? l.at(1) : QString{};
}

QString dsnote_app::active_mnt_lang_name() const {
    auto it = m_available_mnt_langs_map.find(m_active_mnt_lang);
    if (it == m_available_mnt_langs_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.size() > 1 ? l.at(1) : QString{};
}

QString dsnote_app::active_mnt_out_lang_name() const {
    auto it = m_available_mnt_out_langs_map.find(m_active_mnt_out_lang);
    if (it == m_available_mnt_out_langs_map.end()) {
        return {};
    }

    auto l = it.value().toStringList();

    return l.size() > 1 ? l.at(1) : QString{};
}

void dsnote_app::update_active_stt_model() {
    QString new_stt_model;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_stt_model = speech_service::instance()->default_stt_model();
    } else {
        qDebug() << "[app => dbus] get DefaultSttModel";
        new_stt_model = m_dbus_service.defaultSttModel();
    }

    set_active_stt_model(new_stt_model);
}

int dsnote_app::active_tts_model_idx() const {
    auto it = m_available_tts_models_map.find(m_active_tts_model);
    if (it == m_available_tts_models_map.end()) {
        return -1;
    }
    return std::distance(m_available_tts_models_map.begin(), it);
}

int dsnote_app::active_tts_ref_voice_idx() const {
    auto it = m_available_tts_ref_voices_map.find(m_active_tts_ref_voice);
    if (it == m_available_tts_ref_voices_map.end()) {
        return -1;
    }
    return std::distance(m_available_tts_ref_voices_map.begin(), it);
}

int dsnote_app::active_tts_for_in_mnt_ref_voice_idx() const {
    auto it =
        m_available_tts_ref_voices_map.find(m_active_tts_for_in_mnt_ref_voice);
    if (it == m_available_tts_ref_voices_map.end()) {
        return -1;
    }
    return std::distance(m_available_tts_ref_voices_map.begin(), it);
}

int dsnote_app::active_tts_for_out_mnt_ref_voice_idx() const {
    auto it =
        m_available_tts_ref_voices_map.find(m_active_tts_for_out_mnt_ref_voice);
    if (it == m_available_tts_ref_voices_map.end()) {
        return -1;
    }
    return std::distance(m_available_tts_ref_voices_map.begin(), it);
}

int dsnote_app::active_tts_model_for_in_mnt_idx() const {
    auto it = m_available_tts_models_for_in_mnt_map.find(
        m_active_tts_model_for_in_mnt);
    if (it == m_available_tts_models_for_in_mnt_map.end()) {
        return -1;
    }
    return std::distance(m_available_tts_models_for_in_mnt_map.begin(), it);
}

int dsnote_app::active_tts_model_for_out_mnt_idx() const {
    auto it = m_available_tts_models_for_out_mnt_map.find(
        m_active_tts_model_for_out_mnt);
    if (it == m_available_tts_models_for_out_mnt_map.end()) {
        return -1;
    }
    return std::distance(m_available_tts_models_for_out_mnt_map.begin(), it);
}

int dsnote_app::active_mnt_lang_idx() const {
    auto it = m_available_mnt_langs_map.find(m_active_mnt_lang);
    if (it == m_available_mnt_langs_map.end()) {
        return -1;
    }
    return std::distance(m_available_mnt_langs_map.begin(), it);
}

int dsnote_app::active_mnt_out_lang_idx() const {
    auto it = m_available_mnt_out_langs_map.find(m_active_mnt_out_lang);
    if (it == m_available_mnt_out_langs_map.end()) {
        return -1;
    }
    return std::distance(m_available_mnt_out_langs_map.begin(), it);
}

void dsnote_app::update_active_tts_model() {
    QString new_tts_model;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_tts_model = speech_service::instance()->default_tts_model();
    } else {
        qDebug() << "[app => dbus] get DefaultTtsModel";
        new_tts_model = m_dbus_service.defaultTtsModel();
    }

    set_active_tts_model(new_tts_model);
}

void dsnote_app::update_active_tts_model_for_in_mnt() {
    if (m_available_tts_models_for_in_mnt_map.empty()) {
        qWarning() << "no available tts models for in mnt";
        return;
    }

    auto default_model =
        settings::instance()->default_tts_model_for_mnt_lang(m_active_mnt_lang);

    if (default_model.isEmpty() ||
        !m_available_tts_models_for_in_mnt_map.contains(default_model)) {
        set_active_tts_model_for_in_mnt(
            m_available_tts_models_for_in_mnt_map.firstKey());
    } else {
        set_active_tts_model_for_in_mnt(default_model);
    }
}

void dsnote_app::update_active_tts_model_for_out_mnt() {
    if (m_available_tts_models_for_out_mnt_map.empty()) {
        qWarning() << "no available tts models for out mnt";
        return;
    }

    auto default_model = settings::instance()->default_tts_model_for_mnt_lang(
        m_active_mnt_out_lang);

    if (default_model.isEmpty() ||
        !m_available_tts_models_for_out_mnt_map.contains(default_model)) {
        set_active_tts_model_for_out_mnt(
            m_available_tts_models_for_out_mnt_map.firstKey());
    } else {
        set_active_tts_model_for_out_mnt(default_model);
    }
}

void dsnote_app::update_active_mnt_lang() {
    if (m_available_mnt_langs_map.empty()) {
        qWarning() << "no available mnt langs";
        return;
    }

    QString new_mnt_lang;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_mnt_lang = speech_service::instance()->default_mnt_lang();
    } else {
        qDebug() << "[app => dbus] get DefaultMntLang";
        new_mnt_lang = m_dbus_service.defaultMntLang();
    }

    if (new_mnt_lang.isEmpty() ||
        !m_available_mnt_langs_map.contains(new_mnt_lang)) {
        new_mnt_lang = m_available_mnt_langs_map.firstKey();
    }

    set_active_mnt_lang(new_mnt_lang);
}

void dsnote_app::update_active_mnt_out_lang() {
    if (m_available_mnt_out_langs_map.empty()) {
        qWarning() << "no available mnt out langs";
        return;
    }

    QString new_mnt_out_lang;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_mnt_out_lang = speech_service::instance()->default_mnt_out_lang();
    } else {
        qDebug() << "[app => dbus] get DefaultMntOutLang";
        new_mnt_out_lang = m_dbus_service.defaultMntOutLang();
    }

    if (new_mnt_out_lang.isEmpty() ||
        !m_available_mnt_out_langs_map.contains(new_mnt_out_lang)) {
        new_mnt_out_lang = m_available_mnt_out_langs_map.firstKey();
    }

    set_active_mnt_out_lang(new_mnt_out_lang);
}

void dsnote_app::do_keepalive() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone)
        return;

    qDebug() << "[app => dbus] call KeepAliveService";
    const auto time = m_dbus_service.KeepAliveService();

    if (!m_dbus_service.isValid()) {
        if (m_init_attempts >= MAX_INIT_ATTEMPTS) {
            qWarning() << "failed to connect to service";
            emit error(error_t::ErrorNoService);
        } else {
            qDebug() << "failed to connect to service, re-trying after"
                     << KEEPALIVE_TIME << "msec";
            m_keepalive_timer.start(KEEPALIVE_TIME);
            m_init_attempts++;
        }
    } else if (time > 0) {
        m_keepalive_timer.start(static_cast<int>(time * 0.75));
        if (m_init_attempts > 0) {
            update_service_state();
            m_init_attempts = 0;
        }
    }
}

void dsnote_app::start_keepalive() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone)
        return;

    if (!m_keepalive_current_task_timer.isActive() &&
        (m_primary_task == m_current_task || m_side_task == m_current_task)) {
        if (m_primary_task == m_current_task)
            qDebug() << "starting keepalive listen_task";
        if (m_side_task == m_current_task)
            qDebug() << "starting keepalive transcribe_task";
        m_keepalive_current_task_timer.start(KEEPALIVE_TIME);
    }
}

void dsnote_app::handle_keepalive_task_timeout() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone)
        return;

    const auto send_ka = [this](int task) {
        qDebug() << "[app => dbus] call KeepAliveTask";
        auto time = m_dbus_service.KeepAliveTask(task);

        if (time > 0) {
            m_keepalive_current_task_timer.start(static_cast<int>(time * 0.75));
        } else {
            qWarning() << "keepalive task failed";
            if (m_side_task)
                m_side_task.reset();
            else if (m_primary_task)
                m_primary_task.reset();

            update_current_task();
            update_service_state();

            set_task_state(service_task_state_t::TaskStateIdle);
        }
    };

    if (m_side_task.current > INVALID_TASK) {
        qDebug() << "keepalive transcribe task timeout:" << m_side_task.current;
        send_ka(m_side_task.current);
    }

    if (m_primary_task.current > INVALID_TASK) {
        qDebug() << "keepalive listen task timeout:" << m_primary_task.current;
        send_ka(m_primary_task.current);
    }
}

QVariantMap dsnote_app::translations() const {
    if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::service &&
        connected())
        return m_dbus_service.translations();

    return {};
}

void dsnote_app::update_configured_state() {
    auto new_stt_configured = [this] {
        if (m_service_state == service_state_t::StateUnknown ||
            m_service_state == service_state_t::StateNotConfigured ||
            m_available_stt_models_map.empty())
            return false;
        return true;
    }();

    auto new_tts_configured = [this] {
        if (m_service_state == service_state_t::StateUnknown ||
            m_service_state == service_state_t::StateNotConfigured ||
            m_available_tts_models_map.empty())
            return false;
        return true;
    }();

    auto new_ttt_configured = [this] {
        if (m_service_state == service_state_t::StateUnknown ||
            m_service_state == service_state_t::StateNotConfigured ||
            m_available_ttt_models_map.empty())
            return false;
        return true;
    }();

    auto new_mnt_configured = [this] {
        if (m_service_state == service_state_t::StateUnknown ||
            m_service_state == service_state_t::StateNotConfigured ||
            m_available_mnt_langs_map.empty())
            return false;
        return true;
    }();

    if (m_stt_configured != new_stt_configured) {
        qDebug() << "app stt configured:" << m_stt_configured << "=>"
                 << new_stt_configured;
        m_stt_configured = new_stt_configured;
        emit stt_configured_changed();
    }

    if (m_tts_configured != new_tts_configured) {
        qDebug() << "app tts configured:" << m_tts_configured << "=>"
                 << new_tts_configured;
        m_tts_configured = new_tts_configured;
        emit tts_configured_changed();
    }

    if (m_ttt_configured != new_ttt_configured) {
        qDebug() << "app ttt configured:" << m_ttt_configured << "=>"
                 << new_ttt_configured;
        m_ttt_configured = new_ttt_configured;
        emit ttt_configured_changed();
    }

    if (m_mnt_configured != new_mnt_configured) {
        qDebug() << "app mnt configured:" << m_mnt_configured << "=>"
                 << new_mnt_configured;
        m_mnt_configured = new_mnt_configured;
        emit mnt_configured_changed();
    }
}

bool dsnote_app::busy() const {
    return m_service_state == service_state_t::StateBusy ||
           another_app_connected() ||
           (settings::instance()->launch_mode() ==
                settings::launch_mode_t::app &&
            !m_service_reload_update_done);
}

bool dsnote_app::stt_configured() const { return m_stt_configured; }

bool dsnote_app::tts_configured() const { return m_tts_configured; }

bool dsnote_app::ttt_configured() const { return m_ttt_configured; }

bool dsnote_app::mnt_configured() const { return m_mnt_configured; }

bool dsnote_app::connected() const {
    return m_service_state != service_state_t::StateUnknown;
}

double dsnote_app::transcribe_progress() const { return m_transcribe_progress; }

double dsnote_app::speech_to_file_progress() const {
    return m_speech_to_file_progress;
}

bool dsnote_app::another_app_connected() const {
    return m_current_task != INVALID_TASK && m_primary_task != m_current_task &&
           m_side_task != m_current_task;
}

void dsnote_app::copy_to_clipboard_internal(const QString &text) {
    auto *clip = QGuiApplication::clipboard();
    if (!text.isEmpty()) {
        clip->setText(text);
        emit note_copied();
    }
}

void dsnote_app::copy_text_to_clipboard(const QString &text) {
    auto *clip = QGuiApplication::clipboard();
    if (!text.isEmpty()) {
        clip->setText(text);
    }
}

void dsnote_app::copy_to_clipboard() { copy_to_clipboard_internal(note()); }

void dsnote_app::copy_translation_to_clipboard() {
    copy_to_clipboard_internal(m_translated_text);
}

bool dsnote_app::file_exists(const QString &file) const {
    return QFileInfo::exists(file);
}

bool dsnote_app::file_exists(const QUrl &file) const {
    return file_exists(file.toLocalFile());
}

QString dsnote_app::translated_text() const { return m_translated_text; }

void dsnote_app::set_translated_text(const QString text) {
    if (text != m_translated_text) {
        m_translated_text = text;
        emit translated_text_changed();
    }
}

QString dsnote_app::note() const { return settings::instance()->note(); }

void dsnote_app::set_note(const QString text) {
    auto old = can_undo_or_redu_note();
    settings::instance()->set_note(text);
    if (old != can_undo_or_redu_note()) emit can_undo_or_redu_note_changed();
    if (text.isEmpty()) set_translated_text({});
}

void dsnote_app::update_note(const QString &text, bool replace) {
    make_undo();

    if (replace) {
        set_note(text);
    } else {
        set_note(insert_to_note(settings::instance()->note(), text, "",
                                settings::instance()->insert_mode()));
    }
}

void dsnote_app::make_undo() {
    auto old = can_undo_or_redu_note();
    m_prev_text = note();
    m_undo_flag = true;
    if (old != can_undo_or_redu_note()) emit can_undo_or_redu_note_changed();
}

bool dsnote_app::can_undo_note() const {
    return can_undo_or_redu_note() && m_undo_flag;
}

bool dsnote_app::can_redo_note() const {
    return can_undo_or_redu_note() && !m_undo_flag;
}

bool dsnote_app::can_undo_or_redu_note() const {
    return !m_prev_text.isEmpty() && m_prev_text != note();
}

void dsnote_app::undo_or_redu_note() {
    if (!can_undo_or_redu_note()) return;

    auto prev_text{m_prev_text};
    m_prev_text = note();
    set_note(prev_text);

    m_undo_flag = !m_undo_flag;

    emit can_undo_or_redu_note_changed();
}

void dsnote_app::handle_translator_settings_changed() {
    if (settings::instance()->translator_mode()) {
        if (settings::instance()->translate_when_typing()) translate_delayed();
    } else if (m_files_to_open.empty()) {
        cancel();
    }
}

void dsnote_app::handle_note_changed() {
    emit note_changed();

    if (settings::instance()->translator_mode() &&
        settings::instance()->translate_when_typing())
        translate_delayed();
}

void dsnote_app::save_note_to_file(const QString &dest_file) {
    save_note_to_file_internal(note(), dest_file);
}

void dsnote_app::save_note_to_file_translator(const QString &dest_file) {
    save_note_to_file_internal(translated_text(), dest_file);
}

void dsnote_app::save_note_to_file_internal(const QString &text,
                                            const QString &dest_file) {
    QFile file{dest_file};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "failed to open file for write:" << dest_file;
        emit error(error_t::ErrorSaveNoteToFile);
        return;
    }

    QTextStream out{&file};
    out << text;

    file.close();

    emit save_note_to_file_done();
}

bool dsnote_app::load_note_from_file(const QString &input_file, bool replace) {
    QFile file{input_file};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "failed to open file for read:" << input_file;
        emit error(error_t::ErrorLoadNoteFromFile);
        return false;
    }

    make_undo();

    if (replace) {
        set_note(file.readAll());
    } else {
        set_note(insert_to_note(settings::instance()->note(), file.readAll(),
                                "", settings::instance()->insert_mode()));
    }

    return true;
}

void dsnote_app::open_next_file() {
    if (m_files_to_open.empty()) return;

    if (busy()) {
        qDebug() << "delaying opening next file";
        m_open_files_delay_timer.start();
        return;
    }

    if (m_files_to_open.front().isEmpty()) {
        reset_files_queue();
        return;
    }

    qDebug() << "opening file:" << m_files_to_open.front();

    if (media_compressor{}.is_media_file(
            m_files_to_open.front().toStdString())) {
        if (stt_configured())
            transcribe_file(
                m_files_to_open.front(),
                m_stt_text_destination == stt_text_destination_t::note_replace
                    ? true
                    : false);
        else {
            qWarning() << "can't transcribe because stt is not configured";
            reset_files_queue();
            return;
        }
        m_files_to_open.pop();
    } else {
        if (!load_note_from_file(
                m_files_to_open.front(),
                m_stt_text_destination == stt_text_destination_t::note_replace
                    ? true
                    : false)) {
            reset_files_queue();
            return;
        }

        m_files_to_open.pop();

        emit can_open_next_file();
    }
}

void dsnote_app::open_files(const QStringList &input_files, bool replace) {
    reset_files_queue();

    for (auto &file : input_files) m_files_to_open.push(file);

    if (!m_files_to_open.empty()) {
        if (m_files_to_open.size() == 1) m_files_to_open.push({});
        if (!note().isEmpty()) make_undo();
        qDebug() << "opening files:" << input_files;
        m_stt_text_destination = replace ? stt_text_destination_t::note_replace
                                         : stt_text_destination_t::note_add;
        m_open_files_delay_timer.start();
    }
}

void dsnote_app::open_files_url(const QList<QUrl> &input_urls, bool replace) {
    QStringList input_files;

    std::transform(input_urls.cbegin(), input_urls.cend(),
                   std::back_inserter(input_files),
                   [](const auto &url) { return url.toLocalFile(); });

    open_files(input_files, replace);
}

void dsnote_app::reset_files_queue() {
    if (!m_files_to_open.empty()) m_files_to_open = std::queue<QString>{};
}

void dsnote_app::close_desktop_notification() {
    if (!m_desktop_notification) return;

    if (!m_desktop_notification->permanent) return;

    m_desktop_notification->close_request = true;

    m_desktop_notification_delay_timer.start();
}

void dsnote_app::process_pending_desktop_notification() {
    if (!m_desktop_notification) return;

    if (m_desktop_notification->close_request) {
        m_dbus_notifications.CloseNotification(m_desktop_notification->id);
        m_desktop_notification.reset();
    } else {
        auto reply = m_dbus_notifications.Notify(
            "", m_desktop_notification->id, APP_ICON_ID,
            m_desktop_notification->summary, m_desktop_notification->body,
            /*actions=*/{"default", tr("Show")}, {},
            m_desktop_notification->permanent ? 0 : -1);

        reply.waitForFinished();
        if (reply.isValid()) {
            m_desktop_notification->id = reply.argumentAt<0>();
        }
    }
}

void dsnote_app::show_desktop_notification(const QString &summary,
                                           const QString &body,
                                           bool permanent) {
    if (!m_dbus_notifications.isValid()) return;

    if (m_desktop_notification) {
        if (permanent && !m_desktop_notification->permanent) return;

        if (!m_desktop_notification->close_request &&
            summary == m_desktop_notification->summary &&
            body == m_desktop_notification->body) {
            return;
        }

        m_desktop_notification->summary = summary;
        m_desktop_notification->body = body;
        m_desktop_notification->permanent = permanent;
        m_desktop_notification->close_request = false;
    } else {
        m_desktop_notification =
            desktop_notification_t{0, summary, body, permanent, false};
    }

    m_desktop_notification_delay_timer.start();
}

void dsnote_app::handle_desktop_notification_closed(
    [[maybe_unused]] uint id, [[maybe_unused]] uint reason) {
    m_desktop_notification.reset();
}

void dsnote_app::handle_desktop_notification_action_invoked(
    [[maybe_unused]] uint id, [[maybe_unused]] const QString &action_key) {
    emit activate_requested();
    close_desktop_notification();
}

bool dsnote_app::feature_available(const QString &name) const {
    return m_features_availability.contains(name) &&
           m_features_availability.value(name).toList().at(0).toBool();
}

bool dsnote_app::feature_gpu_stt() const {
    return feature_available("faster-whisper-stt-gpu") ||
           feature_available("whispercpp-stt-cuda") ||
           feature_available("whispercpp-stt-hip") ||
           feature_available("whispercpp-stt-opencl");
}

bool dsnote_app::feature_gpu_tts() const {
    return feature_available("coqui-tts-gpu");
}

bool dsnote_app::feature_punctuator() const {
    return feature_available("punctuator");
}

bool dsnote_app::feature_diacritizer_he() const {
    return feature_available("diacritizer-he");
}

bool dsnote_app::feature_global_shortcuts() const {
    return feature_available("ui-global-shortcuts");
}

bool dsnote_app::feature_text_active_window() const {
    return feature_available("ui-text-active-window");
}

bool dsnote_app::feature_coqui_tts() const {
    return feature_available("coqui-tts");
}

QVariantList dsnote_app::features_availability() {
    QVariantList list;

    auto old_features_availability = m_features_availability;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        m_features_availability =
            speech_service::instance()->features_availability();
    } else {
        qDebug() << "[app => dbus] call FeaturesAvailability";

        QVariantMap dbus_map = m_dbus_service.FeaturesAvailability();

        m_features_availability.clear();

        auto it = dbus_map.cbegin();
        while (it != dbus_map.cend()) {
            m_features_availability.insert(
                it.key(),
                qdbus_cast<QVariantList>(it.value().value<QDBusArgument>()));
            ++it;
        }
    }

#ifdef USE_DESKTOP
    if (!m_features_availability.isEmpty()) {
        auto has_xbc = settings::instance()->is_xcb();
        m_features_availability.insert(
            "ui-global-shortcuts",
            QVariantList{has_xbc, tr("Global keyboard shortcuts")});
        m_features_availability.insert(
            "ui-text-active-window",
            QVariantList{has_xbc, tr("Insert text to active window")});
    }
#endif

    auto it = m_features_availability.cbegin();
    while (it != m_features_availability.cend()) {
        auto val = it.value().toList();
        if (val.size() > 1) list.push_back(QVariantList{val.at(0), val.at(1)});
        ++it;
    }

    if (old_features_availability != m_features_availability) {
        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app) {
            models_manager::instance()->update_models_using_availability(
                {/*tts_coqui=*/feature_available("coqui-tts"),
                 /*tts_mimic3=*/feature_available("mmic3-tts"),
                 /*tts_mimic3_de=*/feature_available("mmic3-tts-de"),
                 /*tts_mimic3_es=*/feature_available("mmic3-tts-es"),
                 /*tts_mimic3_fr=*/feature_available("mmic3-tts-fr"),
                 /*tts_mimic3_it=*/feature_available("mmic3-tts-it"),
                 /*tts_mimic3_ru=*/feature_available("mmic3-tts-ru"),
                 /*tts_mimic3_sw=*/feature_available("mmic3-tts-sw"),
                 /*tts_mimic3_fa=*/feature_available("mmic3-tts-fa"),
                 /*tts_mimic3_nl=*/feature_available("mmic3-tts-nl"),
                 /*stt_fasterwhisper=*/feature_available("faster-whisper-stt"),
                 /*ttt_hftc=*/feature_available("punctuator"),
                 /*option_r=*/feature_available("coqui-tts-ko")});
        }

        emit features_changed();
    }

    return list;
}

void dsnote_app::execute_pending_action() {
    if (!m_pending_action) return;

    if (busy()) {
        m_action_delay_timer.start();
        return;
    }

    execute_action(m_pending_action.value());

    m_pending_action.reset();
}

void dsnote_app::execute_action_name(const QString &action_name) {
    if (action_name.isEmpty()) return;

    if (!settings::instance()->actions_api_enabled()) {
        qWarning() << "actions api is not enabled in the settings";
        return;
    }

    if (action_name.compare("start-listening", Qt::CaseInsensitive) == 0) {
        execute_action(action_t::start_listening);
    } else if (action_name.compare("start-listening-active-window",
                                   Qt::CaseInsensitive) == 0) {
        execute_action(action_t::start_listening_active_window);
    } else if (action_name.compare("start-listening-clipboard",
                                   Qt::CaseInsensitive) == 0) {
        execute_action(action_t::start_listening_clipboard);
    } else if (action_name.compare("stop-listening", Qt::CaseInsensitive) ==
               0) {
        execute_action(action_t::stop_listening);
    } else if (action_name.compare("start-reading", Qt::CaseInsensitive) == 0) {
        execute_action(action_t::start_reading);
    } else if (action_name.compare("start-reading-clipboard",
                                   Qt::CaseInsensitive) == 0) {
        execute_action(action_t::start_reading_clipboard);
    } else if (action_name.compare("pause-resume-reading",
                                   Qt::CaseInsensitive) == 0) {
        execute_action(action_t::pause_resume_reading);
    } else if (action_name.compare("cancel", Qt::CaseInsensitive) == 0) {
        execute_action(action_t::cancel);
    } else {
        qWarning() << "invalid action:" << action_name;
    }
}

#ifdef USE_DESKTOP
void dsnote_app::execute_tray_action(tray_icon::action_t action) {
    switch (action) {
        case tray_icon::action_t::start_listening:
            execute_action(action_t::start_listening);
            break;
        case tray_icon::action_t::start_listening_active_window:
            execute_action(action_t::start_listening_active_window);
            break;
        case tray_icon::action_t::start_listening_clipboard:
            execute_action(action_t::start_listening_clipboard);
            break;
        case tray_icon::action_t::stop_listening:
            execute_action(action_t::stop_listening);
            break;
        case tray_icon::action_t::start_reading:
            execute_action(action_t::start_reading);
            break;
        case tray_icon::action_t::start_reading_clipboard:
            execute_action(action_t::start_reading_clipboard);
            break;
        case tray_icon::action_t::pause_resume_reading:
            execute_action(action_t::pause_resume_reading);
            break;
        case tray_icon::action_t::cancel:
            execute_action(action_t::cancel);
            break;
        case tray_icon::action_t::quit:
            QGuiApplication::quit();
            break;
    }
}
#endif

void dsnote_app::execute_action(action_t action) {
    if (busy()) {
        m_pending_action = action;
        m_action_delay_timer.start();
        qDebug() << "delaying action:" << action;
        return;
    }

    qDebug() << "executing action:" << action;

    switch (action) {
        case dsnote_app::action_t::start_listening:
            listen();
            break;
        case dsnote_app::action_t::start_listening_active_window:
            listen_to_active_window();
            break;
        case dsnote_app::action_t::start_listening_clipboard:
            listen_to_clipboard();
            break;
        case dsnote_app::action_t::stop_listening:
            stop_listen();
            break;
        case dsnote_app::action_t::start_reading:
            play_speech();
            break;
        case dsnote_app::action_t::start_reading_clipboard:
            play_speech_from_clipboard();
            break;
        case dsnote_app::action_t::pause_resume_reading:
            if (task_state() == service_task_state_t::TaskStateSpeechPaused)
                resume_speech();
            else
                pause_speech();
            break;
        case dsnote_app::action_t::cancel:
            cancel();
            break;
    }
}

QString dsnote_app::download_content(const QUrl &url) {
    QString text;

    if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https")) {
        qDebug() << "downloading content:" << url.toString();

        auto data = downloader{}.download_data(url);

        qDebug() << "downlaoded content:" << data.mime << data.bytes.size();

        if (data.mime.contains(QLatin1String{"text"}, Qt::CaseInsensitive))
            text = QString::fromUtf8(data.bytes);
    }

    if (text.isEmpty()) emit error(error_t::ErrorContentDownload);

    return text;
}

bool dsnote_app::stt_translate_needed_by_id(const QString &id) const {
    if (m_available_stt_models_map.contains(id)) {
        auto model = m_available_stt_models_map.value(id).toStringList();
        return model.size() > 2 && model.at(2).contains('t');
    }

    return false;
}

bool dsnote_app::tts_ref_voice_needed_by_id(const QString &id) const {
    if (m_available_tts_models_map.contains(id)) {
        auto model = m_available_tts_models_map.value(id).toStringList();
        return model.size() > 2 && model.at(2).contains('x');
    }

    return false;
}

bool dsnote_app::stt_translate_needed() const {
    return stt_translate_needed_by_id(m_active_stt_model);
}

bool dsnote_app::tts_ref_voice_needed() const {
    return tts_ref_voice_needed_by_id(m_active_tts_model);
}

bool dsnote_app::tts_for_in_mnt_ref_voice_needed() const {
    return tts_ref_voice_needed_by_id(m_active_tts_model_for_in_mnt);
}

bool dsnote_app::tts_for_out_mnt_ref_voice_needed() const {
    return tts_ref_voice_needed_by_id(m_active_tts_model_for_out_mnt);
}

QString dsnote_app::cache_dir() {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString dsnote_app::import_ref_voice_file_path() {
    return QDir{cache_dir()}.absoluteFilePath(s_imported_ref_file_name);
}

QString dsnote_app::player_import_from_url(const QUrl &url) {
    auto path = url.toLocalFile();
    auto wav_file_path = import_ref_voice_file_path();

    try {
        media_compressor{}.decompress({path.toStdString()},
                                      wav_file_path.toStdString(), false);
    } catch (const std::runtime_error &error) {
        qWarning() << "can't import file:" << error.what();
        return {};
    }

    if (!m_player) create_player();

    m_player->setMedia(
        QUrl{QStringLiteral("gst-pipeline: filesrc location=%1 ! wavparse ! "
                            "audioconvert ! alsasink")
                 .arg(wav_file_path)});

    auto mtag = mtag_tools::read(path.toStdString());
    if (mtag && !mtag->title.empty())
        return QString::fromStdString(mtag->title);
    else
        return QFileInfo{path}.baseName();
}

QString dsnote_app::player_import_mic_rec() {
    auto wav_file_path = import_ref_voice_file_path();

    if (!m_player) create_player();

    m_player->setMedia(
        QUrl{QStringLiteral("gst-pipeline: filesrc location=%1 ! wavparse ! "
                            "audioconvert ! alsasink")
                 .arg(wav_file_path)});

    return {};
}

QString dsnote_app::tts_ref_voice_auto_name() const {
    return tts_ref_voice_unique_name(tr("Voice"), true);
}

QString dsnote_app::tts_ref_voice_unique_name(QString name,
                                              bool add_number) const {
    QStringList names;
    for (auto it = m_available_tts_ref_voices_map.constBegin();
         it != m_available_tts_ref_voices_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (!v.isEmpty()) names.push_back(v.front());
    }

    if (!add_number && !names.contains(name)) return name;

    int i = 1;
    QRegExp rx{"\\d+$"};
    if (auto idx = rx.indexIn(name); idx >= 0) {
        bool ok = false;
        auto ii = name.midRef(idx).toInt(&ok);
        if (ok && ii < 99999) {
            i = ii;
            name = name.mid(0, idx) + "%1";
        } else {
            name += " %1";
        }
    } else if (name.endsWith('-')) {
        name += "%1";
    } else {
        name += " %1";
    }

    for (; i < 99999; ++i) {
        if (!names.contains(name.arg(i))) {
            name = name.arg(i);
            break;
        }
    }

    return name;
}

void dsnote_app::player_export_ref_voice(long long start, long long stop,
                                         const QString &name) {
    QDir ref_voices_dir{
        QDir{QStandardPaths::writableLocation(QStandardPaths::DataLocation)}
            .filePath(s_ref_voices_dir_name)};

    QString out_file_path;
    for (int i = 1; i < 99999; ++i) {
        out_file_path = ref_voices_dir.absoluteFilePath(
            QStringLiteral("voice-%1.opus").arg(i));
        if (!QFileInfo::exists(out_file_path)) break;
    }

    auto wav_file_path = import_ref_voice_file_path();

    try {
        media_compressor{}.compress(
            {wav_file_path.toStdString()}, out_file_path.toStdString(),
            media_compressor::format_t::ogg_opus,
            media_compressor::quality_t::vbr_high,
            {static_cast<uint64_t>(start), static_cast<uint64_t>(stop), 0, 0});
    } catch (const std::runtime_error &error) {
        qWarning() << "can't compress file:" << error.what();
        return;
    }

    mtag_tools::mtag_t mtag;
    mtag.title =
        tts_ref_voice_unique_name(
            name.isEmpty() ? QFileInfo{out_file_path}.baseName() : name, false)
            .toStdString();
    mtag_tools::write(out_file_path.toStdString(), mtag);

    player_reset();

    update_available_tts_ref_voices();
}

void dsnote_app::player_reset() {
    if (!m_player) return;

    QFile{import_ref_voice_file_path()}.remove();
    m_player->setMedia({});
}

bool dsnote_app::player_ready() const {
    return m_player &&
           (m_player->mediaStatus() == QMediaPlayer::MediaStatus::LoadedMedia ||
            m_player->mediaStatus() ==
                QMediaPlayer::MediaStatus::BufferedMedia ||
            m_player->mediaStatus() == QMediaPlayer::MediaStatus::EndOfMedia);
}

void dsnote_app::create_recorder() {
    m_recorder = std::make_unique<recorder>(import_ref_voice_file_path());

    connect(
        m_recorder.get(), &recorder::recording_changed, this,
        [this]() {
            qDebug() << "recorder recording changed:"
                     << m_recorder->recording();
            emit recorder_recording_changed();
        },
        Qt::QueuedConnection);
    connect(
        m_recorder.get(), &recorder::duration_changed, this,
        [this]() { emit recorder_duration_changed(); }, Qt::QueuedConnection);
    connect(
        m_recorder.get(), &recorder::processing_changed, this,
        [this]() {
            player_import_mic_rec();
            emit recorder_processing_changed();
        },
        Qt::QueuedConnection);
}

bool dsnote_app::recorder_recording() const {
    return m_recorder && m_recorder->recording();
}

bool dsnote_app::recorder_processing() const {
    return m_recorder && m_recorder->processing();
}

long long dsnote_app::recorder_duration() const {
    return m_recorder ? m_recorder->duration() : 0;
}

void dsnote_app::recorder_start() {
    if (!m_recorder) create_recorder();

    m_recorder->start();
}

void dsnote_app::recorder_stop() {
    if (!m_recorder) return;

    m_recorder->stop();
}

bool dsnote_app::player_playing() const {
    return m_player && m_player->state() == QMediaPlayer::State::PlayingState;
}

void dsnote_app::player_set_position(long long position) {
    if (!m_player) return;

    m_player->stop();
    m_player->setPosition(position);
}

void dsnote_app::player_play(long long start, long long stop) {
    if (!m_player) return;

    m_player_stop_position = stop;
    m_player_requested_play_position = start;
    m_player->setPosition(10);
}

void dsnote_app::player_stop() {
    if (!m_player) return;

    m_player->pause();
}

long long dsnote_app::player_position() const {
    return m_player ? m_player->position() : 0;
}

long long dsnote_app::player_duration() const {
    return m_player ? m_player->duration() : 0;
}

void dsnote_app::update_tray_state() {
#ifdef USE_DESKTOP
    switch (m_service_state) {
        case service_state_t::StateUnknown:
        case service_state_t::StateNotConfigured:
        case service_state_t::StateBusy:
            m_tray.set_state(tray_icon::state_t::busy);
            break;
        case service_state_t::StateIdle:
            m_tray.set_state(tray_icon::state_t::idle);
            break;
        case service_state_t::StateListeningManual:
        case service_state_t::StateListeningAuto:
        case service_state_t::StateListeningSingleSentence:
            m_tray.set_state(tray_icon::state_t::stt);
            break;
        case service_state_t::StateTranscribingFile:
            m_tray.set_state(tray_icon::state_t::stt_file);
            break;
        case service_state_t::StatePlayingSpeech:
            m_tray.set_state(tray_icon::state_t::tts);
            break;
        case service_state_t::StateWritingSpeechToFile:
            m_tray.set_state(tray_icon::state_t::tts_file);
            break;
        case service_state_t::StateTranslating:
            m_tray.set_state(tray_icon::state_t::mnt);
            break;
    }
#endif
}

void dsnote_app::update_tray_task_state() {
#ifdef USE_DESKTOP
    switch (m_task_state) {
        case service_task_state_t::TaskStateIdle:
            m_tray.set_task_state(tray_icon::task_state_t::idle);
            break;
        case service_task_state_t::TaskStateSpeechDetected:
        case service_task_state_t::TaskStateProcessing:
            m_tray.set_task_state(tray_icon::task_state_t::processing);
            break;
        case service_task_state_t::TaskStateInitializing:
            m_tray.set_task_state(tray_icon::task_state_t::initializing);
            break;
        case service_task_state_t::TaskStateSpeechPlaying:
            m_tray.set_task_state(tray_icon::task_state_t::playing);
            break;
        case service_task_state_t::TaskStateSpeechPaused:
            m_tray.set_task_state(tray_icon::task_state_t::paused);
            break;
    }
#endif
}

void dsnote_app::show_tray() {
#ifdef USE_DESKTOP
    m_tray.show();
#endif
}
void dsnote_app::hide_tray() {
#ifdef USE_DESKTOP
    m_tray.hide();
#endif
}

void dsnote_app::register_hotkeys() {
#ifdef USE_DESKTOP
    auto *s = settings::instance();

    if (!s->is_xcb()) {
        qWarning() << "hot keys are supported only under x11";
        return;
    }

    QObject::disconnect(&m_hotkeys.start_listening);
    QObject::disconnect(&m_hotkeys.start_listening_active_window);
    QObject::disconnect(&m_hotkeys.start_listening_clipboard);
    QObject::disconnect(&m_hotkeys.stop_listening);
    QObject::disconnect(&m_hotkeys.start_reading);
    QObject::disconnect(&m_hotkeys.start_reading_clipboard);
    QObject::disconnect(&m_hotkeys.pause_resume_reading);
    QObject::disconnect(&m_hotkeys.cancel);
    m_hotkeys.start_listening.setRegistered(false);
    m_hotkeys.start_listening_active_window.setRegistered(false);
    m_hotkeys.start_listening_clipboard.setRegistered(false);
    m_hotkeys.stop_listening.setRegistered(false);
    m_hotkeys.start_reading.setRegistered(false);
    m_hotkeys.start_reading_clipboard.setRegistered(false);
    m_hotkeys.pause_resume_reading.setRegistered(false);
    m_hotkeys.cancel.setRegistered(false);

    if (s->hotkeys_enabled()) {
        if (!s->hotkey_start_listening().isEmpty()) {
            m_hotkeys.start_listening.setShortcut(
                QKeySequence{s->hotkey_start_listening()}, true);
            QObject::connect(
                &m_hotkeys.start_listening, &QHotkey::activated, this, [&]() {
                    qDebug() << "hot key activated: start-listening";
                    execute_action(action_t::start_listening);
                });
        }

        if (!s->hotkey_start_listening_active_window().isEmpty()) {
            m_hotkeys.start_listening_active_window.setShortcut(
                QKeySequence{s->hotkey_start_listening_active_window()}, true);
            QObject::connect(
                &m_hotkeys.start_listening_active_window, &QHotkey::activated,
                this, [&]() {
                    qDebug()
                        << "hot key activated: start-listening-active-window";
                    execute_action(action_t::start_listening_active_window);
                });
        }

        if (!s->hotkey_start_listening_clipboard().isEmpty()) {
            m_hotkeys.start_listening_clipboard.setShortcut(
                QKeySequence{s->hotkey_start_listening_clipboard()}, true);
            QObject::connect(
                &m_hotkeys.start_listening_clipboard, &QHotkey::activated, this,
                [&]() {
                    qDebug() << "hot key activated: start-listening-clipboard";
                    execute_action(action_t::start_listening_clipboard);
                });
        }

        if (!s->hotkey_stop_listening().isEmpty()) {
            m_hotkeys.stop_listening.setShortcut(
                QKeySequence{s->hotkey_stop_listening()}, true);
            QObject::connect(
                &m_hotkeys.stop_listening, &QHotkey::activated, this, [&]() {
                    qDebug() << "hot key activated: stop-listening";
                    execute_action(action_t::stop_listening);
                });
        }

        if (!s->hotkey_start_reading().isEmpty()) {
            m_hotkeys.start_reading.setShortcut(
                QKeySequence{s->hotkey_start_reading()}, true);
            QObject::connect(&m_hotkeys.start_reading, &QHotkey::activated,
                             this, [&]() {
                                 qDebug() << "hot key activated: start-reading";
                                 execute_action(action_t::start_reading);
                             });
        }

        if (!s->hotkey_start_reading_clipboard().isEmpty()) {
            m_hotkeys.start_reading_clipboard.setShortcut(
                QKeySequence{s->hotkey_start_reading_clipboard()}, true);
            QObject::connect(
                &m_hotkeys.start_reading_clipboard, &QHotkey::activated, this,
                [&]() {
                    qDebug() << "hot key activated: start-reading-clipboard";
                    execute_action(action_t::start_reading_clipboard);
                });
        }

        if (!s->hotkey_pause_resume_reading().isEmpty()) {
            m_hotkeys.pause_resume_reading.setShortcut(
                QKeySequence{s->hotkey_pause_resume_reading()}, true);
            QObject::connect(
                &m_hotkeys.pause_resume_reading, &QHotkey::activated, this,
                [&]() {
                    qDebug() << "hot key activated: pause-resume-reading";
                    execute_action(action_t::pause_resume_reading);
                });
        }

        if (!s->hotkey_cancel().isEmpty()) {
            m_hotkeys.cancel.setShortcut(QKeySequence{s->hotkey_cancel()},
                                         true);
            QObject::connect(&m_hotkeys.cancel, &QHotkey::activated, this,
                             [&]() {
                                 qDebug() << "hot key activated: cancel";
                                 execute_action(action_t::cancel);
                             });
        }
    }
#endif
}

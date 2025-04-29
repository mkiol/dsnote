/* Copyright (C) 2021-2025 Michal Kosciesza <michal@mkiol.net>
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
#include <QKeySequence>
#include <QRegExp>
#include <QTextStream>
#include <algorithm>
#include <iterator>
#include <utility>

#include "downloader.hpp"
#include "media_compressor.hpp"
#include "module_tools.hpp"
#include "mtag_tools.hpp"
#include "qtlogger.hpp"
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
        case dsnote_app::service_state_t::StateRepairingText:
            d << "repairing-text";
            break;
        case dsnote_app::service_state_t::StateImporting:
            d << "importing";
            break;
        case dsnote_app::service_state_t::StateExporting:
            d << "exporting";
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
        case dsnote_app::service_task_state_t::TaskStateCancelling:
            d << "cancelling";
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
        case dsnote_app::error_t::ErrorMntRuntime:
            d << "mnt-runtime-error";
            break;
        case dsnote_app::error_t::ErrorTextRepairEngine:
            d << "text-repair-engine-error";
            break;
        case dsnote_app::error_t::ErrorNoService:
            d << "no-service-error";
            break;
        case dsnote_app::error_t::ErrorExportFileGeneral:
            d << "save-note-to-file-error";
            break;
        case dsnote_app::error_t::ErrorImportFileGeneral:
            d << "import-file-general";
            break;
        case dsnote_app::error_t::ErrorImportFileNoStreams:
            d << "import-file-no-streams";
            break;
        case dsnote_app::error_t::ErrorSttNotConfigured:
            d << "import-stt-not-configured";
            break;
        case dsnote_app::error_t::ErrorTtsNotConfigured:
            d << "import-tts-not-configured";
            break;
        case dsnote_app::error_t::ErrorMntNotConfigured:
            d << "import-mnt-not-configured";
            break;
        case dsnote_app::error_t::ErrorContentDownload:
            d << "content-download-error";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::file_import_result_t result) {
    switch (result) {
        case dsnote_app::file_import_result_t::ok_streams_selection:
            d << "ok-stream-selection";
            break;
        case dsnote_app::file_import_result_t::ok_import_audio:
            d << "ok-import-audio";
            break;
        case dsnote_app::file_import_result_t::ok_import_subtitles:
            d << "ok-import-subtitles";
            break;
        case dsnote_app::file_import_result_t::ok_import_text:
            d << "ok-import-text";
            break;
        case dsnote_app::file_import_result_t::error_no_supported_streams:
            d << "error-no-supported-streams";
            break;
        case dsnote_app::file_import_result_t::error_requested_stream_not_found:
            d << "error-requested-stream-not-found";
            break;
        case dsnote_app::file_import_result_t::
            error_import_audio_stt_not_configured:
            d << "error-import-audio-stt-not_configured";
            break;
        case dsnote_app::file_import_result_t::error_import_subtitles_error:
            d << "error-import-subtitles-error";
            break;
        case dsnote_app::file_import_result_t::error_import_text_error:
            d << "error-import-text-error";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::action_t action) {
    switch (action) {
#define X(name, str)                 \
    case dsnote_app::action_t::name: \
        d << str;                    \
        break;
        ACTION_TABLE
#undef X
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::auto_text_format_t format) {
    switch (format) {
        case dsnote_app::auto_text_format_t::AutoTextFormatRaw:
            d << "raw";
            break;
        case dsnote_app::auto_text_format_t::AutoTextFormatSubRip:
            d << "subrip";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::stt_request_t request) {
    switch (request) {
        case dsnote_app::stt_request_t::listen:
            d << "listen";
            break;
        case dsnote_app::stt_request_t::listen_translate:
            d << "listen-translate";
            break;
        case dsnote_app::stt_request_t::listen_active_window:
            d << "listen-active-window";
            break;
        case dsnote_app::stt_request_t::listen_translate_active_window:
            d << "listen-translate-active-window";
            break;
        case dsnote_app::stt_request_t::listen_clipboard:
            d << "listen-clipboard";
            break;
        case dsnote_app::stt_request_t::listen_translate_clipboard:
            d << "listen-translate-clipboard";
            break;
        case dsnote_app::stt_request_t::transcribe_file:
            d << "transcribe-file";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, const dsnote_app::trans_rule_t &rule) {
    d.nospace() << "name=" << rule.name << ", type=" << rule.type
                << ", pattern=" << rule.pattern << ", replace=" << rule.replace
                << ", flags=[" << rule.flags << "]";
    return d;
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

static media_compressor::quality_t audio_quality_to_media_quality(
    settings::audio_quality_t quality) {
    switch (quality) {
        case settings::audio_quality_t::AudioQualityVbrHigh:
            return media_compressor::quality_t::vbr_high;
        case settings::audio_quality_t::AudioQualityVbrMedium:
            return media_compressor::quality_t::vbr_medium;
        case settings::audio_quality_t::AudioQualityVbrLow:
            return media_compressor::quality_t::vbr_low;
    }

    throw std::runtime_error{"invalid quality value"};
}

dsnote_app::dsnote_app(QObject *parent)
    : QObject{parent},
      m_dbus_service{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                     QDBusConnection::sessionBus()},
      m_dbus_notifications{"org.freedesktop.Notifications",
                           "/org/freedesktop/Notifications",
                           QDBusConnection::sessionBus()},
      m_audio_dm{[&] { emit audio_devices_changed(); }} {
    initQtLogger();
    qDebug() << "starting app:" << settings::launch_mode;

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
    connect(settings::instance(), &settings::mnt_clean_text_changed, this,
            &dsnote_app::handle_translator_settings_changed,
            Qt::QueuedConnection);
    connect(settings::instance(), &settings::mnt_text_format_changed, this,
            &dsnote_app::handle_translator_settings_changed,
            Qt::QueuedConnection);
    connect(settings::instance(), &settings::audio_input_device_changed, this,
            [this] { emit audio_source_changed(); });
    connect(settings::instance(), &settings::insert_mode_changed, this,
            [this] { set_last_cursor_position(-1); });

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
            m_primary_task.reset();
            m_current_task = INVALID_TASK;
            reset_progress();
        }
    });
#ifdef USE_DESKTOP
    connect(this, &dsnote_app::available_stt_models_changed, this,
            [this]() { m_tray.set_stt_models(available_stt_models()); });
    connect(this, &dsnote_app::active_stt_model_changed, this, [this]() {
        m_tray.set_active_stt_model(active_stt_model_name(),
                                    stt_translate_needed());
    });
    connect(this, &dsnote_app::available_tts_models_changed, this,
            [this]() { m_tray.set_tts_models(available_tts_models()); });
    connect(this, &dsnote_app::active_tts_model_changed, this,
            [this]() { m_tray.set_active_tts_model(active_tts_model_name()); });
#endif
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
    connect(
        this, &dsnote_app::audio_devices_changed, this,
        [this] { update_audio_sources(); }, Qt::QueuedConnection);
    connect(&m_mc, &media_converter::state_changed, this,
            &dsnote_app::handle_mc_state_changed, Qt::QueuedConnection);
    connect(&m_mc, &media_converter::progress_changed, this,
            &dsnote_app::handle_mc_progress_changed, Qt::QueuedConnection);

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

    m_auto_format_delay_timer.setSingleShot(true);
    m_auto_format_delay_timer.setInterval(500);
    connect(&m_auto_format_delay_timer, &QTimer::timeout, this,
            &dsnote_app::update_auto_text_format, Qt::QueuedConnection);

    connect(&m_dbus_notifications,
            &OrgFreedesktopNotificationsInterface::NotificationClosed, this,
            &dsnote_app::handle_desktop_notification_closed);
    connect(&m_dbus_notifications,
            &OrgFreedesktopNotificationsInterface::ActionInvoked, this,
            &dsnote_app::handle_desktop_notification_action_invoked);

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
    handle_note_changed();
    update_audio_sources();

#ifdef USE_DESKTOP
    connect(&m_tray, &QSystemTrayIcon::activated, this,
            [this]([[maybe_unused]] QSystemTrayIcon::ActivationReason reason) {
                emit tray_activated();
            });
    connect(&m_tray, &tray_icon::action_triggered, this,
            &dsnote_app::execute_tray_action);
    connect(&m_gs_manager, &global_hotkeys_manager::hotkey_activated, this,
            [this](const QString &action_id, const QString &extra) {
                execute_action_id(action_id, extra, true);
            });
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
    if (settings::launch_mode != settings::launch_mode_t::app) return;

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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    update_stt_auto_lang(lang);

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

std::pair<QString, int> dsnote_app::insert_to_note(QString note,
                                                   QString new_text,
                                                   const QString &lang,
                                                   settings::insert_mode_t mode,
                                                   int last_cursor_position) {
    if (mode == settings::insert_mode_t::InsertReplace) {
        return {std::move(new_text), new_text.size()};
    }

    if (new_text.isEmpty()) {
        return {std::move(note), last_cursor_position};
    }

    auto [dot, space] = full_stop(lang);

    bool insert_into_cursor_position =
        mode == settings::insert_mode_t::InsertAtCursor && !note.isEmpty() &&
        last_cursor_position >= 0 && last_cursor_position <= note.size();

    if (insert_into_cursor_position) {
        QString note_prefix = note.mid(0, last_cursor_position);
        QString note_sufix = note.mid(last_cursor_position);
        QTextStream ss{&note_prefix, QIODevice::WriteOnly};

        // insert space to prefix if needed
        if (!note_prefix.isEmpty() &&
            !note_prefix.at(note_prefix.size() - 1).isSpace() &&
            !new_text.at(0).isSpace() && !new_text.at(0).isPunct()) {
            ss << ' ';
        }

        // make new text start upper or lower
        auto tprefix = note_prefix.trimmed();
        new_text[0] = note_prefix.isEmpty() || tprefix.right(1) == dot ||
                              tprefix.right(1) == QChar{'?'} ||
                              tprefix.right(1) == QChar{'!'}
                          ? new_text[0].toUpper()
                          : new_text[0].toLower();

        if (auto c = new_text.at(new_text.size() - 1);
            c == dot || c == '?' || c == '!') {
            // remove trailing dot if sufix starts with dot or doesn't start
            // with upper
            auto tsufix = note_sufix.trimmed();
            if (!tsufix.isEmpty() &&
                ((tsufix.at(0).isLower() && !tsufix.at(0).isUpper()) ||
                 tsufix.at(0) == dot || tsufix.at(0) == '?' ||
                 tsufix.at(0) == '!')) {
                new_text = new_text.left(new_text.size() - 1);
            }
        } else if (new_text.at(new_text.size() - 1).isLetterOrNumber()) {
            // add trailing dot if sufix starts with with upper
            auto tsufix = note_sufix.trimmed();
            if (!tsufix.isEmpty() && tsufix.at(0).isUpper() &&
                !tsufix.at(0).isLower()) {
                new_text += dot;
            }
        }

        ss << new_text;

        last_cursor_position = note_prefix.size();

        if (!note_sufix.isEmpty()) {
            // insert space if needed
            if (!new_text.at(new_text.size() - 1).isSpace() &&
                !note_sufix.at(0).isSpace() && !note_sufix.at(0).isPunct()) {
                ss << ' ';
            }

            // make sufix star upper or lower
            auto tnew = new_text.trimmed();
            if (tnew.right(1) == dot || tnew.right(1) == QChar{'?'} ||
                tnew.right(1) == QChar{'!'}) {
                note_sufix[0] = note_sufix[0].toUpper();
            }
            ss << note_sufix;
        } else {
            // insert dot if needed
            if (note_prefix.at(note_prefix.size() - 1).isLetterOrNumber())
                ss << dot;
        }

        return {std::move(note_prefix), last_cursor_position};
    } else {
        QTextStream ss{&note, QIODevice::WriteOnly};

        switch (mode) {
            case settings::insert_mode_t::InsertAtCursor:
            case settings::insert_mode_t::InsertInLine:
                if (!note.isEmpty()) {
                    auto last_char = note.at(note.size() - 1);
                    if (last_char.isLetterOrNumber() && new_text.at(0) != dot)
                        ss << dot << space;
                    else if (last_char == dot && !new_text.at(0).isSpace())
                        ss << space;
                    else if (!last_char.isSpace() && !new_text.at(0).isSpace())
                        ss << ' ';
                }
                break;
            case settings::insert_mode_t::InsertNewLine:
                if (!note.isEmpty()) {
                    auto last_char = note.at(note.size() - 1);
                    if (last_char.isLetterOrNumber() && new_text.at(0) != dot)
                        ss << dot << '\n';
                    else if (last_char != '\n' && new_text.at(0) != '\n')
                        ss << '\n';
                }
                break;
            case settings::insert_mode_t::InsertAfterEmptyLine:
                if (!note.isEmpty()) {
                    auto last_char = note.at(note.size() - 1);
                    auto one_before_last_char =
                        note.size() > 1 ? note.at(note.size() - 2) : '\0';
                    if (last_char.isLetterOrNumber())
                        ss << dot << "\n\n";
                    else if (last_char == '\n' && one_before_last_char != '\n')
                        ss << '\n';
                    else if (last_char != '\n')
                        ss << "\n\n";
                }
                break;
            case settings::insert_mode_t::InsertReplace:
                break;
        }

        new_text[0] = new_text[0].toUpper();

        ss << new_text;

        if (new_text.at(new_text.size() - 1).isLetterOrNumber()) ss << dot;

        return {std::move(note), note.size()};
    }
}

settings::trans_rule_flags_t dsnote_app::apply_trans_rule(
    QString &text, const trans_rule_t &rule) {
    using trans_rule_type_t = settings::trans_rule_type_t;
    using trans_rule_flags_t = settings::trans_rule_flags_t;

    // qDebug() << "trans rule:" << rule;

    bool case_sens = (rule.flags & trans_rule_flags_t::TransRuleCaseSensitive);
    bool rule_matches = false;

    switch (rule.type) {
        case trans_rule_type_t::TransRuleTypeNone:
            break;
        case trans_rule_type_t::TransRuleTypeMatchSimple:
            rule_matches = text.contains(rule.pattern, case_sens ? Qt::CaseSensitive : Qt::CaseInsensitive)
                               ? rule.flags
                               : trans_rule_flags_t::TransRuleNone;
            break;
        case trans_rule_type_t::TransRuleTypeMatchRe:
            rule_matches =
                text.contains(QRegExp{rule.pattern, case_sens ? Qt::CaseSensitive : Qt::CaseInsensitive});
            break;
        case trans_rule_type_t::TransRuleTypeReplaceSimple: {
            rule_matches = text.contains(rule.pattern, case_sens ? Qt::CaseSensitive : Qt::CaseInsensitive);
            if (rule_matches)
                text.replace(rule.pattern, rule.replace, case_sens ? Qt::CaseSensitive : Qt::CaseInsensitive);
            break;
        }
        case trans_rule_type_t::TransRuleTypeReplaceRe: {
            auto replace = rule.replace;
            replace.replace("\\n", "\n");

            QRegExp rx{rule.pattern, case_sens ? Qt::CaseSensitive : Qt::CaseInsensitive};

            rule_matches = text.contains(rx);
            if (rule_matches) {
                if (!rule.replace.contains("\\U") && !replace.contains("\\u")) {
                    text.replace(rx, replace);
                } else {
                    int pos = 0;
                    while (pos < text.size() && (pos = rx.indexIn(text, pos)) != -1 && rx.matchedLength() > 0) {
                        QString after = replace;
                        for (int i = 1; i < rx.captureCount() + 1; ++i) {
                            after.replace(QStringLiteral("\\U\\%1").arg(i),
                                          rx.cap(i).toUpper());
                            after.replace(QStringLiteral("\\u\\%1").arg(i),
                                          rx.cap(i).toLower());
                            after.replace(QStringLiteral("\\%1").arg(i),
                                          rx.cap(i));
                        }
                        text.replace(pos, rx.matchedLength(), after);
                        pos += after.size();
                    }
                }
            }

            break;
        }
    }

    return rule_matches ? (rule.flags | trans_rule_flags_t::TransRuleMatched)
                        : trans_rule_flags_t::TransRuleNone;
}

dsnote_app::trans_rule_result_t dsnote_app::transform_text(
    QString &text, transform_text_target_t target, const QString &lang) {
    trans_rule_result_t result;

    for (const auto &vrule : settings::instance()->trans_rules()) {
        auto vl = vrule.toList();
        if (vl.size() < 6) continue;

        auto flags =
            static_cast<settings::trans_rule_flags_t>(vl.at(0).toUInt());

        switch (target) {
            case transform_text_target_t::stt:
                if ((flags &
                     settings::trans_rule_flags_t::TransRuleTargetStt) == 0)
                    continue;
                break;
            case transform_text_target_t::tts:
                if ((flags &
                     settings::trans_rule_flags_t::TransRuleTargetTts) == 0)
                    continue;
                break;
        }

        auto langs = vl.at(5).toString().trimmed();
        if (lang.isEmpty()) {
            if (!langs.isEmpty()) {
                // rule for specific language but tts lang is unknown
                continue;
            }
        } else {
            if (!langs.isEmpty() &&
                !langs.contains(lang, Qt::CaseInsensitive)) {
                // rule for different language
                continue;
            }
        }

        auto actions = apply_trans_rule(
            text, {/*flags=*/flags,
                   /*type=*/
                   static_cast<settings::trans_rule_type_t>(vl.at(1).toUInt()),
                   /*name=*/vl.at(2).toString(),
                   /*pattern=*/vl.at(3).toString(),
                   /*replace=*/vl.at(4).toString()});

        result.action_flags |= actions;
    }

    return result;
}

QVariantList dsnote_app::test_trans_rule(unsigned int flags, const QString &text,
                                         const QString &pattern,
                                         const QString &replace,
                                         unsigned int type) {
    QString out_text{text};

    if (pattern.isEmpty()) {
        //qWarning() << "invalid trans rule";
        return {false, out_text};
    }

    auto rflags = apply_trans_rule(
        out_text, {/*flags=*/static_cast<settings::trans_rule_flags_t>(flags),
                   /*type=*/static_cast<settings::trans_rule_type_t>(type),
                   /*name=*/{},
                   /*pattern=*/pattern,
                   /*replace=*/replace});

    return {(rflags & settings::trans_rule_flags_t::TransRuleMatched) > 0,
            out_text};
}

bool dsnote_app::trans_rule_re_pattern_valid(const QString &pattern) {
    return QRegExp{pattern}.isValid();
}

void dsnote_app::update_trans_rule(int index, unsigned int flags,
                                   const QString &name, const QString &pattern,
                                   const QString &replace, const QString &langs,
                                   unsigned int type, const QString &test_text) {
    if (pattern.isEmpty()) {
        //qWarning() << "invalid trans rule";
        return;
    }

    auto rules = settings::instance()->trans_rules();
    if (index >= rules.size()) {
        qWarning() << "invalid trans rule index";
        return;
    }

    QVariantList rule{
        /*[0] flags=*/flags,
        /*[1] type=*/
        static_cast<std::underlying_type_t<settings::trans_rule_type_t>>(type),
        /*[2] name=*/name,
        /*[3] pattern=*/pattern,
        /*[4] replace=*/replace,
        /*[5] langs=*/langs,
        /*[6] test_text=*/test_text,
    };

    if (index < 0) {
        // new rule
        rules.push_back(rule);
    } else {
        // update existing rule
        rules[index] = rule;
    }

    settings::instance()->set_trans_rules(rules);
}

QString dsnote_app::trans_rules_test_text() const { return m_trans_rules_test_text; }

void dsnote_app::set_trans_rules_test_text(const QString &value) {
    if (m_trans_rules_test_text != value) {
        m_trans_rules_test_text = value;
        emit trans_rules_test_text_changed();
    }
}

void dsnote_app::update_voice_prompt(const QString &name,
                                     const QString &new_name,
                                     const QString &desc) const {
    if (new_name.isEmpty() || desc.isEmpty()) {
        qWarning() << "empty voice prompt";
        return;
    }

    settings::instance()->tts_update_voice_prompt(name, {new_name, desc});
}

void dsnote_app::delete_voice_prompt(const QString &name) const {
    if (name.isEmpty()) {
        qWarning() << "empty voice prompt name";
        return;
    }

    settings::instance()->tts_delete_voice_prompt(name);
}

void dsnote_app::clone_voice_prompt(const QString &name) const {
    if (name.isEmpty()) {
        qWarning() << "empty voice prompt name";
        return;
    }

    settings::instance()->tts_clone_voice_prompt(name);
}

bool dsnote_app::test_voice_prompt(const QString &name, const QString &new_name,
                                   const QString &desc) const {
    if (new_name.isEmpty() || desc.isEmpty()) return false;

    bool new_exists =
        static_cast<bool>(settings::instance()->tts_voice_prompt(new_name));

    if (name.isEmpty()) {
        return !new_exists;
    }

    return name == new_name ? new_exists : !new_exists;
}

void dsnote_app::handle_stt_text_decoded(QString text, const QString &lang,
                                         int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    update_stt_auto_lang(lang);

    if (settings::instance()->trans_rules_enabled()) {
        transform_text(text, transform_text_target_t::stt, lang);
    }

    switch (m_text_destination) {
        case text_destination_t::note_add: {
            make_undo();
            auto n =
                insert_to_note(settings::instance()->note(), text, lang,
                               settings::instance()->stt_tts_text_format() ==
                                       settings::text_format_t::TextFormatSubRip
                                   ? settings::insert_mode_t::InsertInLine
                                   : settings::instance()->insert_mode(),
                               m_last_cursor_position);
            set_note(n.first);
            set_last_cursor_position(n.second);
            this->m_intermediate_text.clear();
            emit text_changed();
            emit intermediate_text_changed();
            break;
        }
        case text_destination_t::note_replace: {
            make_undo();
            auto n = insert_to_note(QString{}, text, lang,
                                    settings::instance()->insert_mode(), -1);
            set_note(n.first);
            set_last_cursor_position(n.second);
            m_text_destination = text_destination_t::note_add;
            this->m_intermediate_text.clear();
            emit text_changed();
            emit intermediate_text_changed();
            break;
        }
        case text_destination_t::active_window:
#ifdef USE_DESKTOP
            try {
                m_fake_keyboard.emplace();
                m_fake_keyboard->send_text(text);
            } catch (const std::runtime_error &err) {
                qWarning() << "fake-keyboard error:" << err.what();
            }

            emit text_decoded_to_active_window();
#else
            qWarning() << "send to keyboard is not implemented";
#endif
            break;
        case text_destination_t::clipboard:
            QGuiApplication::clipboard()->setText(text);
            emit text_decoded_to_clipboard();
            break;
        case text_destination_t::internal:
            emit text_decoded_internal(text);
            return;
    }

    // echo mode handling
    if (settings::instance()->stt_echo() && m_current_stt_request &&
        m_current_stt_request.value() != stt_request_t::transcribe_file) {
        if (service_state() == service_state_t::StateListeningAuto &&
            m_current_stt_request) {
            // in always-on listening, set pending request to resume listening
            // just after playing speech
            m_pending_stt_request = m_current_stt_request;
        }

        play_speech_from_text(text, active_tts_model());
    }
}

void dsnote_app::handle_tts_partial_speech(const QString &text,
                                         int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
                    speech_service::instance()->task_state());
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
                &speech_service::ttt_repair_text_finished, this,
                &dsnote_app::handle_ttt_repair_text_finished,
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
                &speech_service::mnt_translate_progress_changed, this,
                &dsnote_app::handle_mnt_translate_progress,
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
                &OrgMkiolSpeechInterface::TttRepairTextFinished, this,
                &dsnote_app::handle_ttt_repair_text_finished);
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
        connect(&m_dbus_service, &OrgMkiolSpeechInterface::MntTranslateProgress,
                this, &dsnote_app::handle_mnt_translate_progress);
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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        qDebug() << "ttt models changed";
    } else {
        qDebug() << "[dbus => app] signal TttModelsPropertyChanged";
    }

    m_available_ttt_models_map = models;
    emit available_ttt_models_changed();

    update_configured_state();
}

void dsnote_app::handle_state_changed(int status) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal StatusPropertyChanged:" << status;
    }

    set_service_state(static_cast<service_state_t>(status));
}

void dsnote_app::handle_task_state_changed(int state) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TaskStatePropertyChanged:" << state;
    }

    if (m_primary_task != m_current_task) {
        qWarning() << "ignore TaskStatePropertyChanged signal";
        return;
    }

    set_task_state(make_new_task_state(state));
}

void dsnote_app::handle_current_task_changed(int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    start_keepalive();
    update_task_state();
}

void dsnote_app::handle_tts_play_speech_finished(int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsPlaySpeechFinished:" << task;
    }

    if (m_pending_stt_request) {
        auto request = m_pending_stt_request.value();
        m_pending_stt_request.reset();

        switch (request) {
            case dsnote_app::stt_request_t::listen:
                listen();
                break;
            case dsnote_app::stt_request_t::listen_translate:
                listen_translate();
                break;
            case dsnote_app::stt_request_t::listen_active_window:
                listen_to_active_window();
                break;
            case dsnote_app::stt_request_t::listen_translate_active_window:
                listen_translate_to_active_window();
                break;
            case dsnote_app::stt_request_t::listen_clipboard:
                listen_to_clipboard();
                break;
            case dsnote_app::stt_request_t::listen_translate_clipboard:
                listen_translate_to_clipboard();
                break;
            case dsnote_app::stt_request_t::transcribe_file:
                break;
        }
    } else {
        this->m_intermediate_text.clear();
        emit intermediate_text_changed();
    }
}

void dsnote_app::handle_ttt_repair_text_finished(const QString &text,
                                                 int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TttRepairTextFinished:" << task;
    }

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    update_note(text, true);

    emit text_repair_done();
}

void dsnote_app::handle_tts_speech_to_file_finished(const QStringList &files,
                                                    int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsSpeechToFileFinished:" << task;
    }

    qDebug() << "speech to file finished";

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    if (m_dest_file_info.output_path.isEmpty()) {
        qWarning() << "input file is empty";
        return;
    }

    if (QFile::exists(m_dest_file_info.output_path))
        QFile::remove(m_dest_file_info.output_path);

    if (m_dest_file_info.input_path.isEmpty()) {
        export_to_audio_internal(files, m_dest_file_info.output_path);
    } else if (QFile::exists(m_dest_file_info.input_path)) {
        export_to_audio_mix_internal(m_dest_file_info.input_path,
                                     m_dest_file_info.input_stream_index, files,
                                     m_dest_file_info.output_path);
    } else {
        qWarning() << "input file doesn't exist:"
                   << m_dest_file_info.input_path;
    }
}

void dsnote_app::handle_tts_speech_to_file_progress(double new_progress,
                                                    int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsSpeechToFileProgress:"
                 << new_progress << task;
    }

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    m_speech_to_file_progress = new_progress;
    emit speech_to_file_progress_changed();
}

void dsnote_app::handle_mnt_translate_progress(double new_progress, int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal MntTranslateProgress:" << new_progress
                 << task;
    }

    if (m_current_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    m_translate_progress = new_progress;

    qDebug() << "translate progress:" << m_translate_progress;
    emit translate_progress_changed();
}

void dsnote_app::handle_mnt_translate_finished(
    [[maybe_unused]] const QString &in_text,
    [[maybe_unused]] const QString &in_lang, const QString &out_text,
    [[maybe_unused]] const QString &out_lang, int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal MntTranslateFinished:" << task;
    }

#ifdef DEBUG
    qDebug() << "translated text:" << out_text;
#endif

    set_translated_text(out_text);
}

void dsnote_app::handle_stt_default_model_changed(const QString &model) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
        update_stt_auto_lang({});
        emit active_stt_model_changed();
    }
}

static QVariantMap::const_iterator active_model_or_lang_it(
    const QString &model_or_lang, const QVariantMap &map) {
    QVariantMap::const_iterator it = map.find(model_or_lang);

    if (it == map.cend()) {
        // trying find model by lang
        it = std::find_if(
            map.cbegin(), map.cend(), [&model_or_lang](const auto &key) {
                auto l = key.toStringList();
                return l.size() > 3 &&
                       l.at(3).compare(model_or_lang, Qt::CaseInsensitive) == 0;
            });
    }

    return it;
}

void dsnote_app::set_active_stt_model_or_lang(const QString &model_or_lang) {
    if (m_active_stt_model != model_or_lang) {
        auto it =
            active_model_or_lang_it(model_or_lang, m_available_stt_models_map);
        if (it == m_available_stt_models_map.cend()) {
            qWarning() << "can't swith to stt model or lang:" << model_or_lang;
        } else {
            set_active_stt_model_idx(
                std::distance(m_available_stt_models_map.cbegin(), it));
        }
    }
}

void dsnote_app::set_active_tts_model_or_lang(const QString &model_or_lang) {
    if (m_active_tts_model != model_or_lang) {
        auto it =
            active_model_or_lang_it(model_or_lang, m_available_tts_models_map);
        if (it == m_available_tts_models_map.cend()) {
            qWarning() << "can't swith to tts model or lang:" << model_or_lang;
        } else {
            set_active_tts_model_idx(
                std::distance(m_available_tts_models_map.cbegin(), it));
        }
    }
}

void dsnote_app::set_active_mnt_lang(const QString &lang) {
    if (m_active_mnt_lang != lang) {
        qDebug() << "app active mnt lang:" << m_active_mnt_lang << "=>" << lang;

        if (lang == active_mnt_out_lang()) {
            m_active_mnt_out_lang = m_active_mnt_lang;
        }

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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsDefaultModelPropertyChanged:"
                 << model;
    }

    set_active_tts_model(model);
}

void dsnote_app::handle_mnt_default_lang_changed(const QString &lang) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal MntDefaultLangPropertyChanged:"
                 << lang;
    }

    set_active_mnt_lang(lang);
}

void dsnote_app::handle_mnt_default_out_lang_changed(const QString &lang) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal SttFileTranscribeProgress:"
                 << new_progress << task;
    }

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    qDebug() << "transcribe progress:" << new_progress;

    m_transcribe_progress = new_progress;
    emit transcribe_progress_changed();
}

void dsnote_app::handle_stt_file_transcribe_finished(int task) {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal SttFileTranscribeFinished:" << task;
    }

    if (m_primary_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    if (m_text_destination != text_destination_t::internal) {
        emit transcribe_done();
    }

    emit can_open_next_file();
}

void dsnote_app::handle_service_error(int code) {
    auto error_code = static_cast<error_t>(code);

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

void dsnote_app::update_progress() {
    double new_stt_progress = 0.0;
    double new_tts_progress = 0.0;

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        new_stt_progress =
            speech_service::instance()->stt_transcribe_file_progress(
                m_primary_task.current);
        new_tts_progress =
            speech_service::instance()->tts_speech_to_file_progress(
                m_primary_task.current);
    } else {
        qDebug() << "[app => dbus] call SttGetFileTranscribeProgress";
        new_stt_progress =
            m_dbus_service.SttGetFileTranscribeProgress(m_primary_task.current);
        qDebug() << "[app => dbus] call TtsGetSpeechToFileProgress";
        new_tts_progress =
            m_dbus_service.TtsGetSpeechToFileProgress(m_primary_task.current);
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        new_state =
            static_cast<service_state_t>(speech_service::instance()->state());
    } else {
        qDebug() << "[app => dbus] get Status";
        new_state = static_cast<service_state_t>(m_dbus_service.state());
    }

    set_service_state(new_state);
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    auto scan_ref_voices = [&] {
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
                file, QStringList{std::move(name), std::move(path),
                                  QString::fromStdString(mtag->comment)});
        }
    };

    scan_ref_voices();

    if (new_available_tts_ref_voices_map.empty() &&
        (settings::instance()->hint_done_flags() &
         settings::hint_done_flags_t::HintDoneRefVoiceDefault) == 0) {
        auto copy_default_ref_voice = [&](int nb) {
            auto voice_ref_file_name = QStringLiteral("voice-%1.opus").arg(nb);
            QFile{
                module_tools::path_to_share_dir_for_path(voice_ref_file_name) +
                '/' + voice_ref_file_name}
                .copy(ref_voices_dir + '/' + voice_ref_file_name);
        };

        // copy default ref-voices
        copy_default_ref_voice(1);
        copy_default_ref_voice(2);

        settings::instance()->set_hint_done(settings::HintDoneRefVoiceDefault);

        scan_ref_voices();
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

        if (m_available_mnt_out_langs_map.contains(active_mnt_out_lang())) {
            if (settings::launch_mode ==
                settings::launch_mode_t::app_stanalone) {
                speech_service::instance()->set_default_mnt_out_lang(
                    active_mnt_out_lang());
            } else {
                qDebug() << "[app => dbus] set DefaultMntOutLang:"
                         << active_mnt_out_lang();
                m_dbus_service.setDefaultMntOutLang(active_mnt_out_lang());
            }
        }

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

QVariantList dsnote_app::available_stt_models_info() const {
    QVariantList list;

    for (auto it = m_available_stt_models_map.constBegin();
         it != m_available_stt_models_map.constEnd(); ++it) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), it.key());
        map.insert(QStringLiteral("name"), it.value().toStringList().at(1));
        list.push_back(map);
    }

    return list;
}

QVariantList dsnote_app::available_tts_models_info() const {
    QVariantList list;

    for (auto it = m_available_tts_models_map.constBegin();
         it != m_available_tts_models_map.constEnd(); ++it) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), it.key());
        map.insert(QStringLiteral("name"), it.value().toStringList().at(1));
        list.push_back(map);
    }

    return list;
}

QVariantList dsnote_app::available_tts_ref_voices() const {
    QVariantList list;

    for (auto it = m_available_tts_ref_voices_map.constBegin();
         it != m_available_tts_ref_voices_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 2) list.push_back(v);
    }

    return list;
}

QVariantList dsnote_app::available_tts_ref_voice_names() const {
    QVariantList list;

    for (auto it = m_available_tts_ref_voices_map.constBegin();
         it != m_available_tts_ref_voices_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 2 && !v.at(2).isEmpty()) {
            list.push_back(v.front());
        }
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

void dsnote_app::update_tts_ref_voice(int idx, const QString &new_name,
                                      const QString &new_text) {
    auto &item = std::next(m_available_tts_ref_voices_map.begin(), idx).value();

    auto list = item.toStringList();
    if (list.size() < 2) return;

    auto file_path_std = list.at(1).toStdString();
    auto mtag = mtag_tools::read(file_path_std);

    if (!mtag) return;

    if (mtag->title == new_name.toStdString() &&
        mtag->comment == new_text.toStdString()) {
        // no change
        return;
    }

    mtag->title.assign(new_name.toStdString());
    mtag->comment.assign(new_text.toStdString());

    if (!mtag_tools::write(file_path_std, mtag.value())) return;

    list[0] = new_name;
    list[2] = new_text;
    item = list;

    emit available_tts_ref_voices_changed();
    emit active_tts_ref_voice_changed();
    emit active_tts_for_in_mnt_ref_voice_changed();
    emit active_tts_for_out_mnt_ref_voice_changed();
}

bool dsnote_app::test_tts_ref_voice(int idx, const QString &new_name) const {
    // find if name already taken
    int i = 0;
    for (auto it = m_available_tts_ref_voices_map.constBegin();
         it != m_available_tts_ref_voices_map.constEnd(); ++it, ++i) {
        if (i == idx) continue;
        auto v = it.value().toStringList();
        if (v.isEmpty()) continue;
        if (v.front() == new_name) {
            return false;
        }
    }

    return true;
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

        if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->set_default_stt_model(id);
        } else {
            qDebug() << "[app => dbus] set DefaultSttModel:" << idx << id;
            m_dbus_service.setDefaultSttModel(id);
        }
    }
}

void dsnote_app::set_active_next_stt_model() {
    auto idx = active_stt_model_idx();
    if (idx < 0) return;

    idx = idx >= m_available_stt_models_map.size() - 1 ? 0 : idx + 1;

    set_active_stt_model_idx(idx);
}

void dsnote_app::set_active_next_tts_model() {
    auto idx = active_tts_model_idx();
    if (idx < 0) return;

    idx = idx >= m_available_tts_models_map.size() - 1 ? 0 : idx + 1;

    set_active_tts_model_idx(idx);
}

void dsnote_app::set_active_prev_stt_model() {
    auto idx = active_stt_model_idx();
    if (idx < 0) return;

    idx = idx == 0 ? m_available_stt_models_map.size() - 1 : idx - 1;

    set_active_stt_model_idx(idx);
}

void dsnote_app::set_active_prev_tts_model() {
    auto idx = active_tts_model_idx();
    if (idx < 0) return;

    idx = idx == 0 ? m_available_tts_models_map.size() - 1 : idx - 1;

    set_active_tts_model_idx(idx);
}

void dsnote_app::set_active_tts_model_idx(int idx) {
    if (active_tts_model_idx() != idx && idx > -1 &&
        idx < m_available_tts_models_map.size()) {
        auto id = std::next(m_available_tts_models_map.cbegin(), idx).key();

        if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

        if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

        if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->set_default_mnt_out_lang(id);
        } else {
            qDebug() << "[app => dbus] set DefaultMntOutLang:" << idx << id;
            m_dbus_service.setDefaultMntOutLang(id);
        }
    }
}

void dsnote_app::switch_mnt_langs() {
    auto in_id = active_mnt_lang();
    auto out_id = active_mnt_out_lang();

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        speech_service::instance()->set_default_mnt_lang(out_id);
        speech_service::instance()->set_default_mnt_out_lang(in_id);
    } else {
        qDebug() << "[app => dbus] set DefaultMntLang:" << out_id;
        m_dbus_service.setDefaultMntLang(out_id);
        qDebug() << "[app => dbus] set DefaultMntOutLang:" << in_id;
        m_dbus_service.setDefaultMntOutLang(in_id);
    }
}

void dsnote_app::cancel_if_internal() {
    if (m_text_destination != text_destination_t::internal) {
        return;
    }

    cancel();
}

void dsnote_app::cancel() {
    if (busy()) return;

    m_current_stt_request.reset();
    m_pending_stt_request.reset();

    if (!m_open_files_delay_timer.isActive()) reset_files_queue();

    switch (m_mc.state()) {
        case media_converter::state_t::idle:
        case media_converter::state_t::error:
            break;
        case media_converter::state_t::cancelling:
            return;
        case media_converter::state_t::importing_subtitles:
        case media_converter::state_t::exporting_to_subtitles:
        case media_converter::state_t::exporting_to_audio:
        case media_converter::state_t::exporting_to_audio_mix:
            qDebug() << "cancelling mc";
            m_mc.cancel();
            return;
    }

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        if (m_primary_task) {
            if (m_primary_task.current != INVALID_TASK) {
                speech_service::instance()->cancel(m_primary_task.current);
            } else {
                speech_service::instance()->cancel(m_primary_task.previous);
            }
        }
    } else {
        qDebug() << "[app => dbus] call Cancel";

        if (m_primary_task) {
            if (m_primary_task.current != INVALID_TASK) {
                m_dbus_service.Cancel(m_primary_task.current);
            } else {
                m_dbus_service.Cancel(m_primary_task.previous);
            }
        }
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::transcribe_file(const QString &file_path, int stream_index,
                                 bool replace) {
    qDebug() << "requested stream index for transcribe:" << stream_index;

    m_text_destination = replace ? text_destination_t::note_replace
                                 : text_destination_t::note_add;

    auto *s = settings::instance();

    int new_task = 0;

    QVariantMap options;
    options.insert("text_format", static_cast<int>(s->stt_tts_text_format()));
    options.insert("sub_min_segment_dur", s->sub_min_segment_dur());
    options.insert("stream_index", stream_index);
    options.insert("insert_stats", s->stt_insert_stats());
    if (s->sub_break_lines()) {
        options.insert("sub_min_line_length", s->sub_min_line_length());
        options.insert("sub_max_line_length", s->sub_max_line_length());
    }
    if (!replace && s->stt_tts_text_format() == settings::text_format_t::TextFormatRaw && s->stt_use_note_as_prompt()) {
        options.insert("initial_prompt", note_as_prompt());
    }

    m_current_stt_request = stt_request_t::transcribe_file;

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_transcribe_file(
            file_path, {},
            s->whisper_translate() ? QStringLiteral("en") : QString{}, options);
    } else {
        qDebug() << "[app => dbus] call SttTranscribeFile";

        new_task = m_dbus_service.SttTranscribeFile(
            file_path, {},
            s->whisper_translate() ? QStringLiteral("en") : QString{}, options);
    }

    m_primary_task.set(new_task);
}

void dsnote_app::transcribe_ref_file_import(long long start, long long stop) {
    auto wav_file_path = import_ref_voice_file_path();
    auto out_file_path = QStringLiteral("%1_cut.ogg").arg(wav_file_path);

    try {
        media_compressor::options_t opts{
            media_compressor::quality_t::vbr_high,
            media_compressor::flags_t::flag_none,
            1.0,
            {},
            /*clip_info=*/
            media_compressor::clip_info_t{static_cast<uint64_t>(start),
                                          static_cast<uint64_t>(stop), 0, 0}};

        media_compressor{}.compress_to_file(
            {wav_file_path.toStdString()}, out_file_path.toStdString(),
            media_compressor::format_t::audio_ogg_opus, opts);
    } catch (const std::runtime_error &error) {
        qWarning() << "can't compress file:" << error.what();
        return;
    }

    transcribe_ref_file(out_file_path);
}

void dsnote_app::transcribe_ref_file(const QString &file_path) {
    m_text_destination = text_destination_t::internal;

    int new_task = 0;

    QVariantMap options;
    options.insert("text_format",
                   static_cast<int>(settings::text_format_t::TextFormatRaw));
    options.insert("stream_index", 0);
    options.insert("insert_stats", false);

    m_current_stt_request = stt_request_t::transcribe_file;

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_transcribe_file(
            file_path, {}, {}, options);
    } else {
        qDebug() << "[app => dbus] call SttTranscribeFile";

        new_task = m_dbus_service.SttTranscribeFile(file_path, {}, {}, options);
    }

    m_primary_task.set(new_task);
}

void dsnote_app::listen() {
    m_text_destination = text_destination_t::note_add;
    m_current_stt_request = stt_request_t::listen;
    listen_internal(stt_translate_req_t::conf);
}

void dsnote_app::listen_translate() {
    m_text_destination = text_destination_t::note_add;
    m_current_stt_request = stt_request_t::listen_translate;
    listen_internal(stt_translate_req_t::on);
}

void dsnote_app::listen_to_active_window() {
    m_text_destination = text_destination_t::active_window;
    m_current_stt_request = stt_request_t::listen_active_window;
    listen_internal(stt_translate_req_t::conf);
}

void dsnote_app::listen_translate_to_active_window() {
    m_text_destination = text_destination_t::active_window;
    m_current_stt_request = stt_request_t::listen_translate_active_window;
    listen_internal(stt_translate_req_t::on);
}

void dsnote_app::listen_to_clipboard() {
    m_text_destination = text_destination_t::clipboard;
    m_current_stt_request = stt_request_t::listen_clipboard;
    listen_internal(stt_translate_req_t::conf);
}

void dsnote_app::listen_translate_to_clipboard() {
    m_text_destination = text_destination_t::clipboard;
    m_current_stt_request = stt_request_t::listen_translate_clipboard;
    listen_internal(stt_translate_req_t::on);
}

void dsnote_app::listen_internal(stt_translate_req_t translate_req) {
    auto *s = settings::instance();

    int new_task = 0;

    QVariantMap options;
    auto text_format =
        m_text_destination == text_destination_t::note_add ||
                m_text_destination == text_destination_t::note_replace
            ? s->stt_tts_text_format()
            : settings::text_format_t::TextFormatRaw;
    options.insert("text_format", static_cast<int>(text_format));
    options.insert("audio_input", s->audio_input_device());
    bool insert_stats =
        m_text_destination == text_destination_t::note_add ||
                m_text_destination == text_destination_t::note_replace
            ? s->stt_insert_stats()
            : false;
    options.insert("insert_stats", insert_stats);
    options.insert("stt_clear_mic_audio_when_decoding", s->stt_clear_mic_audio_when_decoding());
    options.insert("stt_play_beep", s->stt_play_beep());
    options.insert("sub_min_segment_dur", s->sub_min_segment_dur());
    if (s->sub_break_lines()) {
        options.insert("sub_min_line_length", s->sub_min_line_length());
        options.insert("sub_max_line_length", s->sub_max_line_length());
    }
    if (text_format == settings::text_format_t::TextFormatRaw && s->stt_use_note_as_prompt()) {
        options.insert("initial_prompt", note_as_prompt());
    }

    auto out_lang = [&]() {
        switch (translate_req) {
            case stt_translate_req_t::conf:
                return s->whisper_translate() ? QStringLiteral("en")
                                              : QString{};
            case stt_translate_req_t::on:
                return QStringLiteral("en");
            case stt_translate_req_t::off:
                return QString{};
        }
        return QString{};
    }();

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_start_listen(
            static_cast<speech_service::speech_mode_t>(s->speech_mode()), {},
            out_lang, options);
    } else {
        qDebug() << "[app => dbus] call SttStartListen2:" << s->speech_mode();
        new_task = m_dbus_service.SttStartListen2(
            static_cast<int>(s->speech_mode()), {}, out_lang, options);
    }

    m_primary_task.set(new_task);

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::stop_listen() {
    if (m_primary_task) {
        if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->stt_stop_listen(m_primary_task.current);
        } else {
            qDebug() << "[app => dbus] call SttStopListen";
            m_dbus_service.SttStopListen(m_primary_task.current);
        }
    }
}

void dsnote_app::pause_speech() {
    if (!m_primary_task) return;

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

QString dsnote_app::lang_from_model_id(const QString &model_id) {
    auto l = model_id.split('_');
    return l.isEmpty() ? QString{} : l.front().toLower();
}

void dsnote_app::play_speech_internal(QString text, const QString &model_id,
                                      const QString &ref_voice,
                                      const QString &ref_prompt,
                                      settings::text_format_t text_format) {
    if (text.isEmpty()) {
        qWarning() << "text is empty";
        return;
    }

    if (settings::instance()->trans_rules_enabled()) {
        transform_text(text, transform_text_target_t::tts,
                       lang_from_model_id(model_id));
    }

    int new_task = 0;

    QVariantMap options;
    options.insert(
        "speech_speed",
        settings::instance()->stt_tts_text_format() !=
                    settings::text_format_t::TextFormatSubRip ||
                settings::instance()->tts_subtitles_sync() ==
                    settings::tts_subtitles_sync_mode_t::TtsSubtitleSyncOff ||
                settings::instance()->tts_subtitles_sync() ==
                    settings::tts_subtitles_sync_mode_t::
                        TtsSubtitleSyncOnDontFit
            ? settings::instance()->speech_speed()
            : 10);
    options.insert("text_format", static_cast<int>(text_format));
    options.insert(
        "sync_subs",
        static_cast<int>(settings::instance()->tts_subtitles_sync()));
    options.insert("split_into_sentences",
                   settings::instance()->tts_split_into_sentences());
    options.insert("use_engine_speed_control",
                   settings::instance()->tts_use_engine_speed_control());
    options.insert("normalize_audio",
                   settings::instance()->tts_normalize_audio());
    options.insert("tag_mode",
                   static_cast<int>(settings::instance()->tts_tag_mode()));
    options.insert("ref_prompt",
                   settings::instance()->tts_desc_of_voice_prompt(ref_prompt));

    if (m_available_tts_ref_voices_map.contains(ref_voice)) {
        auto l = m_available_tts_ref_voices_map.value(ref_voice).toStringList();
        if (l.size() > 1) options.insert("ref_voice_file", l.at(1));
    }

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
        note(), active_tts_model(),
        tts_ref_voice_needed() ? active_tts_ref_voice() : QString{},
        tts_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt()
            : QString{},
        settings::instance()->stt_tts_text_format());
}

void dsnote_app::play_speech_selected(int start, int end) {
    auto size = note().size();

    if (size == 0) return;

    start = std::clamp(start, 0, size);
    end = end < 0 ? size : std::clamp(end, start, size);

    if (start == end) return;

    play_speech_internal(
        note().mid(start, end - start), active_tts_model(),
        tts_ref_voice_needed() ? active_tts_ref_voice() : QString{},
        tts_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt()
            : QString{},
        settings::instance()->stt_tts_text_format());
}

void dsnote_app::play_speech_translator_selected(int start, int end,
                                                 bool transtalated) {
    auto size = transtalated ? m_translated_text.size() : note().size();

    if (size == 0) return;

    start = std::clamp(start, 0, size);
    end = end < 0 ? size : std::clamp(end, start, size);

    if (start == end) return;

    if (!transtalated && m_active_tts_model_for_in_mnt.isEmpty()) {
        qWarning() << "no active tts model for in mnt";
        return;
    }

    if (transtalated && m_active_tts_model_for_out_mnt.isEmpty()) {
        qWarning() << "no active tts model for out mnt";
        return;
    }

    play_speech_internal(
        transtalated ? m_translated_text.mid(start, end - start)
                     : note().mid(start, end - start),
        transtalated ? m_active_tts_model_for_out_mnt
                     : m_active_tts_model_for_in_mnt,
        transtalated                        ? tts_for_out_mnt_ref_voice_needed()
                                                  ? active_tts_for_out_mnt_ref_voice()
                                                  : QString{}
                               : tts_for_in_mnt_ref_voice_needed() ? active_tts_for_in_mnt_ref_voice()
                                            : QString{},
        transtalated
            ? tts_for_out_mnt_ref_prompt_needed()
                  ? settings::instance()->tts_active_voice_prompt_for_out_mnt()
                  : QString{}
        : tts_for_in_mnt_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt_for_in_mnt()
            : QString{},
        settings::instance()->mnt_text_format());
}

void dsnote_app::restore_diacritics_ar() {
    repair_text(text_repair_task_type_t::restore_diacritics_ar);
}

void dsnote_app::restore_diacritics_he() {
    repair_text(text_repair_task_type_t::restore_diacritics_he);
}

void dsnote_app::restore_punctuation() {
    repair_text(text_repair_task_type_t::restore_punctuation);
}

void dsnote_app::repair_text(text_repair_task_type_t task_type) {
    if (note().isEmpty()) {
        qWarning() << "text is empty";
        return;
    }

    int new_task = 0;

    QVariantMap options;
    options.insert(
        "text_format",
        static_cast<int>(settings::instance()->stt_tts_text_format()));
    options.insert("task_type", static_cast<int>(task_type));

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->ttt_repair_text(note(), options);
    } else {
        qDebug() << "[app => dbus] call TttRepairText";

        new_task = m_dbus_service.TttRepairText(note(), options);
    }

    m_primary_task.set(new_task);

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::play_speech_from_text(const QString &text,
                                       const QString &model_id) {
    play_speech_internal(text, model_id,
                         tts_ref_voice_needed_by_id(model_id)
                             ? active_tts_ref_voice()
                             : QString{},
                         tts_ref_prompt_needed_by_id(model_id)
                             ? settings::instance()->tts_active_voice_prompt()
                             : QString{},
                         settings::text_format_t::TextFormatRaw);
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

    play_speech_internal(
        transtalated ? m_translated_text : note(),
        transtalated ? m_active_tts_model_for_out_mnt
                     : m_active_tts_model_for_in_mnt,
        transtalated                        ? tts_for_out_mnt_ref_voice_needed()
                                                  ? active_tts_for_out_mnt_ref_voice()
                                                  : QString{}
                               : tts_for_in_mnt_ref_voice_needed() ? active_tts_for_in_mnt_ref_voice()
                                            : QString{},
        transtalated
            ? tts_for_out_mnt_ref_prompt_needed()
                  ? settings::instance()->tts_active_voice_prompt_for_out_mnt()
                  : QString{}
        : tts_for_in_mnt_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt_for_in_mnt()
            : QString{},
        settings::instance()->mnt_text_format());
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

void dsnote_app::translate_selected(int start, int end) {
    auto size = note().size();

    if (size == 0) return;

    start = std::clamp(start, 0, size);
    end = end < 0 ? size : std::clamp(end, start, size);

    if (start == end) return;

    translate_internal(note().mid(start, end - start));
}

void dsnote_app::translate() { translate_internal(note()); }

void dsnote_app::translate_internal(const QString &text) {
    if (m_active_mnt_lang.isEmpty() || m_active_mnt_out_lang.isEmpty()) {
        qWarning() << "invalid active mnt lang";
        return;
    }

    if (text.isEmpty()) {
        set_translated_text({});
    } else {
        int new_task = 0;

        QVariantMap options;
        options.insert("clean_text", settings::instance()->mnt_clean_text());
        options.insert(
            "text_format",
            static_cast<int>(settings::instance()->mnt_text_format()));

        if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
            new_task = speech_service::instance()->mnt_translate(
                text, m_active_mnt_lang, m_active_mnt_out_lang, options);
        } else {
            qDebug() << "[app => dbus] call MntTranslate";

            new_task = m_dbus_service.MntTranslate2(
                text, m_active_mnt_lang, m_active_mnt_out_lang, options);
        }

        m_primary_task.set(new_task);
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::speech_to_file(const QString &dest_file,
                                const QString &title_tag,
                                const QString &track_tag) {
    m_dest_file_info = dest_file_info_t{};

    speech_to_file_internal(
        note(), active_tts_model(), dest_file, title_tag, track_tag,
        tts_ref_voice_needed() ? active_tts_ref_voice() : QString{},
        tts_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt()
            : QString{},
        settings::instance()->stt_tts_text_format(),
        settings::instance()->audio_format(),
        settings::instance()->audio_quality());
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

    m_dest_file_info = dest_file_info_t{};

    speech_to_file_internal(
        transtalated ? m_translated_text : note(),
        transtalated ? m_active_tts_model_for_out_mnt
                     : m_active_tts_model_for_in_mnt,
        dest_file, title_tag, track_tag,
        transtalated                        ? tts_for_out_mnt_ref_voice_needed()
                                                  ? active_tts_for_out_mnt_ref_voice()
                                                  : QString{}
                               : tts_for_in_mnt_ref_voice_needed() ? active_tts_for_in_mnt_ref_voice()
                                            : QString{},
        transtalated
            ? tts_for_out_mnt_ref_prompt_needed()
                  ? settings::instance()->tts_active_voice_prompt_for_out_mnt()
                  : QString{}
        : tts_for_in_mnt_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt_for_in_mnt()
            : QString{},
        settings::instance()->mnt_text_format(),
        settings::instance()->audio_format(),
        settings::instance()->audio_quality());
}

void dsnote_app::speech_to_file_internal(
    QString text, const QString &model_id, const QString &dest_file,
    const QString &title_tag, const QString &track_tag,
    const QString &ref_voice, const QString &ref_prompt,
    settings::text_format_t text_format, settings::audio_format_t audio_format,
    settings::audio_quality_t audio_quality) {
    if (text.isEmpty()) {
        qWarning() << "text is empty";
        return;
    }

    if (dest_file.isEmpty()) {
        qWarning() << "dest file is empty";
        return;
    }

    if (settings::instance()->trans_rules_enabled()) {
        transform_text(text, transform_text_target_t::tts,
                       lang_from_model_id(model_id));
    }

    int new_task = 0;

    QVariantMap options;
    options.insert(
        "speech_speed",
        settings::instance()->stt_tts_text_format() !=
                    settings::text_format_t::TextFormatSubRip ||
                settings::instance()->tts_subtitles_sync() ==
                    settings::tts_subtitles_sync_mode_t::TtsSubtitleSyncOff ||
                settings::instance()->tts_subtitles_sync() ==
                    settings::tts_subtitles_sync_mode_t::
                        TtsSubtitleSyncOnDontFit
            ? settings::instance()->speech_speed()
            : 10);
    options.insert("text_format", static_cast<int>(text_format));
    options.insert(
        "sync_subs",
        static_cast<int>(settings::instance()->tts_subtitles_sync()));
    options.insert("not_merge_files", true);
    options.insert("split_into_sentences",
                   settings::instance()->tts_split_into_sentences());
    options.insert("use_engine_speed_control",
                   settings::instance()->tts_use_engine_speed_control());
    options.insert("normalize_audio",
                   settings::instance()->tts_normalize_audio());
    options.insert("tag_mode",
                   static_cast<int>(settings::instance()->tts_tag_mode()));
    options.insert("ref_prompt",
                   settings::instance()->tts_desc_of_voice_prompt(ref_prompt));

    if (m_available_tts_ref_voices_map.contains(ref_voice)) {
        auto l = m_available_tts_ref_voices_map.value(ref_voice).toStringList();
        if (l.size() > 1) options.insert("ref_voice_file", l.at(1));
    }

    auto audio_format_str =
        settings::audio_format_str_from_filename(audio_format, dest_file);
    auto audio_ext = settings::audio_ext_from_filename(audio_format, dest_file);

    options.insert("audio_format", audio_format_str);
    options.insert("audio_quality", audio_quality_to_str(audio_quality));

    if (QFileInfo{dest_file}.suffix().toLower() != audio_ext) {
        qDebug() << "file name doesn't have proper extension for audio format";
        if (dest_file.endsWith('.'))
            m_dest_file_info.output_path = dest_file + audio_ext;
        else
            m_dest_file_info.output_path =
                m_dest_file_info.output_path + '.' + audio_ext;
    } else {
        m_dest_file_info.output_path = dest_file;
    }

    m_dest_file_info.title_tag = title_tag;
    m_dest_file_info.track_tag = track_tag;

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->tts_speech_to_file(
            text, model_id, options);
    } else {
        qDebug() << "[app => dbus] call TtsSpeechToFile";

        new_task = m_dbus_service.TtsSpeechToFile(text, model_id, options);
    }

    m_primary_task.set(new_task);
}

void dsnote_app::stop_play_speech() {}

void dsnote_app::set_service_state(service_state_t new_service_state) {
    switch (m_mc.state()) {
        case media_converter::state_t::idle:
        case media_converter::state_t::error:
        case media_converter::state_t::cancelling:
            break;
        case media_converter::state_t::importing_subtitles:
            new_service_state = service_state_t::StateImporting;
            break;
        case media_converter::state_t::exporting_to_subtitles:
        case media_converter::state_t::exporting_to_audio:
        case media_converter::state_t::exporting_to_audio_mix:
            new_service_state = service_state_t::StateExporting;

            break;
    }

    if (m_service_state != new_service_state) {
        const auto old_busy = busy();
        const auto old_connected = connected();

        qDebug() << "app service state:" << m_service_state << "=>"
                 << new_service_state;

        m_service_state = new_service_state;

        if (settings::launch_mode == settings::launch_mode_t::app &&
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

        if (old_busy != busy()) {
            qDebug() << "app busy:" << old_busy << "=>" << busy();
            emit busy_changed();
        }

        if (old_connected != connected()) {
            qDebug() << "app connected:" << old_connected << " = > "
                     << connected();
            emit connected_changed();
        }
    }
}

void dsnote_app::set_task_state(service_task_state_t new_task_state) {
    switch (m_mc.state()) {
        case media_converter::state_t::cancelling:
            new_task_state = service_task_state_t::TaskStateCancelling;
            break;
        case media_converter::state_t::idle:
        case media_converter::state_t::error:
        case media_converter::state_t::importing_subtitles:
        case media_converter::state_t::exporting_to_subtitles:
        case media_converter::state_t::exporting_to_audio:
        case media_converter::state_t::exporting_to_audio_mix:
            break;
    }

    if (m_task_state != new_task_state) {
        qDebug() << "app task state:" << m_task_state << "=>" << new_task_state;
        m_task_state = new_task_state;
        emit task_state_changed();

        update_tray_task_state();
    }
}

dsnote_app::service_task_state_t dsnote_app::make_new_task_state(
    int task_state_from_service) {
    switch (task_state_from_service) {
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
        case 5:
            return service_task_state_t::TaskStateSpeechPaused;
        case 6:
            return service_task_state_t::TaskStateCancelling;
    }
    return service_task_state_t::TaskStateIdle;
}

void dsnote_app::update_task_state() {
    if (m_primary_task != m_current_task) {
        qWarning() << "invalid task, reseting task state";
        set_task_state(service_task_state_t::TaskStateIdle);
        return;
    }

    int task_state_from_service = 0;
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
        task_state_from_service = speech_service::instance()->task_state();
    } else {
        qDebug() << "[app => dbus] get TaskState";
        task_state_from_service = m_dbus_service.taskState();
    }
    set_task_state(make_new_task_state(task_state_from_service));
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) return;

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
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) return;

    if (!m_keepalive_current_task_timer.isActive() &&
        (m_primary_task == m_current_task)) {
        if (m_primary_task == m_current_task)
            qDebug() << "starting keepalive current task";
        m_keepalive_current_task_timer.start(KEEPALIVE_TIME);
    }
}

void dsnote_app::handle_keepalive_task_timeout() {
    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) return;

    const auto send_ka = [this](int task) {
        qDebug() << "[app => dbus] call KeepAliveTask";
        auto time = m_dbus_service.KeepAliveTask(task);

        if (time > 0) {
            m_keepalive_current_task_timer.start(static_cast<int>(time * 0.75));
        } else {
            qWarning() << "keepalive task failed";
            if (m_primary_task) m_primary_task.reset();

            update_current_task();
            update_service_state();

            set_task_state(service_task_state_t::TaskStateIdle);
        }
    };

    if (m_primary_task.current > INVALID_TASK) {
        qDebug() << "keepalive task timeout:" << m_primary_task.current;
        send_ka(m_primary_task.current);
    }
}

QVariantMap dsnote_app::translations() const {
    if (settings::launch_mode == settings::launch_mode_t::service &&
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

    auto new_ttt_diacritizer_ar_configured = [this] {
        if (m_service_state == service_state_t::StateUnknown ||
            m_service_state == service_state_t::StateNotConfigured ||
            m_available_ttt_models_map.empty())
            return false;
        return m_available_ttt_models_map.contains("ar_tashkeel_diacritizer");
    }();

    auto new_ttt_diacritizer_he_configured = [this] {
        if (m_service_state == service_state_t::StateUnknown ||
            m_service_state == service_state_t::StateNotConfigured ||
            m_available_ttt_models_map.empty())
            return false;
        return m_available_ttt_models_map.contains("he_unikud_diacritizer");
    }();

    auto new_ttt_punctuation_configured = [this] {
        if (m_service_state == service_state_t::StateUnknown ||
            m_service_state == service_state_t::StateNotConfigured ||
            m_available_ttt_models_map.empty())
            return false;
        return m_available_ttt_models_map.contains("bg_hftc_kredor") ||
               m_available_ttt_models_map.contains("en_hftc_kredor") ||
               m_available_ttt_models_map.contains("de_hftc_kredor") ||
               m_available_ttt_models_map.contains("fr_hftc_kredor") ||
               m_available_ttt_models_map.contains("es_hftc_kredor") ||
               m_available_ttt_models_map.contains("it_hftc_kredor") ||
               m_available_ttt_models_map.contains("pl_hftc_kredor") ||
               m_available_ttt_models_map.contains("nl_hftc_kredor") ||
               m_available_ttt_models_map.contains("cs_hftc_kredor") ||
               m_available_ttt_models_map.contains("pt_hftc_kredor") ||
               m_available_ttt_models_map.contains("sl_hftc_kredor") ||
               m_available_ttt_models_map.contains("sv_hftc_kredor") ||
               m_available_ttt_models_map.contains("hu_hftc_kredor") ||
               m_available_ttt_models_map.contains("ro_hftc_kredor") ||
               m_available_ttt_models_map.contains("sk_hftc_kredor") ||
               m_available_ttt_models_map.contains("da_hftc_kredor");
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

    if (m_ttt_diacritizer_ar_configured != new_ttt_diacritizer_ar_configured) {
        qDebug() << "app ttt diacritize-ar configured:"
                 << m_ttt_diacritizer_ar_configured << "=>"
                 << new_ttt_diacritizer_ar_configured;
        m_ttt_diacritizer_ar_configured = new_ttt_diacritizer_ar_configured;
        emit ttt_diacritizer_ar_configured_changed();
    }

    if (m_ttt_diacritizer_he_configured != new_ttt_diacritizer_he_configured) {
        qDebug() << "app ttt diacritize-he configured:"
                 << m_ttt_diacritizer_he_configured << "=>"
                 << new_ttt_diacritizer_he_configured;
        m_ttt_diacritizer_he_configured = new_ttt_diacritizer_he_configured;
        emit ttt_diacritizer_he_configured_changed();
    }

    if (m_ttt_punctuation_configured != new_ttt_punctuation_configured) {
        qDebug() << "app ttt punctuation configured:"
                 << m_ttt_punctuation_configured << "=>"
                 << new_ttt_punctuation_configured;
        m_ttt_punctuation_configured = new_ttt_punctuation_configured;
        emit ttt_punctuation_configured_changed();
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
           (settings::launch_mode == settings::launch_mode_t::app &&
            !m_service_reload_update_done);
}

bool dsnote_app::stt_configured() const { return m_stt_configured; }

bool dsnote_app::tts_configured() const { return m_tts_configured; }

bool dsnote_app::ttt_diacritizer_ar_configured() const {
    return m_ttt_diacritizer_ar_configured;
}

bool dsnote_app::ttt_diacritizer_he_configured() const {
    return m_ttt_diacritizer_he_configured;
}

bool dsnote_app::ttt_punctuation_configured() const {
    return m_ttt_punctuation_configured;
}

bool dsnote_app::mnt_configured() const { return m_mnt_configured; }

bool dsnote_app::connected() const {
    return m_service_state != service_state_t::StateUnknown;
}

double dsnote_app::transcribe_progress() const { return m_transcribe_progress; }

double dsnote_app::speech_to_file_progress() const {
    return m_speech_to_file_progress;
}

double dsnote_app::translate_progress() const { return m_translate_progress; }

bool dsnote_app::another_app_connected() const {
    return m_current_task != INVALID_TASK && m_primary_task != m_current_task;
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

QVariantMap dsnote_app::file_info(const QString &file) const {
    QVariantMap map;

    if (!QFileInfo::exists(file)) return map;

    auto info = media_compressor{}.media_info(file.toStdString());

    QString type = "unknown";

    if (info) {
        if (!info->audio_streams.empty())
            type = "audio";
        else if (!info->subtitles_streams.empty())
            type = "sub";
        map.insert("type", type);

        map.insert("audio_streams", make_streams_names(info->audio_streams));
        map.insert("sub_streams", make_streams_names(info->subtitles_streams));
    }

    map.insert("type", type);

    return map;
}

QString dsnote_app::translated_text() const { return m_translated_text; }

void dsnote_app::set_translated_text(const QString text) {
    if (text != m_translated_text) {
        m_translated_text = text;
        emit translated_text_changed();
    }
}

QString dsnote_app::note() const { return settings::instance()->note(); }

QString dsnote_app::note_as_prompt() const {
    return settings::instance()->note().right(5000).simplified();
}

void dsnote_app::set_note(const QString &text) {
    auto old = can_undo_or_redu_note();
    settings::instance()->set_note(text);
    if (old != can_undo_or_redu_note()) emit can_undo_or_redu_note_changed();
    if (text.isEmpty()) set_translated_text({});
}

void dsnote_app::update_note(const QString &text, bool replace) {
    make_undo();

    if (replace) {
        set_note(text);
        set_last_cursor_position(text.size());
    } else {
        auto n = insert_to_note(settings::instance()->note(), text, "",
                                settings::instance()->insert_mode(),
                                m_last_cursor_position);
        set_note(n.first);
        set_last_cursor_position(n.second);
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
    } else if (service_state() == service_state_t::StateTranslating) {
        cancel();
    }
}

void dsnote_app::handle_note_changed() {
    emit note_changed();

    if (!settings::instance()->subtitles_support() || note().isEmpty()) {
        settings::instance()->set_stt_tts_text_format(
            settings::text_format_t::TextFormatRaw);
        settings::instance()->set_mnt_text_format(
            settings::text_format_t::TextFormatRaw);
    }

    if (settings::instance()->translator_mode() &&
        settings::instance()->translate_when_typing())
        translate_delayed();

    update_auto_text_format_delayed();
}

static QString make_note_tmp_file(const QString &text) {
    auto tmp_file = QDir{settings::instance()->cache_dir()}.filePath("tmp.srt");

    QFile file{tmp_file};

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "failed to open tmp file for write";
        return {};
    }

    QTextStream out{&file};
    out << text;
    file.close();

    return tmp_file;
}

void dsnote_app::export_to_audio_mix_translator(bool transtalated,
                                                const QString &input_file,
                                                int input_stream_index,
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

    m_dest_file_info = dest_file_info_t{};
    m_dest_file_info.input_path = input_file;
    m_dest_file_info.input_stream_index = input_stream_index;

    speech_to_file_internal(
        transtalated ? m_translated_text : note(),
        transtalated ? m_active_tts_model_for_out_mnt
                     : m_active_tts_model_for_in_mnt,
        dest_file, title_tag, track_tag,
        transtalated                        ? tts_for_out_mnt_ref_voice_needed()
                                                  ? active_tts_for_out_mnt_ref_voice()
                                                  : QString{}
                               : tts_for_in_mnt_ref_voice_needed() ? active_tts_for_in_mnt_ref_voice()
                                            : QString{},
        transtalated
            ? tts_for_out_mnt_ref_prompt_needed()
                  ? settings::instance()->tts_active_voice_prompt_for_out_mnt()
                  : QString{}
        : tts_for_in_mnt_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt_for_in_mnt()
            : QString{},
        settings::instance()->mnt_text_format(),
        settings::audio_format_t::AudioFormatOggOpus,
        settings::audio_quality_t::AudioQualityVbrHigh);
}

void dsnote_app::export_to_audio_mix(const QString &input_file,
                                     int input_stream_index,
                                     const QString &dest_file,
                                     const QString &title_tag,
                                     const QString &track_tag) {
    m_dest_file_info = dest_file_info_t{};
    m_dest_file_info.input_path = input_file;
    m_dest_file_info.input_stream_index = input_stream_index;

    speech_to_file_internal(
        note(), active_tts_model(), dest_file, title_tag, track_tag,
        tts_ref_voice_needed() ? active_tts_ref_voice() : QString{},
        tts_ref_prompt_needed()
            ? settings::instance()->tts_active_voice_prompt()
            : QString{},
        settings::instance()->stt_tts_text_format(),
        settings::audio_format_t::AudioFormatOggOpus,
        settings::audio_quality_t::AudioQualityVbrHigh);
}

void dsnote_app::export_to_audio_internal(const QStringList &input_files,
                                          const QString &dest_file) {
    auto format = settings::instance()->audio_format();

    if (format == settings::audio_format_t::AudioFormatAuto)
        format = settings::filename_to_audio_format_static(
            QFileInfo{dest_file}.fileName());

    if (!m_mc.export_to_audio_async(
            input_files, dest_file,
            [format] {
                switch (format) {
                    case settings::audio_format_t::AudioFormatMp3:
                        return media_compressor::format_t::audio_mp3;
                    case settings::audio_format_t::AudioFormatOggOpus:
                        return media_compressor::format_t::audio_ogg_opus;
                    case settings::audio_format_t::AudioFormatOggVorbis:
                        return media_compressor::format_t::audio_ogg_vorbis;
                    case settings::audio_format_t::AudioFormatWav:
                        return media_compressor::format_t::audio_wav;
                    case settings::audio_format_t::AudioFormatAuto:
                        break;
                }
                throw std::runtime_error{"invalid format"};
            }(),
            audio_quality_to_media_quality(
                settings::instance()->audio_quality()))) {
        qWarning() << "failed export to audio";
        emit error(error_t::ErrorExportFileGeneral);
    }
}

void dsnote_app::export_to_audio_mix_internal(const QString &main_input_file,
                                              int main_stream_index,
                                              const QStringList &input_files,
                                              const QString &dest_file) {
    auto format = settings::instance()->audio_format();

    if (format == settings::audio_format_t::AudioFormatAuto)
        format = settings::filename_to_audio_format_static(
            QFileInfo{dest_file}.fileName());

    if (!m_mc.export_to_audio_mix_async(
            main_input_file, main_stream_index, input_files, dest_file,
            [format] {
                switch (format) {
                    case settings::audio_format_t::AudioFormatMp3:
                        return media_compressor::format_t::audio_mp3;
                    case settings::audio_format_t::AudioFormatOggOpus:
                        return media_compressor::format_t::audio_ogg_opus;
                    case settings::audio_format_t::AudioFormatOggVorbis:
                        return media_compressor::format_t::audio_ogg_vorbis;
                    case settings::audio_format_t::AudioFormatWav:
                        return media_compressor::format_t::audio_wav;
                    case settings::audio_format_t::AudioFormatAuto:
                        break;
                }
                throw std::runtime_error{"invalid format"};
            }(),
            audio_quality_to_media_quality(
                settings::instance()->audio_quality()),
            settings::instance()->mix_volume_change())) {
        qWarning() << "failed export to audio mix";
        emit error(error_t::ErrorExportFileGeneral);
    }
}

void dsnote_app::export_to_subtitles(const QString &dest_file,
                                     settings::text_file_format_t format,
                                     const QString &text) {
    auto tmp_file = make_note_tmp_file(text);
    if (tmp_file.isEmpty()) {
        emit error(error_t::ErrorExportFileGeneral);
        return;
    }

    if (!m_mc.export_to_subtitles_async(tmp_file, dest_file, [format] {
            switch (format) {
                case settings::text_file_format_t::TextFileFormatAss:
                    return media_compressor::format_t::sub_ass;
                case settings::text_file_format_t::TextFileFormatVtt:
                    return media_compressor::format_t::sub_vtt;
                case settings::text_file_format_t::TextFileFormatSrt:
                case settings::text_file_format_t::TextFileFormatRaw:
                case settings::text_file_format_t::TextFileFormatAuto:
                    break;
            }
            return media_compressor::format_t::sub_srt;
        }())) {
        qWarning() << "failed to export to subtitles:" << dest_file;
        emit error(error_t::ErrorExportFileGeneral);
    }
}

void dsnote_app::export_to_text_file(const QString &dest_file,
                                     bool translation) {
    auto format = settings::instance()->text_file_format();

    if (format == settings::text_file_format_t::TextFileFormatAuto)
        format = settings::filename_to_text_file_format_static(
            QFileInfo{dest_file}.fileName());

    switch (format) {
        case settings::text_file_format_t::TextFileFormatRaw:
            export_to_file_internal(translation ? translated_text() : note(),
                                    dest_file);
            break;
        case settings::text_file_format_t::TextFileFormatSrt:
        case settings::text_file_format_t::TextFileFormatAss:
        case settings::text_file_format_t::TextFileFormatVtt:
            export_to_subtitles(dest_file, format,
                                translation ? translated_text() : note());
            break;
        case settings::text_file_format_t::TextFileFormatAuto:
            break;
    }
}

void dsnote_app::export_to_file_internal(const QString &text,
                                         const QString &dest_file) {
    QFile file{dest_file};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "failed to open file for write:" << dest_file;
        emit error(error_t::ErrorExportFileGeneral);
        return;
    }

    QTextStream out{&file};
    out << text;

    file.close();

    emit save_note_to_file_done();
}

bool dsnote_app::import_text_file(const QString &input_file, bool replace) {
    QFile file{input_file};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "failed to open file for read:" << input_file;
        emit error(error_t::ErrorImportFileGeneral);
        return false;
    }

    make_undo();

    if (replace) {
        auto n = QString::fromUtf8(file.readAll());
        set_note(n);
        set_last_cursor_position(n.size());
    } else {
        auto n = insert_to_note(settings::instance()->note(), file.readAll(),
                                "", settings::instance()->insert_mode(), -1);
        set_note(n.first);
        set_last_cursor_position(n.second);
    }

    return true;
}

void dsnote_app::import_file(const QString &file_path, int stream_index,
                             bool replace) {
    auto result = import_file_internal(file_path, stream_index, replace);

    qDebug() << "import file result:" << result;

    switch (result) {
        case dsnote_app::file_import_result_t::ok_streams_selection:
        case dsnote_app::file_import_result_t::ok_import_audio:
        case dsnote_app::file_import_result_t::ok_import_subtitles:
        case dsnote_app::file_import_result_t::ok_import_text:
            break;
        case dsnote_app::file_import_result_t::error_requested_stream_not_found:
        case dsnote_app::file_import_result_t::error_no_supported_streams:
            reset_files_queue();
            emit error(error_t::ErrorImportFileNoStreams);
            break;
        case dsnote_app::file_import_result_t::
            error_import_audio_stt_not_configured:
            reset_files_queue();
            emit error(error_t::ErrorSttNotConfigured);
            break;
        case dsnote_app::file_import_result_t::error_import_subtitles_error:
        case dsnote_app::file_import_result_t::error_import_text_error:
            reset_files_queue();
            emit error(error_t::ErrorImportFileGeneral);
            break;
    }
}

void dsnote_app::handle_mc_state_changed() {
    update_service_state();
    update_task_state();

    if (m_mc.state() != media_converter::state_t::idle &&
        m_mc.state() != media_converter::state_t::error)
        return;

    if (!m_mc.cancelled()) {
        auto mc_error = m_mc.state() == media_converter::state_t::error;

        switch (m_mc.task()) {
            case media_converter::task_t::import_subtitles_async:
                if (mc_error) {
                    reset_files_queue();
                    emit error(error_t::ErrorImportFileGeneral);
                } else {
                    update_note(
                        m_mc.string_data(),
                        m_text_destination == text_destination_t::note_replace);

                    settings::instance()->set_stt_tts_text_format(
                        settings::text_format_t::TextFormatSubRip);
                    settings::instance()->set_mnt_text_format(
                        settings::text_format_t::TextFormatSubRip);

                    emit can_open_next_file();
                }
                break;
            case media_converter::task_t::export_to_audio_async:
            case media_converter::task_t::export_to_audio_mix_async:
                if (mc_error) {
                    emit error(error_t::ErrorExportFileGeneral);
                } else {
                    if (settings::instance()->mtag() &&
                        settings::audio_format_from_filename(
                            m_dest_file_info.output_path) !=
                            settings::audio_format_t::AudioFormatWav) {
                        mtag_tools::write(
                            /*path=*/m_dest_file_info.output_path.toStdString(),
                            /*mtag=*/{
                                /*title=*/m_dest_file_info.title_tag
                                    .toStdString(),
                                /*artist=*/
                                settings::instance()
                                    ->mtag_artist_name()
                                    .toStdString(),
                                /*album=*/
                                settings::instance()
                                    ->mtag_album_name()
                                    .toStdString(),
                                /*comment=*/{},
                                /*track=*/m_dest_file_info.track_tag.toInt()});
                    }
                    emit save_note_to_file_done();
                }
                break;
            case media_converter::task_t::export_to_subtitles_async:
                if (mc_error) {
                    emit error(error_t::ErrorExportFileGeneral);
                } else {
                    emit save_note_to_file_done();
                }
                break;
            case media_converter::task_t::none:
                break;
        }
    }

    m_mc.clear();
}

void dsnote_app::handle_mc_progress_changed() {
    qDebug() << "mc progress:" << m_mc.progress();
    emit mc_progress_changed();
}

QStringList dsnote_app::make_streams_names(
    const std::vector<media_compressor::stream_t> &streams) {
    QStringList names;
    std::transform(streams.cbegin(), streams.cend(), std::back_inserter(names),
                   [&](const auto &stream) {
                       QString name;

                       switch (stream.media_type) {
                           case media_compressor::media_type_t::audio:
                               name.append(tr("Audio"));
                               break;
                           case media_compressor::media_type_t::video:
                               name.append(tr("Video"));
                               break;
                           case media_compressor::media_type_t::subtitles:
                               name.append(tr("Subtitles"));
                               break;
                           case media_compressor::media_type_t::unknown:
                               throw std::runtime_error{"invalid media type"};
                       }

                       name.append(" · ");

                       if (stream.title.empty()) {
                           name.append(tr("Unnamed stream") + " " +
                                       QString::number(names.size() + 1));
                       } else {
                           name.append(QString::fromStdString(stream.title));
                       }

                       if (!stream.language.empty()) {
                           name.append(QStringLiteral(" / %1").arg(
                               QString::fromStdString(stream.language)));
                       }

                       name.append(QStringLiteral(" (%1)").arg(stream.index));

                       return name;
                   });
    return names;
}

dsnote_app::file_import_result_t dsnote_app::import_file_internal(
    const QString &file_path, int stream_index, bool replace) {
    qDebug() << "opening file:" << file_path << stream_index;

    auto media_info = media_compressor{}.media_info(file_path.toStdString());

    if (media_info) {
        qDebug() << QString::fromStdString([&]() {
            std::ostringstream os;
            os << *media_info;
            return os.str();
        }());

        if (media_info->audio_streams.empty() &&
            media_info->subtitles_streams.empty()) {
            qWarning() << "file does not contain audio or subtitles streams";
            return file_import_result_t::error_no_supported_streams;
        }

        if (stream_index < 0) {
            if ((!media_info->audio_streams.empty() &&
                 !media_info->subtitles_streams.empty()) ||
                media_info->audio_streams.size() > 1 ||
                media_info->subtitles_streams.size() > 1) {
                qDebug() << "file contains more than one stream";

                QStringList streams_names;
                streams_names.append(
                    make_streams_names(media_info->audio_streams));
                streams_names.append(
                    make_streams_names(media_info->subtitles_streams));

                emit import_file_multiple_streams(file_path, streams_names,
                                                  replace);

                return file_import_result_t::ok_streams_selection;
            }

            if (!media_info->audio_streams.empty())
                stream_index = media_info->audio_streams.front().index;
            else
                stream_index = media_info->subtitles_streams.front().index;
        }

        auto stream_by_id_it =
            std::find_if(media_info->audio_streams.cbegin(),
                         media_info->audio_streams.cend(),
                         [stream_index](const auto &stream) {
                             return stream.index == stream_index;
                         });
        if (stream_by_id_it == media_info->audio_streams.cend())
            stream_by_id_it =
                std::find_if(media_info->subtitles_streams.cbegin(),
                             media_info->subtitles_streams.cend(),
                             [stream_index](const auto &stream) {
                                 return stream.index == stream_index;
                             });

        if (stream_by_id_it == media_info->subtitles_streams.cend()) {
            qDebug() << "requested stream not found:" << stream_index;
            return file_import_result_t::error_requested_stream_not_found;
        }

        switch (stream_by_id_it->media_type) {
            case media_compressor::media_type_t::audio:
                if (!stt_configured()) {
                    qWarning()
                        << "can't transcribe because stt is not configured";
                    return file_import_result_t::
                        error_import_audio_stt_not_configured;
                }

                transcribe_file(file_path, stream_by_id_it->index, replace);
                return file_import_result_t::ok_import_audio;
            case media_compressor::media_type_t::subtitles:
                if (!m_mc.import_subtitles_async(file_path,
                                                 stream_by_id_it->index)) {
                    qCritical() << "can't extract subtitles";
                    return file_import_result_t::error_import_subtitles_error;
                }

                m_text_destination = replace ? text_destination_t::note_replace
                                             : text_destination_t::note_add;
                return file_import_result_t::ok_import_subtitles;
            case media_compressor::media_type_t::video:
            case media_compressor::media_type_t::unknown:
                break;
        }

        qCritical() << "invalid stream type";
        return file_import_result_t::error_no_supported_streams;
    }

    if (!import_text_file(file_path, replace)) {
        qWarning() << "can't open text file";
        return file_import_result_t::error_no_supported_streams;
    }

    settings::instance()->set_stt_tts_text_format(
        settings::text_format_t::TextFormatRaw);
    settings::instance()->set_mnt_text_format(
        settings::text_format_t::TextFormatRaw);

    return file_import_result_t::ok_import_text;
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

    auto result = import_file_internal(
        m_files_to_open.front(), -1,
        m_text_destination == text_destination_t::note_replace ? true : false);

    qDebug() << "import file result:" << result;

    switch (result) {
        case dsnote_app::file_import_result_t::ok_streams_selection:
            reset_files_queue();
            break;
        case dsnote_app::file_import_result_t::ok_import_audio:
        case dsnote_app::file_import_result_t::ok_import_subtitles:
        case dsnote_app::file_import_result_t::ok_import_text:
            m_files_to_open.pop();
            emit can_open_next_file();
            break;
        case dsnote_app::file_import_result_t::error_requested_stream_not_found:
        case dsnote_app::file_import_result_t::error_no_supported_streams:
            reset_files_queue();
            emit error(error_t::ErrorImportFileNoStreams);
            break;
        case dsnote_app::file_import_result_t::
            error_import_audio_stt_not_configured:
            reset_files_queue();
            emit error(error_t::ErrorSttNotConfigured);
            break;
        case dsnote_app::file_import_result_t::error_import_subtitles_error:
        case dsnote_app::file_import_result_t::error_import_text_error:
            reset_files_queue();
            emit error(error_t::ErrorImportFileGeneral);
            break;
    }
}

void dsnote_app::import_files(const QStringList &input_files, bool replace) {
    reset_files_queue();

    for (auto &file : input_files) m_files_to_open.push(file);

    if (!m_files_to_open.empty()) {
        if (m_files_to_open.size() == 1) m_files_to_open.push({});
        if (!note().isEmpty()) make_undo();
        qDebug() << "importing files:" << input_files;
        m_text_destination = replace ? text_destination_t::note_replace
                                     : text_destination_t::note_add;
        m_open_files_delay_timer.start();
    }
}

void dsnote_app::import_files_url(const QList<QUrl> &input_urls, bool replace) {
    QStringList input_files;

    std::transform(input_urls.cbegin(), input_urls.cend(),
                   std::back_inserter(input_files),
                   [](const auto &url) { return url.toLocalFile(); });

    import_files(input_files, replace);
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
    uint id, [[maybe_unused]] uint reason) {
    if (!m_desktop_notification || m_desktop_notification->id != id) return;
    m_desktop_notification.reset();
}

void dsnote_app::handle_desktop_notification_action_invoked(
    uint id, [[maybe_unused]] const QString &action_key) {
    if (!m_desktop_notification || m_desktop_notification->id != id) return;
    emit activate_requested();
    close_desktop_notification();
}

bool dsnote_app::feature_available(const QString &name,
                                   bool default_value) const {
    if (m_features_availability.contains(name))
        return m_features_availability.value(name).toList().at(0).toBool();
    return default_value;
}

bool dsnote_app::feature_whispercpp_stt() const {
    return feature_available("whispercpp-stt", false);
}

bool dsnote_app::feature_whispercpp_gpu() const {
    return feature_available("whispercpp-stt-cuda", false) ||
           feature_available("whispercpp-stt-hip", false) ||
           feature_available("whispercpp-stt-opencl", false) ||
           feature_available("whispercpp-stt-openvino", false) ||
           feature_available("whispercpp-stt-vulkan", false);
}

bool dsnote_app::feature_fasterwhisper_stt() const {
    return feature_available("faster-whisper-stt", false);
}

bool dsnote_app::feature_fasterwhisper_gpu() const {
    return feature_available("faster-whisper-stt-cuda", false) ||
           feature_available("faster-whisper-stt-hip", false);
}

bool dsnote_app::feature_coqui_tts() const {
    return feature_available("coqui-tts", false);
}

bool dsnote_app::feature_coqui_gpu() const {
    return feature_available("coqui-tts-cuda", false) ||
           feature_available("coqui-tts-hip", false);
}

bool dsnote_app::feature_whisperspeech_tts() const {
    return feature_available("whisperspeech-tts", false);
}

bool dsnote_app::feature_whisperspeech_gpu() const {
    return feature_available("whisperspeech-tts-cuda", false) ||
           feature_available("whisperspeech-tts-hip", false);
}

bool dsnote_app::feature_parler_tts() const {
    return feature_available("parler-tts", false);
}

bool dsnote_app::feature_parler_gpu() const {
    return feature_available("parler-tts-cuda", false) ||
           feature_available("parler-tts-hip", false);
}

bool dsnote_app::feature_f5_tts() const {
    return feature_available("f5-tts", false);
}

bool dsnote_app::feature_f5_gpu() const {
    return feature_available("f5-tts-cuda", false) ||
           feature_available("f5-tts-hip", false);
}

bool dsnote_app::feature_kokoro_tts() const {
    return feature_available("kokoro-tts", false);
}

bool dsnote_app::feature_kokoro_gpu() const {
    return feature_available("kokoro-tts-cuda", false) ||
           feature_available("kokoro-tts-hip", false);
}

bool dsnote_app::feature_punctuator() const {
    return feature_available("punctuator", false);
}

bool dsnote_app::feature_diacritizer_he() const {
    return feature_available("diacritizer-he", false);
}

bool dsnote_app::feature_translator() const {
    return feature_available("translator", true);
}

bool dsnote_app::feature_fake_keyboard() const {
    return feature_available("fake-keyboard", false);
}

bool dsnote_app::feature_fake_keyboard_ydo() const {
    return fake_keyboard::is_ydo_supported();
}

bool dsnote_app::feature_hotkeys() const {
    return feature_available("hotkeys", false);
}

bool dsnote_app::feature_hotkeys_portal() const {
    return m_gs_manager.is_portal_supported();
}

void dsnote_app::update_feature_statuses() {
    bool changed = false;

    // update fake-keyboard status
    if (m_features_availability.contains(QStringLiteral("fake-keyboard"))) {
        auto new_value = fake_keyboard::is_supported();
        auto current_value =
            m_features_availability.value(QStringLiteral("fake-keyboard"))
                .toList();
        if (current_value.isEmpty() ||
            new_value != current_value.front().toBool()) {
            m_features_availability.insert(
                QStringLiteral("fake-keyboard"),
                QVariantList{new_value, tr("Insert text to active window")});
            changed = true;
        }
#ifdef USE_DESKTOP
        m_tray.set_fake_keyboard_supported(new_value);
#endif
    }

    // update hotkeys status
    if (m_features_availability.contains(QStringLiteral("hotkeys"))) {
        auto new_value = m_gs_manager.is_supported();
        auto current_value =
            m_features_availability.value(QStringLiteral("hotkeys")).toList();
        if (current_value.isEmpty() ||
            new_value != current_value.front().toBool()) {
            m_features_availability.insert(
                QStringLiteral("hotkeys"),
                QVariantList{new_value, tr("Global keyboard shortcuts")});
            changed = true;
        }
    }

    if (changed) {
        emit features_changed();
    }
}

QVariantList dsnote_app::features_availability() {
    QVariantList list;

    auto old_features_availability = m_features_availability;

    if (settings::launch_mode == settings::launch_mode_t::app_stanalone) {
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

        // update hw devices and flags only in app mode
        settings::instance()->update_hw_devices_from_fa(
            m_features_availability);
        settings::instance()->update_system_flags_from_fa(
            m_features_availability);
        settings::instance()->update_addon_flags_from_fa(
            m_features_availability);
    }

#ifdef USE_DESKTOP
    if (!m_features_availability.isEmpty()) {
        m_features_availability.insert(
            "hotkeys", QVariantList{m_gs_manager.is_supported(),
                                    tr("Global keyboard shortcuts")});
        auto fake_keyboard_supported = fake_keyboard::is_supported();
        m_features_availability.insert(
            "fake-keyboard", QVariantList{fake_keyboard_supported,
                                          tr("Insert text to active window")});
        m_tray.set_fake_keyboard_supported(fake_keyboard_supported);
    }
#endif

    // update py version
    if (m_features_availability.contains("py-version")) {
        auto vl = m_features_availability.value("py-version").toList();
        if (!vl.isEmpty()) {
            auto new_py_version = vl.front().toString();
            if (new_py_version != m_py_version) {
                m_py_version = new_py_version;
                emit py_version_changed();
            }
        }
    }

    auto it = m_features_availability.cbegin();
    while (it != m_features_availability.cend()) {
        auto val = it.value().toList();
        if (val.size() > 1 && val.at(0).type() == QVariant::Bool &&
            val.at(1).type() == QVariant::String) {
            list.push_back(QVariantList{val.at(0), val.at(1)});
        }
        ++it;
    }

    if (old_features_availability != m_features_availability) {
        auto translator_enabled = feature_available("translator", false);
        if (!translator_enabled) {
            settings::instance()->set_translator_mode(false);
        }

        if (settings::launch_mode == settings::launch_mode_t::app) {
            models_manager::instance()->update_models_using_availability(
                {/*tts_coqui=*/feature_available("coqui-tts", false),
                 /*tts_mimic3=*/feature_available("mmic3-tts", false),
                 /*tts_mimic3_de=*/feature_available("mmic3-tts-de", false),
                 /*tts_mimic3_es=*/feature_available("mmic3-tts-es", false),
                 /*tts_mimic3_fr=*/feature_available("mmic3-tts-fr", false),
                 /*tts_mimic3_it=*/feature_available("mmic3-tts-it", false),
                 /*tts_mimic3_ru=*/feature_available("mmic3-tts-ru", false),
                 /*tts_mimic3_sw=*/feature_available("mmic3-tts-sw", false),
                 /*tts_mimic3_fa=*/feature_available("mmic3-tts-fa", false),
                 /*tts_mimic3_nl=*/feature_available("mmic3-tts-nl", false),
                 /*tts_rhvoice=*/feature_available("rhvoice-tts", false),
                 /*tts_whisperspeech=*/
                 feature_available("whisperspeech-tts", false),
                 /*tts_parler=*/
                 feature_available("parler-tts", false),
                 /*tts_f5=*/
                 feature_available("f5-tts", false),
                 /*tts_kokoro=*/
                 feature_available("kokoro-tts", false),
                 /*tts_kokoro_ja=*/
                 feature_available("kokoro-tts-ja", false),
                 /*tts_kokoro_zh=*/
                 feature_available("kokoro-tts-zh", false),
                 /*stt_fasterwhisper=*/
                 feature_available("faster-whisper-stt", false),
                 /*stt_ds=*/feature_available("coqui-stt", false),
                 /*stt_vosk=*/feature_available("vosk-stt", false),
                 /*stt_whisper=*/feature_available("whispercpp-stt", false),
                 /*mnt_bergamot=*/translator_enabled,
                 /*ttt_hftc=*/feature_available("punctuator", false),
                 /*option_r=*/feature_available("coqui-tts-ko", false)});
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

    execute_action(m_pending_action.value().first,
                   m_pending_action.value().second);

    m_pending_action.reset();
}

QVariantMap dsnote_app::execute_action_id(const QString &action_id,
                                          const QString &extra,
                                          bool trusted_source) {
    QVariantMap result;

    auto update_result = [&result](action_error_code_t error_code) {
        result.insert(QStringLiteral("error"), static_cast<int>(error_code));
    };

    if (action_id.isEmpty()) {
        update_result(action_error_code_t::unknown_name);
        return result;
    }

    if (!trusted_source && !settings::instance()->actions_api_enabled()) {
        qWarning() << "actions api is not enabled in the settings";
        update_result(action_error_code_t::not_enabled);
        return result;
    }

    if (false) {
#define X(name, str)                                             \
    }                                                            \
    else if (action_id.compare(str, Qt::CaseInsensitive) == 0) { \
        execute_action(action_t::name, extra);
        ACTION_TABLE
#undef X
    } else {
        qWarning() << "invalid action:" << action_id << extra;
        update_result(action_error_code_t::unknown_name);
        return result;
    }

    update_result(action_error_code_t::success);
    return result;
}

#ifdef USE_DESKTOP
void dsnote_app::execute_tray_action(tray_icon::action_t action, int value) {
    switch (action) {
        case tray_icon::action_t::start_listening:
            execute_action(action_t::start_listening, {});
            break;
        case tray_icon::action_t::start_listening_translate:
            execute_action(action_t::start_listening_translate, {});
            break;
        case tray_icon::action_t::start_listening_active_window:
            execute_action(action_t::start_listening_active_window, {});
            break;
        case tray_icon::action_t::start_listening_translate_active_window:
            execute_action(action_t::start_listening_translate_active_window,
                           {});
            break;
        case tray_icon::action_t::start_listening_clipboard:
            execute_action(action_t::start_listening_clipboard, {});
            break;
        case tray_icon::action_t::start_listening_translate_clipboard:
            execute_action(action_t::start_listening_translate_clipboard, {});
            break;
        case tray_icon::action_t::stop_listening:
            execute_action(action_t::stop_listening, {});
            break;
        case tray_icon::action_t::start_reading:
            execute_action(action_t::start_reading, {});
            break;
        case tray_icon::action_t::start_reading_clipboard:
            execute_action(action_t::start_reading_clipboard, {});
            break;
        case tray_icon::action_t::pause_resume_reading:
            execute_action(action_t::pause_resume_reading, {});
            break;
        case tray_icon::action_t::cancel:
            execute_action(action_t::cancel, {});
            break;
        case tray_icon::action_t::quit:
            QGuiApplication::quit();
            break;
        case tray_icon::action_t::toggle_app_window:
            if (m_app_window)
                m_app_window->setProperty(
                    "visible", !m_app_window->property("visible").toBool());
            break;
        case tray_icon::action_t::change_stt_model:
            set_active_stt_model_idx(value);
            break;
        case tray_icon::action_t::change_tts_model:
            set_active_tts_model_idx(value);
            break;
    }
}
#endif

dsnote_app::action_t dsnote_app::convert_action(action_t action) const {
    if (!settings::instance()->use_toggle_for_hotkey()) {
        // no conversion if toggle is disabled
        return action;
    }

    switch (action) {
        case dsnote_app::action_t::start_listening:
        case dsnote_app::action_t::start_listening_translate:
        case dsnote_app::action_t::start_listening_active_window:
        case dsnote_app::action_t::start_listening_translate_active_window:
        case dsnote_app::action_t::start_listening_clipboard:
        case dsnote_app::action_t::start_listening_translate_clipboard:
            if (service_state() ==
                    service_state_t::StateListeningSingleSentence ||
                service_state() == service_state_t::StateListeningManual ||
                service_state() == service_state_t::StateListeningAuto) {
                action = action_t::stop_listening;
            }
            break;
        case dsnote_app::action_t::start_reading:
        case dsnote_app::action_t::start_reading_clipboard:
        case dsnote_app::action_t::start_reading_text: {
            if (service_state() == service_state_t::StatePlayingSpeech) {
                action = action_t::cancel;
            }
            break;
        }
        case dsnote_app::action_t::stop_listening:
        case dsnote_app::action_t::pause_resume_reading:
        case dsnote_app::action_t::cancel:
        case dsnote_app::action_t::switch_to_next_stt_model:
        case dsnote_app::action_t::switch_to_prev_stt_model:
        case dsnote_app::action_t::switch_to_next_tts_model:
        case dsnote_app::action_t::switch_to_prev_tts_model:
        case dsnote_app::action_t::set_stt_model:
        case dsnote_app::action_t::set_tts_model:
            break;
    }

    return action;
}

void dsnote_app::execute_action(action_t action, const QString &extra) {
    if (busy()) {
        m_pending_action = std::make_pair(action, extra);
        m_action_delay_timer.start();
        qDebug() << "delaying action:" << action;
        return;
    }

    qDebug() << "executing action:" << action
             << "extra =" << extra.trimmed().left(10);

    auto caction = convert_action(action);

    if (caction != action) {
        qDebug() << "converting action:" << action << "=>" << caction;
    }

    switch (caction) {
        case dsnote_app::action_t::start_listening:
            listen();
            break;
        case dsnote_app::action_t::start_listening_translate:
            listen_translate();
            break;
        case dsnote_app::action_t::start_listening_active_window:
            listen_to_active_window();
            break;
        case dsnote_app::action_t::start_listening_translate_active_window:
            listen_translate_to_active_window();
            break;
        case dsnote_app::action_t::start_listening_clipboard:
            listen_to_clipboard();
            break;
        case dsnote_app::action_t::start_listening_translate_clipboard:
            listen_translate_to_clipboard();
            break;
        case dsnote_app::action_t::stop_listening:
            stop_listen();
            break;
        case dsnote_app::action_t::start_reading:
            play_speech();
            break;
        case dsnote_app::action_t::start_reading_clipboard:
        case dsnote_app::action_t::start_reading_text: {
            auto segment =
                text_tools::raw_taged_segment_from_text(extra.toStdString());
            play_speech_from_text(
                caction == action_t::start_reading_clipboard
                    ? QGuiApplication::clipboard()->text()
                    : QString::fromStdString(segment.text),
                segment.tags.empty() ||
                        !text_tools::valid_model_id(segment.tags.front())
                    ? active_tts_model()
                    : QString::fromStdString(segment.tags.front()));
            break;
        }
        case dsnote_app::action_t::pause_resume_reading:
            if (task_state() == service_task_state_t::TaskStateSpeechPaused)
                resume_speech();
            else
                pause_speech();
            break;
        case dsnote_app::action_t::cancel:
            cancel();
            break;
        case dsnote_app::action_t::switch_to_next_stt_model:
            set_active_next_stt_model();
            break;
        case dsnote_app::action_t::switch_to_prev_stt_model:
            set_active_prev_stt_model();
            break;
        case dsnote_app::action_t::switch_to_next_tts_model:
            set_active_next_tts_model();
            break;
        case dsnote_app::action_t::switch_to_prev_tts_model:
            set_active_prev_tts_model();
            break;
        case dsnote_app::action_t::set_stt_model:
            if (!extra.isEmpty()) set_active_stt_model_or_lang(extra);
            break;
        case dsnote_app::action_t::set_tts_model:
            if (!extra.isEmpty()) set_active_tts_model_or_lang(extra);
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

static bool has_model_options(const QVariantMap &map, const QString &id,
                              char option) {
    if (map.contains(id)) {
        auto model = map.value(id).toStringList();
        return model.size() > 2 && model.at(2).contains(option);
    }

    return false;
}

bool dsnote_app::stt_translate_needed_by_id(const QString &id) const {
    return has_model_options(m_available_stt_models_map, id, 't');
}

bool dsnote_app::tts_ref_voice_needed_by_id(const QString &id) const {
    return has_model_options(m_available_tts_models_map, id, 'x');
}

bool dsnote_app::tts_ref_prompt_needed_by_id(const QString &id) const {
    return has_model_options(m_available_tts_models_map, id, 'v');
}

bool dsnote_app::stt_translate_needed() const {
    return stt_translate_needed_by_id(m_active_stt_model);
}

bool dsnote_app::tts_ref_voice_needed() const {
    return tts_ref_voice_needed_by_id(m_active_tts_model);
}

bool dsnote_app::tts_ref_prompt_needed() const {
    return tts_ref_prompt_needed_by_id(m_active_tts_model);
}

bool dsnote_app::tts_for_in_mnt_ref_voice_needed() const {
    return tts_ref_voice_needed_by_id(m_active_tts_model_for_in_mnt);
}

bool dsnote_app::tts_for_out_mnt_ref_voice_needed() const {
    return tts_ref_voice_needed_by_id(m_active_tts_model_for_out_mnt);
}

bool dsnote_app::tts_for_out_mnt_ref_prompt_needed() const {
    return tts_ref_prompt_needed_by_id(m_active_tts_model_for_out_mnt);
}

bool dsnote_app::tts_for_in_mnt_ref_prompt_needed() const {
    return tts_ref_prompt_needed_by_id(m_active_tts_model_for_in_mnt);
}

QString dsnote_app::cache_dir() {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString dsnote_app::import_ref_voice_file_path() {
    return QDir{cache_dir()}.absoluteFilePath(s_imported_ref_file_name);
}

void dsnote_app::player_stop_voice_ref() {
    if (!m_player) return;

    m_player->setMedia({});

    m_player_current_voice_ref_idx = -1;
    emit player_current_voice_ref_idx_changed();
}

void dsnote_app::player_play_voice_ref_idx(int idx) {
    auto item = std::next(m_available_tts_ref_voices_map.cbegin(), idx).value();

    auto list = item.toStringList();
    if (list.size() < 2) return;

    auto wav_file = QDir{cache_dir()}.absoluteFilePath(
        QFileInfo{list.at(1)}.fileName() + "-refvoice.wav");

    try {
        media_compressor{}.decompress_to_file({list.at(1).toStdString()},
                                              wav_file.toStdString(), {});
    } catch (const std::runtime_error &err) {
        qCritical() << err.what();
        return;
    }

    player_set_path(wav_file);

    m_player->play();

    m_player_current_voice_ref_idx = idx;
    emit player_current_voice_ref_idx_changed();
}

void dsnote_app::player_import_from_url(const QUrl &url) {
    player_import_from_rec_path(url.toLocalFile());
}

void dsnote_app::player_import_from_rec_path(const QString &path) {
    m_recorder = std::make_unique<recorder>(path, import_ref_voice_file_path());

    connect(
        m_recorder.get(), &recorder::processing_changed, this,
        [this, path]() {
            if (m_recorder && !m_recorder->processing()) {
                auto mtag = mtag_tools::read(path.toStdString());
                if (mtag && !mtag->title.empty())
                    emit recorder_new_stream_name(
                        QString::fromStdString(mtag->title));
                else
                    emit recorder_new_stream_name(QFileInfo{path}.baseName());

                player_import_rec();
            }

            emit recorder_processing_changed();
        },
        Qt::QueuedConnection);

    m_recorder->process();
}

void dsnote_app::player_import_rec() {
    player_set_path(import_ref_voice_file_path());

    if (m_recorder) {
        QVariantList vprobs;
        auto probs = m_recorder->probs();
        std::transform(probs.cbegin(), probs.cend(), std::back_inserter(vprobs),
                       [](auto prob) { return QVariant::fromValue(prob); });

        emit recorder_new_probs(vprobs);
    }
}

void dsnote_app::player_set_path(const QString &wav_file_path) {
    if (!m_player) create_player();

    m_player->setMedia(
        QUrl{QStringLiteral("gst-pipeline: filesrc location=%1 ! wavparse ! "
                            "audioconvert ! alsasink")
                 .arg(wav_file_path)});
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
                                         const QString &name,
                                         const QString &text) {
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
        media_compressor::options_t opts{
            media_compressor::quality_t::vbr_high,
            media_compressor::flags_t::flag_none,
            1.0,
            {},
            /*clip_info=*/
            media_compressor::clip_info_t{static_cast<uint64_t>(start),
                                          static_cast<uint64_t>(stop), 0, 0}};

        media_compressor{}.compress_to_file(
            {wav_file_path.toStdString()}, out_file_path.toStdString(),
            media_compressor::format_t::audio_ogg_opus, opts);
    } catch (const std::runtime_error &error) {
        qWarning() << "can't compress file:" << error.what();
        return;
    }

    mtag_tools::mtag_t mtag;
    mtag.title =
        tts_ref_voice_unique_name(
            name.isEmpty() ? QFileInfo{out_file_path}.baseName() : name, false)
            .toStdString();
    mtag.comment = text.toStdString();
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

void dsnote_app::create_mic_recorder() {
    m_recorder = std::make_unique<recorder>(import_ref_voice_file_path());

    connect(
        m_recorder.get(), &recorder::recording_changed, this,
        [this]() {
            emit recorder_recording_changed();
        },
        Qt::QueuedConnection);
    connect(
        m_recorder.get(), &recorder::duration_changed, this,
        [this]() { emit recorder_duration_changed(); }, Qt::QueuedConnection);
    connect(
        m_recorder.get(), &recorder::processing_changed, this,
        [this]() {
            if (m_recorder && !m_recorder->processing()) {
                emit recorder_new_stream_name(tts_ref_voice_auto_name());
                player_import_rec();
            }
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
    create_mic_recorder();

    m_recorder->start();
}

void dsnote_app::recorder_stop() {
    if (!m_recorder) return;

    m_recorder->stop();
}

void dsnote_app::recorder_reset() { m_recorder.reset(); }

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
        case service_state_t::StateBusy:
        case service_state_t::StateImporting:
        case service_state_t::StateExporting:
        case service_state_t::StateRepairingText:
            m_tray.set_state(tray_icon::state_t::busy);
            break;
        case service_state_t::StateIdle:
        case service_state_t::StateNotConfigured:
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
        case service_task_state_t::TaskStateCancelling:
            m_tray.set_task_state(tray_icon::task_state_t::cancelling);
            break;
    }
#endif
}

void dsnote_app::update_auto_text_format_delayed() {
    m_auto_format_delay_timer.start();
}

void dsnote_app::update_auto_text_format() {
    auto format = text_tools::subrip_text_start(note().toStdString(), 10)
                      ? auto_text_format_t::AutoTextFormatSubRip
                      : auto_text_format_t::AutoTextFormatRaw;

    if (m_auto_text_format != format) {
        qDebug() << "auto text format:" << m_auto_text_format << "=>" << format;
        m_auto_text_format = format;
        emit auto_text_format_changed();
    }
}

void dsnote_app::set_app_window(QObject *app_window) {
    m_app_window = app_window;
}

QString dsnote_app::special_key_name(int key) const {
    if (key < 0x01000000) {
        // not special key
        return {};
    }
    return QKeySequence(key).toString();
}

void dsnote_app::switch_translated_text() {
    switch_mnt_langs();

    if (m_translated_text.isEmpty()) {
        if (settings::instance()->translate_when_typing()) translate_delayed();
    } else {
        update_note(m_translated_text, true);
        translate_delayed();
    }
}

void dsnote_app::update_audio_sources() {
    m_audio_sources.clear();
    m_audio_sources.push_back(tr("Auto"));

    auto dm_sources = m_audio_dm.sources();
    std::transform(dm_sources.cbegin(), dm_sources.cend(),
                   std::back_inserter(m_audio_sources),
                   [](const auto &dm_source) {
                       return QString::fromStdString(dm_source.description);
                   });

    emit audio_sources_changed();
}

QStringList dsnote_app::audio_sources() const { return m_audio_sources; }

QString dsnote_app::audio_source() {
    auto name = settings::instance()->audio_input_device();

    auto dev = m_audio_dm.source_by_name(name.toStdString());

    if (!dev) return {};

    return QString::fromStdString(dev->description);
}

void dsnote_app::set_audio_source(QString value) {
    if (value == tr("Auto")) value.clear();

    if (!value.isEmpty()) {
        auto dev = m_audio_dm.source_by_description(value.toStdString());
        value = dev ? QString::fromStdString(dev->name) : QString{};
    }

    settings::instance()->set_audio_input_device(value);
}

int dsnote_app::audio_source_idx() {
    auto dev = m_audio_dm.source_by_name(
        settings::instance()->audio_input_device().toStdString());

    if (!dev) return 0;

    auto it = std::find(m_audio_sources.cbegin(), m_audio_sources.cend(),
                        QString::fromStdString(dev->description));
    if (it == m_audio_sources.cend()) return 0;

    return std::distance(m_audio_sources.cbegin(), it);
}

void dsnote_app::set_audio_source_idx(int value) {
    if (value < 0 || value >= m_audio_sources.size()) return;
    set_audio_source(value == 0 ? "" : m_audio_sources.at(value));
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

void dsnote_app::set_last_cursor_position(int position) {
    if (m_last_cursor_position != position) {
        m_last_cursor_position = position;
        emit last_cursor_position_changed();
    }
}

void dsnote_app::update_stt_auto_lang(QString lang_id) {
    if (!active_stt_model().startsWith("auto_")) lang_id.clear();

    if (m_stt_auto_lang_id == lang_id) return;

    qDebug() << "stt auto lang changed:" << m_stt_auto_lang_id << "=>"
             << lang_id;
    m_stt_auto_lang_id = lang_id;
    emit stt_auto_lang_changed();
}

QString dsnote_app::stt_auto_lang_name() const {
    static const std::unordered_map<QString, QString> langs = {
        {QStringLiteral("en"), tr("English")},
        {QStringLiteral("zh"), tr("Chinese")},
        {QStringLiteral("de"), tr("German")},
        {QStringLiteral("es"), tr("Spanish")},
        {QStringLiteral("ru"), tr("Russian")},
        {QStringLiteral("ko"), tr("Korean")},
        {QStringLiteral("fr"), tr("French")},
        {QStringLiteral("ja"), tr("Japanese")},
        {QStringLiteral("pt"), tr("Portuguese")},
        {QStringLiteral("tr"), tr("Turkish")},
        {QStringLiteral("pl"), tr("Polish")},
        {QStringLiteral("ca"), tr("Catalan")},
        {QStringLiteral("nl"), tr("Dutch")},
        {QStringLiteral("ar"), tr("Arabic")},
        {QStringLiteral("sv"), tr("Swedish")},
        {QStringLiteral("it"), tr("Italian")},
        {QStringLiteral("id"), tr("Indonesian")},
        {QStringLiteral("hi"), tr("Hindi")},
        {QStringLiteral("fi"), tr("Finnish")},
        {QStringLiteral("vi"), tr("Vietnamese")},
        {QStringLiteral("he"), tr("Hebrew")},
        {QStringLiteral("uk"), tr("Ukrainian")},
        {QStringLiteral("el"), tr("Greek")},
        {QStringLiteral("ms"), tr("Malay")},
        {QStringLiteral("cs"), tr("Czech")},
        {QStringLiteral("ro"), tr("Romanian")},
        {QStringLiteral("da"), tr("Danish")},
        {QStringLiteral("hu"), tr("Hungarian")},
        {QStringLiteral("ta"), tr("Tamil")},
        {QStringLiteral("no"), tr("Norwegian")},
        {QStringLiteral("th"), tr("Thai")},
        {QStringLiteral("ur"), tr("Urdu")},
        {QStringLiteral("hr"), tr("Croatian")},
        {QStringLiteral("bg"), tr("Bulgarian")},
        {QStringLiteral("lt"), tr("Lithuanian")},
        {QStringLiteral("la"), tr("Latin")},
        {QStringLiteral("mi"), tr("Maori")},
        {QStringLiteral("ml"), tr("Malayalam")},
        {QStringLiteral("cy"), tr("Welsh")},
        {QStringLiteral("sk"), tr("Slovak")},
        {QStringLiteral("te"), tr("Telugu")},
        {QStringLiteral("fa"), tr("Persian")},
        {QStringLiteral("lv"), tr("Latvian")},
        {QStringLiteral("bn"), tr("Bengali")},
        {QStringLiteral("sr"), tr("Serbian")},
        {QStringLiteral("az"), tr("Azerbaijani")},
        {QStringLiteral("sl"), tr("Slovenian")},
        {QStringLiteral("kn"), tr("Kannada")},
        {QStringLiteral("et"), tr("Estonian")},
        {QStringLiteral("mk"), tr("Macedonian")},
        {QStringLiteral("br"), tr("Breton")},
        {QStringLiteral("eu"), tr("Basque")},
        {QStringLiteral("is"), tr("Icelandic")},
        {QStringLiteral("hy"), tr("Armenian")},
        {QStringLiteral("ne"), tr("Nepali")},
        {QStringLiteral("mn"), tr("Mongolian")},
        {QStringLiteral("bs"), tr("Bosnian")},
        {QStringLiteral("kk"), tr("Kazakh")},
        {QStringLiteral("sq"), tr("Albanian")},
        {QStringLiteral("sw"), tr("Swahili")},
        {QStringLiteral("gl"), tr("Galician")},
        {QStringLiteral("mr"), tr("Marathi")},
        {QStringLiteral("pa"), tr("Punjabi")},
        {QStringLiteral("si"), tr("Sinhala")},
        {QStringLiteral("km"), tr("Khmer")},
        {QStringLiteral("sn"), tr("Shona")},
        {QStringLiteral("yo"), tr("Yoruba")},
        {QStringLiteral("so"), tr("Somali")},
        {QStringLiteral("af"), tr("Afrikaans")},
        {QStringLiteral("oc"), tr("Occitan")},
        {QStringLiteral("ka"), tr("Georgian")},
        {QStringLiteral("be"), tr("Belarusian")},
        {QStringLiteral("tg"), tr("Tajik")},
        {QStringLiteral("sd"), tr("Sindhi")},
        {QStringLiteral("gu"), tr("Gujarati")},
        {QStringLiteral("am"), tr("Amharic")},
        {QStringLiteral("yi"), tr("Yiddish")},
        {QStringLiteral("lo"), tr("Lao")},
        {QStringLiteral("uz"), tr("Uzbek")},
        {QStringLiteral("fo"), tr("Faroese")},
        {QStringLiteral("ht"), tr("Haitian creole")},
        {QStringLiteral("ps"), tr("Pashto")},
        {QStringLiteral("tk"), tr("Turkmen")},
        {QStringLiteral("nn"), tr("Nynorsk")},
        {QStringLiteral("mt"), tr("Maltese")},
        {QStringLiteral("sa"), tr("Sanskrit")},
        {QStringLiteral("lb"), tr("Luxembourgish")},
        {QStringLiteral("my"), tr("Myanmar")},
        {QStringLiteral("bo"), tr("Tibetan")},
        {QStringLiteral("tl"), tr("Tagalog")},
        {QStringLiteral("mg"), tr("Malagasy")},
        {QStringLiteral("as"), tr("Assamese")},
        {QStringLiteral("tt"), tr("Tatar")},
        {QStringLiteral("haw"), tr("Hawaiian")},
        {QStringLiteral("ln"), tr("Lingala")},
        {QStringLiteral("ha"), tr("Hausa")},
        {QStringLiteral("ba"), tr("Bashkir")},
        {QStringLiteral("jw"), tr("Javanese")},
        {QStringLiteral("su"), tr("Sundanese")},
        {QStringLiteral("yue"), tr("Cantonese")}};

    auto it = langs.find(m_stt_auto_lang_id);
    if (it == langs.end()) return {};
    return it->second;
}

void dsnote_app::open_hotkeys_editor() {
#ifdef USE_DESKTOP
    m_gs_manager.reset_portal_connection();
#endif
}

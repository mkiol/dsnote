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
#include <QFile>
#include <QGuiApplication>
#include <QTextStream>
#include <algorithm>

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
    }

    return d;
}

dsnote_app::dsnote_app(QObject *parent)
    : QObject{parent},
      m_dbus_service{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                     QDBusConnection::sessionBus()} {
    qDebug() << "starting app:" << settings::instance()->launch_mode();

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
    m_translator_delay_timer.setTimerType(Qt::VeryCoarseTimer);
    m_translator_delay_timer.setInterval(500);
    connect(&m_translator_delay_timer, &QTimer::timeout, this,
            &dsnote_app::handle_translate_delayed, Qt::QueuedConnection);

    m_open_files_delay_timer.setSingleShot(true);
    m_open_files_delay_timer.setTimerType(Qt::VeryCoarseTimer);
    m_open_files_delay_timer.setInterval(250);
    connect(&m_open_files_delay_timer, &QTimer::timeout, this,
            &dsnote_app::open_next_file, Qt::QueuedConnection);

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        connect_service_signals();
        update_service_state();
    } else {
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

QString dsnote_app::insert_to_note(QString note, QString new_text,
                                   settings::insert_mode_t mode) {
    if (new_text.isEmpty()) return note;

    QTextStream ss{&note, QIODevice::WriteOnly};

    switch (mode) {
        case settings::insert_mode_t::InsertInLine:
            if (!note.isEmpty()) {
                auto last_char = note.at(note.size() - 1);
                if (last_char.isLetterOrNumber())
                    ss << ". ";
                else if (!last_char.isSpace())
                    ss << ' ';
            }
            break;
        case settings::insert_mode_t::InsertNewLine:
            if (!note.isEmpty()) {
                auto last_char = note.at(note.size() - 1);
                if (last_char.isLetterOrNumber())
                    ss << ".\n";
                else
                    ss << '\n';
            }
            break;
    }

    new_text[0] = new_text[0].toUpper();

    ss << new_text;

    if (new_text.at(new_text.size() - 1).isLetterOrNumber()) ss << '.';

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

    make_undo();

    set_note(insert_to_note(settings::instance()->note(), text,
                            settings::instance()->insert_mode()));

    this->m_intermediate_text.clear();

    emit text_changed();
    emit intermediate_text_changed();
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

        emit service_state_changed();

        update_configured_state();
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
        if (settings::instance()->mtag()) {
            mtag_tools::write(
                /*path=*/m_dest_file.toStdString(),
                /*title=*/m_dest_file_title_tag.toStdString(),
                /*artist=*/
                settings::instance()->mtag_artist_name().toStdString(),
                /*album=*/
                settings::instance()->mtag_album_name().toStdString(),
                /*track=*/m_dest_file_track_tag.toInt());
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
        const auto &id = std::next(m_available_stt_models_map.cbegin(), idx).key();

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
        const auto &id =
            std::next(m_available_tts_models_map.cbegin(), idx).key();

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech_service::instance()->set_default_tts_model(id);
        } else {
            qDebug() << "[app => dbus] set DefaultTtsModel:" << idx << id;
            m_dbus_service.setDefaultTtsModel(id);
        }
    }
}

void dsnote_app::set_active_tts_model_for_in_mnt_idx(int idx) {
    if (m_active_mnt_lang.isEmpty()) {
        qWarning() << "invalid active mnt lang";
        return;
    }

    if (active_tts_model_for_in_mnt_idx() != idx && idx > -1 &&
        idx < m_available_tts_models_for_in_mnt_map.size()) {
        const auto &id =
            std::next(m_available_tts_models_for_in_mnt_map.cbegin(), idx)
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
        const auto &id =
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
        const auto &id =
            std::next(m_available_mnt_langs_map.cbegin(), idx).key();

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
        const auto &id =
            std::next(m_available_mnt_out_langs_map.cbegin(), idx).key();

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

void dsnote_app::transcribe_file(const QString &source_file) {
    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_transcribe_file(source_file,
                                                                   {}, {});
    } else {
        qDebug() << "[app => dbus] call SttTranscribeFile";

        new_task = m_dbus_service.SttTranscribeFile(source_file, {}, {});
    }

    m_side_task.set(new_task);
}

void dsnote_app::transcribe_file(const QUrl &source_file) {
    transcribe_file(source_file.toLocalFile());
}

void dsnote_app::listen() {
    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_start_listen(
            static_cast<speech_service::speech_mode_t>(s->speech_mode()), {},
            {});
    } else {
        qDebug() << "[app => dbus] call SttStartListen:" << s->speech_mode();
        new_task = m_dbus_service.SttStartListen(
            static_cast<int>(s->speech_mode()), {}, {});
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
                                      const QString &model_id) {
    if (text.isEmpty()) {
        qWarning() << "text is empty";
        return;
    }

    int new_task = 0;

    QVariantMap options;
    options.insert("speech_speed",
                   static_cast<int>(settings::instance()->speech_speed()));

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

void dsnote_app::play_speech() { play_speech_internal(note(), {}); }

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
                                      : m_active_tts_model_for_in_mnt);
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

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            new_task = speech_service::instance()->mnt_translate(
                note(), m_active_mnt_lang, m_active_mnt_out_lang);
        } else {
            qDebug() << "[app => dbus] call MntTranslate";

            new_task = m_dbus_service.MntTranslate(note(), m_active_mnt_lang,
                                                   m_active_mnt_out_lang);
        }

        m_primary_task.set(new_task);
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::speech_to_file(const QUrl &dest_file, const QString &title_tag,
                                const QString &track_tag) {
    speech_to_file(dest_file.toLocalFile(), title_tag, track_tag);
}

void dsnote_app::speech_to_file(const QString &dest_file,
                                const QString &title_tag,
                                const QString &track_tag) {
    speech_to_file_internal(note(), {}, dest_file, title_tag, track_tag);
}

void dsnote_app::speech_to_file_translator(bool transtalated,
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

    speech_to_file_internal(transtalated ? m_translated_text : note(),
                            transtalated ? m_active_tts_model_for_out_mnt
                                         : m_active_tts_model_for_in_mnt,
                            dest_file, title_tag, track_tag);
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
                                         const QString &track_tag) {
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
    options.insert("speech_speed",
                   static_cast<int>(settings::instance()->speech_speed()));

    auto audio_format_str = settings::audio_format_str_from_filename(dest_file);

    options.insert("audio_format", audio_format_str);
    options.insert("audio_quality",
                   audio_quality_to_str(settings::instance()->audio_quality()));

    if (QFileInfo{dest_file}.suffix().toLower() != audio_format_str) {
        qDebug() << "file name doesn't have proper extension for audio format";
        if (dest_file.endsWith('.'))
            m_dest_file = dest_file + audio_format_str;
        else
            m_dest_file = dest_file + '.' + audio_format_str;
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
           another_app_connected();
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
        set_note(insert_to_note(settings::instance()->note(), text,
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
    } else {
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
                                settings::instance()->insert_mode()));
    }

    return true;
}

void dsnote_app::open_next_file() {
    if (m_files_to_open.empty() || busy()) return;

    qDebug() << "opening file:" << m_files_to_open.front();

    if (media_compressor{}.is_media_file(
            m_files_to_open.front().toStdString())) {
        if (stt_configured())
            transcribe_file(m_files_to_open.front());
        else {
            qWarning() << "can't transcribe because stt is not configured";
            reset_files_queue();
            return;
        }
        m_files_to_open.pop();
    } else {
        if (!load_note_from_file(m_files_to_open.front(), false)) {
            reset_files_queue();
            return;
        }

        m_files_to_open.pop();

        emit can_open_next_file();
    }
}

void dsnote_app::open_files(const QStringList &input_files) {
    reset_files_queue();

    for (auto &file : input_files) m_files_to_open.push(file);

    if (!m_files_to_open.empty()) {
        if (!note().isEmpty()) make_undo();
        qDebug() << "opening files:" << input_files;
        m_open_files_delay_timer.start();
    }
}

void dsnote_app::reset_files_queue() {
    if (!m_files_to_open.empty()) m_files_to_open = std::queue<QString>{};
}

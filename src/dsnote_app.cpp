/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dsnote_app.h"

#include <QClipboard>
#include <QDBusConnection>
#include <QGuiApplication>
#include <QTextStream>
#include <algorithm>

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
        case dsnote_app::service_state_t::StateUnknown:
            d << "unknown";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::service_speech_state_t type) {
    switch (type) {
        case dsnote_app::service_speech_state_t::SpeechStateNoSpeech:
            d << "no-speech";
            break;
        case dsnote_app::service_speech_state_t::
            SpeechStateSpeechDecodingEncoding:
            d << "decoding-encoding";
            break;
        case dsnote_app::service_speech_state_t::SpeechStateSpeechInitializing:
            d << "initializing";
            break;
        case dsnote_app::service_speech_state_t::SpeechStateSpeechDetected:
            d << "speech-detected";
            break;
        case dsnote_app::service_speech_state_t::SpeechStateSpeechPlaying:
            d << "speech-playing";
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
        case dsnote_app::error_t::ErrorNoService:
            d << "no-service-error";
            break;
    }

    return d;
}

dsnote_app::dsnote_app(QObject *parent)
    : QObject{parent},
      m_dbus_service{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                     QDBusConnection::sessionBus()} {
    qDebug() << "starting app:" << settings::instance()->launch_mode();

    connect(settings::instance(), &settings::speech_mode_changed, this,
            &dsnote_app::update_listen);
    connect(settings::instance(), &settings::mode_changed, this,
            &dsnote_app::update_listen);
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
            update_current_task();
            update_speech();
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
#ifdef USE_SFOS
    if (settings::instance()->mode() == settings::mode_t::Stt &&
        settings::instance()->speech_mode() ==
            settings::speech_mode_t::SpeechAutomatic) {
        listen();
    } else {
        cancel();
    }
#else
    cancel();
#endif
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

    settings::instance()->set_note(
        insert_to_note(settings::instance()->note(), text,
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

    this->m_intermediate_text = text;

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
            speech_service::instance(), &speech_service::speech_changed, this,
            [this] {
                handle_speech_changed(
                    static_cast<int>(speech_service::instance()->speech()));
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
        connect(&m_dbus_service, &OrgMkiolSpeechInterface::StatePropertyChanged,
                this, &dsnote_app::handle_state_changed);
        connect(&m_dbus_service,
                &OrgMkiolSpeechInterface::SpeechPropertyChanged, this,
                &dsnote_app::handle_speech_changed);
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

void dsnote_app::handle_speech_changed(int speech) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal SpeechPropertyChanged:" << speech;
    }

    if (m_primary_task != m_current_task && m_side_task != m_current_task) {
        qWarning() << "ignore SpeechPropertyChanged signal";
        return;
    }

    auto new_speech_state = [speech] {
        switch (speech) {
            case 0:
                break;
            case 1:
                return service_speech_state_t::SpeechStateSpeechDetected;
            case 2:
                return service_speech_state_t::
                    SpeechStateSpeechDecodingEncoding;
            case 3:
                return service_speech_state_t::SpeechStateSpeechInitializing;
            case 4:
                return service_speech_state_t::SpeechStateSpeechPlaying;
        }
        return service_speech_state_t::SpeechStateNoSpeech;
    }();

    if (m_speech_state != new_speech_state) {
        qDebug() << "app speech:" << m_speech_state << "=>" << new_speech_state;
        m_speech_state = new_speech_state;
        emit speech_changed();
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
    update_speech();
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

    if (QFile::copy(file, m_dest_file)) emit speech_to_file_done();
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

void dsnote_app::handle_tts_default_model_changed(const QString &model) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal TtsDefaultModelPropertyChanged:"
                 << model;
    }

    set_active_tts_model(model);
}

void dsnote_app::set_active_tts_model(const QString &model) {
    if (m_active_tts_model != model) {
        qDebug() << "app active tts model:" << m_active_tts_model << "=>"
                 << model;
        m_active_tts_model = model;
        emit active_tts_model_changed();
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

        if (old_busy != busy()) emit busy_changed();
        if (old_connected != connected()) emit connected_changed();

        update_configured_state();
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

void dsnote_app::cancel() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        if (m_side_task) {
            speech_service::instance()->cancel(m_side_task.current);
            m_side_task.reset();
        } else if (m_primary_task) {
            speech_service::instance()->cancel(m_primary_task.current);
            m_primary_task.reset();
        } else {
            speech_service::instance()->cancel(m_primary_task.previous);
        }
    } else {
        qDebug() << "[app => dbus] call Cancel";

        if (m_side_task) {
            m_dbus_service.Cancel(m_side_task.current);
            m_side_task.reset();
        } else if (m_primary_task) {
            m_dbus_service.Cancel(m_primary_task.current);
            m_primary_task.reset();
        } else {
            m_dbus_service.Cancel(m_primary_task.previous);
        }
    }

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::transcribe_file(const QString &source_file) {
    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->stt_transcribe_file(
            source_file, {}, s->translate());
    } else {
        qDebug() << "[app => dbus] call SttTranscribeFile";

        new_task =
            m_dbus_service.SttTranscribeFile(source_file, {}, s->translate());
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
            s->translate());
    } else {
        qDebug() << "[app => dbus] call SttStartListen:" << s->speech_mode();
        new_task = m_dbus_service.SttStartListen(
            static_cast<int>(s->speech_mode()), {}, s->translate());
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

void dsnote_app::play_speech() {
    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->tts_play_speech(
            settings::instance()->note(), {});
    } else {
        qDebug() << "[app => dbus] call TtsPlaySpeech";

        new_task =
            m_dbus_service.TtsPlaySpeech(settings::instance()->note(), {});
    }

    m_primary_task.set(new_task);

    this->m_intermediate_text.clear();
    emit intermediate_text_changed();
}

void dsnote_app::speech_to_file(const QUrl &dest_file) {
    speech_to_file(dest_file.toLocalFile());
}

void dsnote_app::speech_to_file(const QString &dest_file) {
    auto *s = settings::instance();

    int new_task = 0;
    m_dest_file = dest_file;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = speech_service::instance()->tts_speech_to_file(
            settings::instance()->note(), {});
    } else {
        qDebug() << "[app => dbus] call TtsSpeechToFile";

        new_task =
            m_dbus_service.TtsSpeechToFile(settings::instance()->note(), {});
    }

    m_side_task.set(new_task);
}

void dsnote_app::stop_play_speech() {

}

void dsnote_app::set_speech(service_speech_state_t new_state) {
    if (m_speech_state != new_state) {
        qDebug() << "app speech state:" << m_speech_state << "=>" << new_state;
        m_speech_state = new_state;
        emit speech_changed();
    }
}

void dsnote_app::update_speech() {
    if (m_primary_task != m_current_task) {
        qWarning() << "invalid task, reseting speech state";
        set_speech(service_speech_state_t::SpeechStateNoSpeech);
        return;
    }

    auto new_value = [&] {
        int speech = 0;
        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech = speech_service::instance()->speech();
        } else {
            qDebug() << "[app => dbus] get Speech";
            speech = m_dbus_service.speech();
        }

        switch (speech) {
            case 0:
                return service_speech_state_t::SpeechStateNoSpeech;
            case 1:
                return service_speech_state_t::SpeechStateSpeechDetected;
            case 2:
                return service_speech_state_t::
                    SpeechStateSpeechDecodingEncoding;
            case 3:
                return service_speech_state_t::SpeechStateSpeechInitializing;
            case 4:
                return service_speech_state_t::SpeechStateSpeechPlaying;
        }
        return service_speech_state_t::SpeechStateNoSpeech;
    }();

    set_speech(new_value);
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

            set_speech(service_speech_state_t::SpeechStateNoSpeech);
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

QVariantMap dsnote_app::translate() const {
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
}

bool dsnote_app::busy() const {
    return m_service_state == service_state_t::StateBusy ||
           another_app_connected();
}

bool dsnote_app::stt_configured() const { return m_stt_configured; }

bool dsnote_app::tts_configured() const { return m_tts_configured; }

bool dsnote_app::ttt_configured() const { return m_ttt_configured; }

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

void dsnote_app::copy_to_clipboard() {
    auto *clip = QGuiApplication::clipboard();
    if (!settings::instance()->note().isEmpty()) {
        clip->setText(settings::instance()->note());
        emit note_copied();
    }
}

bool dsnote_app::file_exists(const QString &file) const {
    return QFileInfo::exists(file);
}

bool dsnote_app::file_exists(const QUrl &file) const {
    return file_exists(file.toLocalFile());
}

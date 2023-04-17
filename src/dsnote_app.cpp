/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dsnote_app.h"

#include <QDBusConnection>
#include <QTextStream>
#include <algorithm>

#include "stt_service.h"

QDebug operator<<(QDebug d, dsnote_app::stt_state_t state) {
    switch (state) {
        case dsnote_app::stt_state_t::SttBusy:
            d << "busy";
            break;
        case dsnote_app::stt_state_t::SttIdle:
            d << "idle";
            break;
        case dsnote_app::stt_state_t::SttListeningManual:
            d << "listening-manual";
            break;
        case dsnote_app::stt_state_t::SttListeningAuto:
            d << "listening-auto";
            break;
        case dsnote_app::stt_state_t::SttListeningSingleSentence:
            d << "listening-single-sentence";
            break;
        case dsnote_app::stt_state_t::SttNotConfigured:
            d << "not-configured";
            break;
        case dsnote_app::stt_state_t::SttTranscribingFile:
            d << "transcribing-file";
            break;
        case dsnote_app::stt_state_t::SttUnknown:
            d << "unknown";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, dsnote_app::stt_speech_state_t type) {
    switch (type) {
        case dsnote_app::stt_speech_state_t::SttNoSpeech:
            d << "no-speech";
            break;
        case dsnote_app::stt_speech_state_t::SttSpeechDecoding:
            d << "speech-decoding";
            break;
        case dsnote_app::stt_speech_state_t::SttSpeechInitializing:
            d << "speech-initializing";
            break;
        case dsnote_app::stt_speech_state_t::SttSpeechDetected:
            d << "speech-detected";
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
        case dsnote_app::error_t::ErrorNoSttService:
            d << "no-stt-service-error";
            break;
    }

    return d;
}

dsnote_app::dsnote_app(QObject *parent)
    : QObject{parent},
      m_stt{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
            QDBusConnection::sessionBus()} {
    qDebug() << "starting app:" << settings::instance()->launch_mode();

    connect(settings::instance(), &settings::speech_mode_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::stt_state_changed, this, [this] {
        auto reset_progress = [this]() {
            if (m_transcribe_progress != -1.0) {
                m_transcribe_progress = -1.0;
                emit transcribe_progress_changed();
            }
        };
        if (stt_state() != SttUnknown && stt_state() != SttBusy) {
            update_available_langs();
            update_active_lang();
            update_current_task();
            update_speech();
            if (stt_state() == SttTranscribingFile) {
                update_progress();
            } else {
                reset_progress();
            }
        } else {
            m_transcribe_task.reset();
            m_listen_task.reset();
            m_current_task = INVALID_TASK;
            reset_progress();
        }
    });
    connect(this, &dsnote_app::active_lang_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::available_langs_changed, this,
            &dsnote_app::update_listen);

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        connect_dbus_signals();
        update_stt_state();
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
        connect_dbus_signals();
        do_keepalive();
    }
}

void dsnote_app::update_listen() {
    qDebug() << "update listen";

    if (settings::instance()->speech_mode() ==
        settings::speech_mode_t::SpeechAutomatic) {
        listen();
    } else {
        stop_listen();
    }
}

void dsnote_app::set_intermediate_text(const QString &text, const QString &lang,
                                       int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
#ifdef DEBUG
        qDebug() << "intermediate text decoded:" << text << lang << task;
#else
        qDebug() << "intermediate text decoded: ***" << lang << task;
#endif
    } else {
#ifdef DEBUG
        qDebug() << "[dbus => app] signal IntermediateTextDecoded:" << text
                 << lang << task;
#else
        qDebug() << "[dbus => app] signal IntermediateTextDecoded: ***" << lang
                 << task;
#endif
    }

    if (m_listen_task != task && m_transcribe_task != task) {
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

void dsnote_app::handle_text_decoded(const QString &text, const QString &lang,
                                     int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
#ifdef DEBUG
        qDebug() << "text decoded:" << text << lang << task;
#else
        qDebug() << "text decoded: ***" << lang << task;
#endif
    } else {
#ifdef DEBUG
        qDebug() << "[dbus => app] signal TextDecoded:" << text << lang << task;
#else
        qDebug() << "[dbus => app] signal TextDecoded: ***" << lang << task;
#endif
    }

    if (m_listen_task != task && m_transcribe_task != task) {
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

void dsnote_app::connect_dbus_signals() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        connect(
            stt_service::instance(), &stt_service::models_changed, this,
            [this] {
                handle_stt_models_changed(
                    stt_service::instance()->available_models());
            },
            Qt::QueuedConnection);
        connect(
            stt_service::instance(), &stt_service::state_changed, this,
            [this] {
                handle_stt_state_changed(
                    static_cast<int>(stt_service::instance()->state()));
            },
            Qt::QueuedConnection);
        connect(
            stt_service::instance(), &stt_service::speech_changed, this,
            [this] {
                handle_stt_speech_changed(
                    static_cast<int>(stt_service::instance()->speech()));
            },
            Qt::QueuedConnection);
        connect(
            stt_service::instance(), &stt_service::current_task_changed, this,
            [this] {
                handle_stt_current_task_changed(
                    stt_service::instance()->current_task_id());
            },
            Qt::QueuedConnection);
        connect(
            stt_service::instance(), &stt_service::default_model_changed, this,
            [this] {
                handle_stt_default_model_changed(
                    stt_service::instance()->default_model());
            },
            Qt::QueuedConnection);
        connect(stt_service::instance(),
                &stt_service::transcribe_file_progress_changed, this,
                &dsnote_app::handle_stt_file_transcribe_progress,
                Qt::QueuedConnection);
        connect(stt_service::instance(), &stt_service::file_transcribe_finished,
                this, &dsnote_app::handle_stt_file_transcribe_finished,
                Qt::QueuedConnection);
        connect(
            stt_service::instance(), &stt_service::error, this,
            [this](stt_service::error_t code) {
                handle_stt_error(static_cast<int>(code));
            },
            Qt::QueuedConnection);
        connect(stt_service::instance(),
                &stt_service::intermediate_text_decoded, this,
                &dsnote_app::set_intermediate_text, Qt::QueuedConnection);
        connect(stt_service::instance(), &stt_service::text_decoded, this,
                &dsnote_app::handle_text_decoded, Qt::QueuedConnection);
    } else {
        connect(&m_stt, &OrgMkiolSttInterface::ModelsPropertyChanged, this,
                &dsnote_app::handle_stt_models_changed);
        connect(&m_stt, &OrgMkiolSttInterface::StatePropertyChanged, this,
                &dsnote_app::handle_stt_state_changed);
        connect(&m_stt, &OrgMkiolSttInterface::SpeechPropertyChanged, this,
                &dsnote_app::handle_stt_speech_changed);
        connect(&m_stt, &OrgMkiolSttInterface::CurrentTaskPropertyChanged, this,
                &dsnote_app::handle_stt_current_task_changed);
        connect(&m_stt, &OrgMkiolSttInterface::DefaultModelPropertyChanged,
                this, &dsnote_app::handle_stt_default_model_changed);
        connect(&m_stt, &OrgMkiolSttInterface::FileTranscribeProgress, this,
                &dsnote_app::handle_stt_file_transcribe_progress);
        connect(&m_stt, &OrgMkiolSttInterface::FileTranscribeFinished, this,
                &dsnote_app::handle_stt_file_transcribe_finished);
        connect(&m_stt, &OrgMkiolSttInterface::ErrorOccured, this,
                &dsnote_app::handle_stt_error);
        connect(&m_stt, &OrgMkiolSttInterface::IntermediateTextDecoded, this,
                &dsnote_app::set_intermediate_text);
        connect(&m_stt, &OrgMkiolSttInterface::TextDecoded, this,
                &dsnote_app::handle_text_decoded);
    }
}

void dsnote_app::handle_stt_models_changed(const QVariantMap &models) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        qDebug() << "stt models changed";
    } else {
        qDebug() << "[dbus => app] signal ModelsPropertyChanged";
    }

    m_available_langs_map = models;
    emit available_langs_changed();

    update_active_lang();
}

void dsnote_app::handle_stt_state_changed(int status) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal StatusPropertyChanged:" << status;
    }

    auto old_busy = busy();
    auto old_configured = configured();
    auto old_connected = connected();

    auto new_stt_state = static_cast<stt_state_t>(status);
    if (m_stt_state != new_stt_state) {
        qDebug() << "app stt state:" << m_stt_state << "=>" << new_stt_state;
        m_stt_state = new_stt_state;
        emit stt_state_changed();
    }

    if (old_busy != busy()) {
        qDebug() << "app busy:" << old_busy << "=>" << busy();
        emit busy_changed();
    }
    if (old_configured != configured()) {
        qDebug() << "app configured:" << old_configured << "=>" << configured();
        emit configured_changed();
    }
    if (old_connected != connected()) {
        qDebug() << "app connected:" << old_connected << " = > " << connected();
        emit connected_changed();
    }
}

void dsnote_app::handle_stt_speech_changed(int speech) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal SpeechPropertyChanged:" << speech;
    }

    if (m_listen_task != m_current_task &&
        m_transcribe_task != m_current_task) {
        qWarning() << "ignore SpeechPropertyChanged signal";
        return;
    }

    auto new_speech_state = [speech] {
        switch (speech) {
            case 0:
                return stt_speech_state_t::SttNoSpeech;
            case 1:
                return stt_speech_state_t::SttSpeechDetected;
            case 2:
                return stt_speech_state_t::SttSpeechDecoding;
            case 3:
                return stt_speech_state_t::SttSpeechInitializing;
        }
        return stt_speech_state_t::SttNoSpeech;
    }();

    if (m_speech_state != new_speech_state) {
        qDebug() << "app speech:" << m_speech_state << "=>" << new_speech_state;
        m_speech_state = new_speech_state;
        emit speech_changed();
    }
}

void dsnote_app::handle_stt_current_task_changed(int task) {
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

void dsnote_app::handle_stt_default_model_changed(const QString &model) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal DefaultModelPropertyChanged:"
                 << model;
    }

    if (m_active_lang_value != model) {
        qDebug() << "app active lang:" << m_active_lang_value << "=>" << model;
        m_active_lang_value = model;
        emit active_lang_changed();
    }
}

void dsnote_app::handle_stt_file_transcribe_progress(double new_progress,
                                                     int task) {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
    } else {
        qDebug() << "[dbus => app] signal FileTranscribeProgress:"
                 << new_progress << task;
    }

    if (m_transcribe_task != task) {
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
        qDebug() << "[dbus => app] signal FileTranscribeFinished:" << task;
    }

    if (m_transcribe_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    emit transcribe_done();
}

void dsnote_app::handle_stt_error(int code) {
    auto error_code = static_cast<error_t>(code);

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        qDebug() << "stt error:" << error_code;
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
    if (m_transcribe_task && m_transcribe_task.current != m_current_task) {
        qDebug() << "transcribe task has finished";
        m_transcribe_task.reset();
    }
}

void dsnote_app::update_progress() {
    double new_progress = 0.0;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_progress = m_current_task =
            stt_service::instance()->transcribe_file_progress(
                m_transcribe_task.current);
    } else {
        qDebug() << "[app => dbus] call GetFileTranscribeProgress";
        new_progress =
            m_stt.GetFileTranscribeProgress(m_transcribe_task.current);
    }

    if (m_transcribe_progress != new_progress) {
        m_transcribe_progress = new_progress;
        emit transcribe_progress_changed();
    }
}

void dsnote_app::update_current_task() {
    const auto old = another_app_connected();

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        auto new_current_task = stt_service::instance()->current_task_id();

        if (m_current_task != new_current_task) {
            qDebug() << "app current task:" << m_current_task << "=>"
                     << new_current_task;
            m_current_task = new_current_task;
        }
    } else {
        qDebug() << "[app => dbus] get CurrentTask";
        m_current_task = m_stt.currentTask();
    }

    if (old != another_app_connected()) {
        qDebug() << "app another app connected:" << old << "=>"
                 << another_app_connected();
        emit another_app_connected_changed();
    }

    start_keepalive();
}

void dsnote_app::update_stt_state() {
    stt_state_t new_state;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_state = static_cast<stt_state_t>(stt_service::instance()->state());
    } else {
        qDebug() << "[app => dbus] get Status";
        new_state = static_cast<stt_state_t>(m_stt.state());
    }

    if (m_stt_state != new_state) {
        const auto old_busy = busy();
        const auto old_configured = configured();
        const auto old_connected = connected();

        qDebug() << "app stt state:" << m_stt_state << "=>" << new_state;

        m_stt_state = new_state;
        emit stt_state_changed();
        if (old_busy != busy()) emit busy_changed();
        if (old_configured != configured()) emit configured_changed();
        if (old_connected != connected()) emit connected_changed();
    }
}

void dsnote_app::update_active_lang_idx() {
    auto it = m_available_langs_map.find(active_lang());

    if (it == m_available_langs_map.end()) {
        set_active_lang_idx(-1);
    } else {
        set_active_lang_idx(std::distance(m_available_langs_map.begin(), it));
    }
}

void dsnote_app::update_available_langs() {
    QVariantMap new_available_langs_map{};

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_available_langs_map = stt_service::instance()->available_models();
    } else {
        qDebug() << "[app => dbus] get Models";
        new_available_langs_map = m_stt.models();
    }

    if (m_available_langs_map != new_available_langs_map) {
        qDebug() << "app stt available langs:" << m_available_langs_map.size()
                 << "=>" << new_available_langs_map.size();
        m_available_langs_map = std::move(new_available_langs_map);
        emit available_langs_changed();
    }
}

QVariantList dsnote_app::available_langs() const {
    QVariantList list;

    for (auto it = m_available_langs_map.constBegin();
         it != m_available_langs_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

void dsnote_app::set_active_lang_idx(int idx) {
    if (active_lang_idx() != idx && idx > -1 &&
        idx < m_available_langs_map.size()) {
        const auto &id = std::next(m_available_langs_map.cbegin(), idx).key();

        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            stt_service::instance()->set_default_model(id);
        } else {
            qDebug() << "[app => dbus] set DefaultModel:" << idx << id;
            m_stt.setDefaultModel(id);
        }
    }
}

void dsnote_app::cancel() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        if (m_transcribe_task) {
            stt_service::instance()->cancel(m_transcribe_task.current);
            m_transcribe_task.reset();
        } else if (m_listen_task) {
            stt_service::instance()->cancel(m_listen_task.current);
            m_listen_task.reset();
        } else {
            stt_service::instance()->cancel(m_listen_task.previous);
        }
    } else {
        qDebug() << "[app => dbus] call Cancel";

        if (m_transcribe_task) {
            m_stt.Cancel(m_transcribe_task.current);
            m_transcribe_task.reset();
        } else if (m_listen_task) {
            m_stt.Cancel(m_listen_task.current);
            m_listen_task.reset();
        } else {
            m_stt.Cancel(m_listen_task.previous);
        }
    }
}

void dsnote_app::transcribe_file(const QString &source_file) {
    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = stt_service::instance()->transcribe_file(source_file, {},
                                                            s->translate());
    } else {
        qDebug() << "[app => dbus] call TranscribeFile";
        new_task = m_stt.TranscribeFile(source_file, {}, s->translate());
    }

    m_transcribe_task.set(new_task);
}

void dsnote_app::transcribe_file(const QUrl &source_file) {
    transcribe_file(source_file.toLocalFile());
}

void dsnote_app::listen() {
    auto *s = settings::instance();

    int new_task = 0;

    if (s->launch_mode() == settings::launch_mode_t::app_stanalone) {
        new_task = stt_service::instance()->start_listen(
            static_cast<stt_service::speech_mode_t>(s->speech_mode()), {},
            s->translate());
    } else {
        qDebug() << "[app => dbus] call StartListen:" << s->speech_mode();
        new_task = m_stt.StartListen(static_cast<int>(s->speech_mode()), {},
                                     s->translate());
    }

    m_listen_task.set(new_task);
}

void dsnote_app::stop_listen() {
    if (m_listen_task) {
        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            stt_service::instance()->stop_listen(m_listen_task.current);
        } else {
            qDebug() << "[app => dbus] call StopListen";
            m_stt.StopListen(m_listen_task.current);
        }

        m_listen_task.reset();
    }
}

void dsnote_app::set_speech(stt_speech_state_t new_state) {
    if (m_speech_state != new_state) {
        qDebug() << "app speech state:" << m_speech_state << "=>" << new_state;
        m_speech_state = new_state;
        emit speech_changed();
    }
}

void dsnote_app::update_speech() {
    if (m_listen_task != m_current_task) {
        qWarning() << "ignore update speech";
        return;
    }

    auto new_value = [&] {
        int speech = 0;
        if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::app_stanalone) {
            speech = stt_service::instance()->speech();
        } else {
            qDebug() << "[app => dbus] get Speech";
            speech = m_stt.speech();
        }

        switch (speech) {
            case 0:
                return stt_speech_state_t::SttNoSpeech;
            case 1:
                return stt_speech_state_t::SttSpeechDetected;
            case 2:
                return stt_speech_state_t::SttSpeechDecoding;
            case 3:
                return stt_speech_state_t::SttSpeechInitializing;
        }
        return stt_speech_state_t::SttNoSpeech;
    }();

    set_speech(new_value);
}

int dsnote_app::active_lang_idx() const {
    auto it = m_available_langs_map.find(m_active_lang_value);
    if (it == m_available_langs_map.end()) {
        return -1;
    }
    return std::distance(m_available_langs_map.begin(), it);
}

void dsnote_app::update_active_lang() {
    QString new_lang;

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone) {
        new_lang = stt_service::instance()->default_model();
    } else {
        qDebug() << "[app => dbus] get DefaultModel";
        new_lang = m_stt.defaultModel();
    }

    if (m_active_lang_value != new_lang) {
        qDebug() << "app active lang:" << m_active_lang_value << "=>"
                 << new_lang;
        m_active_lang_value = std::move(new_lang);
        emit active_lang_changed();
    }
}

void dsnote_app::do_keepalive() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone)
        return;

    qDebug() << "[app => dbus] call KeepAliveService";
    const auto time = m_stt.KeepAliveService();

    if (!m_stt.isValid()) {
        if (m_init_attempts >= MAX_INIT_ATTEMPTS) {
            qWarning() << "failed to connect to stt service";
            emit error(error_t::ErrorNoSttService);
        } else {
            m_keepalive_timer.start(KEEPALIVE_TIME);
            m_init_attempts++;
        }
    } else if (time > 0) {
        m_keepalive_timer.start(static_cast<int>(time * 0.75));
        if (m_init_attempts > 0) {
            update_stt_state();
            m_init_attempts = 0;
        }
    }
}

void dsnote_app::start_keepalive() {
    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::app_stanalone)
        return;

    if (!m_keepalive_current_task_timer.isActive() &&
        (m_listen_task == m_current_task ||
         m_transcribe_task == m_current_task)) {
        if (m_listen_task == m_current_task)
            qDebug() << "starting keepalive listen_task";
        if (m_transcribe_task == m_current_task)
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
        auto time = m_stt.KeepAliveTask(task);

        if (time > 0) {
            m_keepalive_current_task_timer.start(static_cast<int>(time * 0.75));
        } else {
            qWarning() << "keepalive task failed";
            if (m_transcribe_task)
                m_transcribe_task.reset();
            else if (m_listen_task)
                m_listen_task.reset();

            update_current_task();
            update_stt_state();

            set_speech(stt_speech_state_t::SttNoSpeech);
        }
    };

    if (m_transcribe_task.current > INVALID_TASK) {
        qDebug() << "keepalive transcribe task timeout:"
                 << m_transcribe_task.current;
        send_ka(m_transcribe_task.current);
    }

    if (m_listen_task.current > INVALID_TASK) {
        qDebug() << "keepalive listen task timeout:" << m_listen_task.current;
        send_ka(m_listen_task.current);
    }
}

QVariantMap dsnote_app::translate() const {
    if (settings::instance()->launch_mode() ==
            settings::launch_mode_t::stt_service &&
        connected())
        return m_stt.translations();

    return {};
}

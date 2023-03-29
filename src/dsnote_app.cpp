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

#include "settings.h"

dsnote_app::dsnote_app(QObject *parent)
    : QObject{parent},
      stt{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH, QDBusConnection::sessionBus()} {
    connect(settings::instance(), &settings::speech_mode_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::stt_state_changed, this, [this] {
        const auto reset_progress = [this]() {
            if (transcribe_progress_value != -1.0) {
                transcribe_progress_value = -1.0;
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
            transcribe_task.reset();
            listen_task.reset();
            current_task_value = INVALID_TASK;
            reset_progress();
        }
    });
    connect(this, &dsnote_app::active_lang_changed, this,
            &dsnote_app::update_listen);
    connect(this, &dsnote_app::available_langs_changed, this,
            &dsnote_app::update_listen);

    keepalive_timer.setSingleShot(true);
    keepalive_timer.setTimerType(Qt::VeryCoarseTimer);
    connect(&keepalive_timer, &QTimer::timeout, this,
            &dsnote_app::do_keepalive);
    keepalive_timer.start(KEEPALIVE_TIME);

    keepalive_current_task_timer.setSingleShot(true);
    keepalive_current_task_timer.setTimerType(Qt::VeryCoarseTimer);
    connect(&keepalive_current_task_timer, &QTimer::timeout, this,
            &dsnote_app::handle_keepalive_task_timeout);

    connect_dbus_signals();
    do_keepalive();
}

void dsnote_app::update_listen() {
    if (settings::instance()->speech_mode() ==
        settings::speech_mode_type::SpeechAutomatic) {
        listen();
    } else {
        stop_listen();
    }
}

void dsnote_app::set_intermediate_text(const QString &text, const QString &lang,
                                       int task) {
#ifdef DEBUG
    qDebug() << "[dbus => app] signal IntermediateTextDecoded:" << text << lang
             << task;
#else
    qDebug() << "[dbus => app] signal IntermediateTextDecoded: ***" << lang
             << task;
#endif

    if (listen_task != task && transcribe_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    if (intermediate_text_value != text) {
        intermediate_text_value = text;
        emit intermediate_text_changed();
    }
}

void dsnote_app::handle_text_decoded(const QString &text, const QString &lang,
                                     int task) {
#ifdef DEBUG
    qDebug() << "[dbus => app] signal TextDecoded:" << text << lang << task;
#else
    qDebug() << "[dbus => app] signal TextDecoded: ***" << lang << task;
#endif
    if (listen_task != task && transcribe_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    auto note = settings::instance()->note();

    QTextStream ss{&note, QIODevice::WriteOnly};

    if (!note.isEmpty()) {
        if (auto last_char = note.at(note.length() - 1); !last_char.isSpace()) {
            if (last_char.isLetterOrNumber())
                ss << ". ";
            else
                ss << ' ';
        }
    }

    auto upper_text{text};

    if (!upper_text.isEmpty()) {
        upper_text[0] = upper_text[0].toUpper();
    }

    ss << upper_text;

    settings::instance()->set_note(note);

    this->intermediate_text_value.clear();

    emit text_changed();
    emit intermediate_text_changed();
}

void dsnote_app::connect_dbus_signals() {
    connect(&stt, &OrgMkiolSttInterface::ModelsPropertyChanged, this,
            [this](const auto &langs) {
                qDebug() << "[dbus => app] signal ModelsPropertyChanged";
                available_langs_map = langs;
                emit available_langs_changed();
                emit active_lang_changed();
            });
    connect(&stt, &OrgMkiolSttInterface::StatePropertyChanged, this,
            [this](int status) {
                qDebug() << "[dbus => app] signal StatusPropertyChanged:"
                         << status;
                const auto old_busy = busy();
                const auto old_configured = configured();
                const auto old_connected = connected();
                stt_state_value = static_cast<stt_state_type>(status);
                emit stt_state_changed();
                if (old_busy != busy()) emit busy_changed();
                if (old_configured != configured()) emit configured_changed();
                if (old_connected != connected()) emit connected_changed();
            });
    connect(&stt, &OrgMkiolSttInterface::SpeechPropertyChanged, this,
            [this](int speech) {
                qDebug() << "[dbus => app] signal SpeechPropertyChanged:"
                         << speech;
                if (listen_task != current_task_value &&
                    transcribe_task != current_task_value) {
                    qWarning() << "ignore SpeechPropertyChanged signal";
                    return;
                }

                speech_state = [speech] {
                    switch (speech) {
                        case 0:
                            return stt_speech_state_type::SttNoSpeech;
                        case 1:
                            return stt_speech_state_type::SttSpeechDetected;
                        case 2:
                            return stt_speech_state_type::SttSpeechDecoding;
                    }
                    return stt_speech_state_type::SttNoSpeech;
                }();

                emit speech_changed();
            });
    connect(&stt, &OrgMkiolSttInterface::CurrentTaskPropertyChanged, this,
            [this](int task) {
                qDebug() << "[dbus => app] signal CurrentTaskPropertyChanged:"
                         << task;
                const auto old = another_app_connected();
                const auto old_busy = busy();
                current_task_value = task;
                if (old != another_app_connected())
                    emit another_app_connected_changed();
                if (old_busy != busy()) emit busy_changed();

                check_transcribe_taks();
                start_keepalive();
                update_speech();
            });
    connect(&stt, &OrgMkiolSttInterface::DefaultModelPropertyChanged, this,
            [this](const auto &lang) {
                qDebug() << "[dbus => app] signal DefaultModelPropertyChanged:"
                         << lang;
                active_lang_value = lang;
                emit active_lang_changed();
            });
    connect(&stt, &OrgMkiolSttInterface::FileTranscribeProgress, this,
            [this](double new_progress, int task) {
                qDebug() << "[dbus => app] signal FileTranscribeProgress:"
                         << new_progress << task;
                if (transcribe_task != task) {
                    qWarning() << "invalid task id";
                    return;
                }
                transcribe_progress_value = new_progress;
                emit transcribe_progress_changed();
            });
    connect(&stt, &OrgMkiolSttInterface::FileTranscribeFinished, this,
            [this](int task) {
                qDebug() << "[dbus => app] signal FileTranscribeFinished:"
                         << task;
                if (transcribe_task != task) {
                    qWarning() << "invalid task id";
                    return;
                }
                emit transcribe_done();
            });
    connect(&stt, &OrgMkiolSttInterface::ErrorOccured, this, [this](int code) {
        qDebug() << "[dbus => app] signal ErrorOccured:" << code;
        const auto error_code = static_cast<error_type>(code);
        if (error_code == error_type::ErrorMicSource &&
            settings::instance()->speech_mode() ==
                settings::speech_mode_type::SpeechAutomatic) {
            qWarning() << "switching to manual mode due to error";
            settings::instance()->set_speech_mode(
                settings::speech_mode_type::SpeechManual);
        }
        this->intermediate_text_value.clear();
        emit intermediate_text_changed();
        emit error(error_code);
    });
    connect(&stt, &OrgMkiolSttInterface::IntermediateTextDecoded, this,
            &dsnote_app::set_intermediate_text);
    connect(&stt, &OrgMkiolSttInterface::TextDecoded, this,
            &dsnote_app::handle_text_decoded);
}

void dsnote_app::check_transcribe_taks() {
    if (transcribe_task && transcribe_task.current != current_task_value) {
        qDebug() << "transcribe task has finished";
        transcribe_task.reset();
    }
}

void dsnote_app::update_progress() {
    qDebug() << "[app => dbus] call GetFileTranscribeProgress";
    const auto new_progress =
        stt.GetFileTranscribeProgress(transcribe_task.current);
    if (transcribe_progress_value != new_progress) {
        transcribe_progress_value = new_progress;
        emit transcribe_progress_changed();
    }
}

void dsnote_app::update_current_task() {
    qDebug() << "[app => dbus] get CurrentTask";
    const auto old = another_app_connected();
    current_task_value = stt.currentTask();
    if (old != another_app_connected()) emit another_app_connected_changed();

    start_keepalive();
}

void dsnote_app::update_stt_state() {
    qDebug() << "[app => dbus] get Status";
    const auto new_state = static_cast<stt_state_type>(stt.state());
    if (stt_state_value != new_state) {
        const auto old_busy = busy();
        const auto old_configured = configured();
        const auto old_connected = connected();
        stt_state_value = new_state;
        emit stt_state_changed();
        if (old_busy != busy()) emit busy_changed();
        if (old_configured != configured()) emit configured_changed();
        if (old_connected != connected()) emit connected_changed();
    }
}

void dsnote_app::update_active_lang_idx() {
    const auto it = available_langs_map.find(active_lang());
    if (it == available_langs_map.end()) {
        set_active_lang_idx(-1);
    } else {
        set_active_lang_idx(std::distance(available_langs_map.begin(), it));
    }
}

void dsnote_app::update_available_langs() {
    auto new_available_langs_map = stt.models();
    if (available_langs_map != new_available_langs_map) {
        available_langs_map = std::move(new_available_langs_map);
        emit available_langs_changed();
    }
}

QVariantList dsnote_app::available_langs() const {
    QVariantList list;
    for (auto it = available_langs_map.constBegin();
         it != available_langs_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

void dsnote_app::set_active_lang_idx(int idx) {
    if (active_lang_idx() != idx && idx > -1 &&
        idx < available_langs_map.size()) {
        const auto &id = std::next(available_langs_map.cbegin(), idx).key();
        qDebug() << "[app => dbus] set DefaultModel:" << idx << id;
        stt.setDefaultModel(id);
    }
}

void dsnote_app::cancel() {
    qDebug() << "[app => dbus] call Cancel";
    if (transcribe_task) {
        stt.Cancel(transcribe_task.current);
        transcribe_task.reset();
    } else if (listen_task) {
        stt.Cancel(listen_task.current);
        listen_task.reset();
    } else {
        stt.Cancel(listen_task.previous);
    }
}

void dsnote_app::transcribe_file(const QString &source_file) {
    qDebug() << "[app => dbus] call TranscribeFile";
    transcribe_task.set(
        stt.TranscribeFile(source_file, {}, settings::instance()->translate()));
}

void dsnote_app::transcribe_file(const QUrl &source_file) {
    qDebug() << "[app => dbus] call TranscribeFile";
    transcribe_task.set(stt.TranscribeFile(source_file.toLocalFile(), {},
                                           settings::instance()->translate()));
}

void dsnote_app::listen() {
    qDebug() << "[app => dbus] call StartListen:"
             << static_cast<int>(settings::instance()->speech_mode());
    auto *s = settings::instance();
    listen_task.set(stt.StartListen(static_cast<int>(s->speech_mode()), {},
                                    s->translate()));
}

void dsnote_app::stop_listen() {
    if (listen_task) {
        qDebug() << "[app => dbus] call StopListen";
        stt.StopListen(listen_task.current);
        listen_task.reset();
    }
}

void dsnote_app::set_speech(stt_speech_state_type new_state) {
    if (speech_state != new_state) {
        qDebug() << "speech state:" << speech_state << "=>" << new_state;
        speech_state = new_state;
        emit speech_changed();
    }
}

void dsnote_app::update_speech() {
    if (listen_task != current_task_value) {
        qWarning() << "ignore update speech";
        return;
    }

    qDebug() << "[app => dbus] get Speech";

    auto new_value = [speech = stt.speech()] {
        switch (speech) {
            case 0:
                return stt_speech_state_type::SttNoSpeech;
            case 1:
                return stt_speech_state_type::SttSpeechDetected;
            case 2:
                return stt_speech_state_type::SttSpeechDecoding;
        }
        return stt_speech_state_type::SttNoSpeech;
    }();

    set_speech(new_value);
}

int dsnote_app::active_lang_idx() const {
    const auto it = available_langs_map.find(active_lang_value);
    if (it == available_langs_map.end()) {
        return -1;
    }
    return std::distance(available_langs_map.begin(), it);
}

void dsnote_app::update_active_lang() {
    qDebug() << "[app => dbus] get DefaultModel";

    auto new_lang = stt.defaultModel();
    if (active_lang_value != new_lang) {
        active_lang_value = std::move(new_lang);
        emit active_lang_changed();
    }
}

void dsnote_app::do_keepalive() {
    qDebug() << "[app => dbus] call KeepAliveService";
    const auto time = stt.KeepAliveService();

    if (!stt.isValid()) {
        if (init_attempts >= MAX_INIT_ATTEMPTS) {
            emit error(error_type::ErrorNoSttService);
            qWarning() << "cannot connect to stt service";
        } else {
            keepalive_timer.start(KEEPALIVE_TIME);
            init_attempts++;
        }
    } else if (time > 0) {
        keepalive_timer.start(static_cast<int>(time * 0.75));
        if (init_attempts > 0) {
            update_stt_state();
            init_attempts = 0;
        }
    }
}

void dsnote_app::start_keepalive() {
    if (!keepalive_current_task_timer.isActive() &&
        (listen_task == current_task_value ||
         transcribe_task == current_task_value)) {
        if (listen_task == current_task_value)
            qDebug() << "starting keepalive listen_task";
        if (transcribe_task == current_task_value)
            qDebug() << "starting keepalive transcribe_task";
        keepalive_current_task_timer.start(KEEPALIVE_TIME);
    }
}

void dsnote_app::handle_keepalive_task_timeout() {
    const auto send_ka = [this](int task) {
        qDebug() << "[app => dbus] call KeepAliveTask";
        const auto time = stt.KeepAliveTask(task);
        if (time > 0) {
            keepalive_current_task_timer.start(static_cast<int>(time * 0.75));
        } else {
            qWarning() << "keepalive task failed";
            if (transcribe_task)
                transcribe_task.reset();
            else if (listen_task)
                listen_task.reset();

            update_current_task();
            update_stt_state();

            set_speech(stt_speech_state_type::SttNoSpeech);
        }
    };

    if (transcribe_task.current > INVALID_TASK) {
        qDebug() << "keepalive transcribe task timeout:"
                 << transcribe_task.current;
        send_ka(transcribe_task.current);
    }

    if (listen_task.current > INVALID_TASK) {
        qDebug() << "keepalive listen task timeout:" << listen_task.current;
        send_ka(listen_task.current);
    }
}

QVariantMap dsnote_app::translate() const {
    if (connected()) {
        return stt.translations();
    }

    return QVariantMap{};
}

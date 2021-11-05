/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dsnote_app.h"

#include <QDBusConnection>
#include <algorithm>

#include "settings.h"
#include "file_source.h"
#include "mic_source.h"

dsnote_app::dsnote_app() : QObject{}, stt{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH, QDBusConnection::sessionBus()}
{
    connect(settings::instance(), &settings::speech_mode_changed, this, &dsnote_app::update_listen);
    connect(this, &dsnote_app::stt_state_changed, this, [this] {
        if (stt_state() != SttUnknown && stt_state() != SttBusy) {
            update_available_langs();
            update_active_lang();
            update_current_task();
            update_speech();
            if (stt_state() == SttTranscribingFile) update_progress();
        } else {
            transcribe_task.reset();
            listen_task.reset();
        }
    });
    connect(this, &dsnote_app::active_lang_changed, this, &dsnote_app::update_listen);
    connect(this, &dsnote_app::available_langs_changed, this, &dsnote_app::update_listen);

    keepalive_timer.setSingleShot(true);
    keepalive_timer.setTimerType(Qt::VeryCoarseTimer);
    connect(&keepalive_timer, &QTimer::timeout, this, &dsnote_app::do_keepalive);
    keepalive_timer.start(KEEPALIVE_TIME);

    keepalive_current_task_timer.setSingleShot(true);
    keepalive_current_task_timer.setTimerType(Qt::VeryCoarseTimer);
    connect(&keepalive_current_task_timer, &QTimer::timeout, this, &dsnote_app::handle_keepalive_task_timeout);

    connect_dbus_signals();
    do_keepalive();
}

void dsnote_app::update_listen()
{
    if (settings::instance()->speech_mode() == settings::speech_mode_type::SpeechAutomatic) {
        listen();
    } else {
        stop_listen();
    }
}

void dsnote_app::set_intermediate_text(const QString &text, const QString &lang, int task)
{
    qDebug() << "[dbus => app] signal IntermediateTextDecoded:" << text << lang << task;

    if (listen_task != task && transcribe_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    if (intermediate_text_value != text) {
        intermediate_text_value = text;
        emit intermediate_text_changed();
    }
}

void dsnote_app::handle_text_decoded(const QString &text, const QString &lang, int task)
{
    qDebug() << "[dbus => app] signal TextDecoded:" << text << lang << task;

    if (listen_task != task && transcribe_task != task) {
        qWarning() << "invalid task id";
        return;
    }

    auto note = settings::instance()->note();

    if (note.isEmpty()) settings::instance()->set_note(text);
    else settings::instance()->set_note(note + "\n" + text);

    this->intermediate_text_value.clear();

    emit text_changed();
    emit intermediate_text_changed();
}

void dsnote_app::connect_dbus_signals()
{
    connect(&stt, &OrgMkiolSttInterface::LangsPropertyChanged, this, [this](const auto &langs) {
        qDebug() << "[dbus => app] signal LangsPropertyChanged";
        available_langs_map = langs;
        emit available_langs_changed();
        emit active_lang_changed();
    });
    connect(&stt, &OrgMkiolSttInterface::StatePropertyChanged, this, [this](int status) {
        qDebug() << "[dbus => app] signal StatusPropertyChanged:" << status;
        const auto old_busy = busy();
        const auto old_configured = configured();
        stt_state_value = static_cast<stt_state_type>(status);
        emit stt_state_changed();
        if (old_busy != busy()) emit busy_changed();
        if (old_configured != configured()) emit configured_changed();
    });
    connect(&stt, &OrgMkiolSttInterface::SpeechPropertyChanged, this, [this](bool speech) {
        qDebug() << "[dbus => app] signal SpeechPropertyChanged:" << speech;
        if (listen_task != current_task_value) {
            qWarning() << "ignore SpeechPropertyChanged signal";
            return;
        }
        speech_value = speech;
        emit speech_changed();
    });
    connect(&stt, &OrgMkiolSttInterface::CurrentTaskPropertyChanged, this, [this](int task) {
        qDebug() << "[dbus => app] signal CurrentTaskPropertyChanged:" << task;
        current_task_value = task;
        update_speech();
    });
    connect(&stt, &OrgMkiolSttInterface::DefaultLangPropertyChanged, this, [this](const auto &lang) {
        qDebug() << "[dbus => app] signal DefaultLangPropertyChanged:" << lang;
        active_lang_value = lang;
        emit active_lang_changed();
    });
    connect(&stt, &OrgMkiolSttInterface::FileTranscribeProgress, this, [this](double new_progress, int task) {
        qDebug() << "[dbus => app] signal FileTranscribeProgress:" << new_progress << task;

        if (transcribe_task != task) {
            qWarning() << "invalid task id";
            return;
        }

        transcribe_progress_value = new_progress;
        emit transcribe_progress_changed();
    });
    connect(&stt, &OrgMkiolSttInterface::FileTranscribeFinished, this, [this](int task) {
        qDebug() << "[dbus => app] signal FileTranscribeFinished:" << task;

        if (transcribe_task != task) {
            qWarning() << "invalid task id";
            return;
        }

        if (listen_task) {
            keepalive_current_task_timer.start(KEEPALIVE_TIME);
        } else {
            keepalive_current_task_timer.stop();
        }

        emit transcribe_done();
    });
    connect(&stt, &OrgMkiolSttInterface::ErrorOccured, this, [this](int code) {
        qDebug() << "[dbus => app] signal ErrorOccured:" << code;
        const auto error_code = static_cast<error_type>(code);
        if (error_code == error_type::ErrorMicSource &&
            settings::instance()->speech_mode() == settings::speech_mode_type::SpeechAutomatic) {
            qWarning() << "switching to manual mode due to error";
            settings::instance()->set_speech_mode(settings::speech_mode_type::SpeechManual);
        }
        emit error(error_code);
    });
    connect(&stt, &OrgMkiolSttInterface::IntermediateTextDecoded, this, &dsnote_app::set_intermediate_text);
    connect(&stt, &OrgMkiolSttInterface::TextDecoded, this, &dsnote_app::handle_text_decoded);
}

void dsnote_app::update_progress()
{
    qDebug() << "[app => dbus] call GetFileTranscribeProgress";
    const auto new_progress = stt.GetFileTranscribeProgress(transcribe_task.current);
    if (transcribe_progress_value != new_progress) {
        transcribe_progress_value = new_progress;
        emit transcribe_progress_changed();
    }
}

void dsnote_app::update_current_task()
{
    qDebug() << "[app => dbus] get CurrentTask";
    current_task_value = stt.currentTask();
}

void dsnote_app::update_stt_state()
{
    qDebug() << "[app => dbus] get Status";
    const auto new_state = static_cast<stt_state_type>(stt.state());
    if (stt_state_value != new_state) {
        const auto old_busy = busy();
        const auto old_configured = configured();
        stt_state_value = new_state;
        emit stt_state_changed();
        if (old_busy != busy()) emit busy_changed();
        if (old_configured != configured()) emit configured_changed();
    }
}

void dsnote_app::update_active_lang_idx()
{
    const auto it = available_langs_map.find(active_lang());
    if (it == available_langs_map.end()) {
        set_active_lang_idx(-1);
    } else {
        set_active_lang_idx(std::distance(available_langs_map.begin(), it));
    }
}

void dsnote_app::update_available_langs()
{
    auto new_available_langs_map = stt.langs();
    if (available_langs_map != new_available_langs_map) {
        available_langs_map = std::move(new_available_langs_map);
        emit available_langs_changed();
    }
}

QVariantList dsnote_app::available_langs() const
{
    QVariantList list;
    for (auto it = available_langs_map.constBegin(); it != available_langs_map.constEnd(); ++it) {
        const auto v = it.value().toStringList();
        if (v.size() > 1) list.push_back(v.at(1));
    }

    return list;
}

void dsnote_app::set_active_lang_idx(int idx)
{
    if (active_lang_idx() != idx && idx > -1 && idx < available_langs_map.size()) {
        const auto& id = std::next(available_langs_map.cbegin(), idx).key();
        qDebug() << "[app => dbus] set DefaultLang:" << idx << id;
        stt.setDefaultLang(id);
    }
}

void dsnote_app::cancel_transcribe()
{
    if (transcribe_task) {
        qDebug() << "[app => dbus] call CancelTranscribeFile";
        stt.CancelTranscribeFile(transcribe_task.current);
        transcribe_task.reset();
        if (listen_task) keepalive_current_task_timer.start(KEEPALIVE_TIME);
    }
}

void dsnote_app::transcribe_file(const QString& source_file)
{
    qDebug() << "[app => dbus] call TranscribeFile";
    transcribe_task.set(stt.TranscribeFile(source_file, {}));
    keepalive_current_task_timer.start(KEEPALIVE_TIME);
}

void dsnote_app::transcribe_file(const QUrl& source_file)
{
    qDebug() << "[app => dbus] call TranscribeFile";
    transcribe_task.set(stt.TranscribeFile(source_file.toLocalFile(), {}));
    keepalive_current_task_timer.start(KEEPALIVE_TIME);
}

void dsnote_app::listen()
{
    qDebug() << "[app => dbus] call StartListen";
    listen_task.set(stt.StartListen(static_cast<int>(settings::instance()->speech_mode()), {}));
    keepalive_current_task_timer.start(KEEPALIVE_TIME);
}

void dsnote_app::stop_listen()
{
    if (listen_task) {
        qDebug() << "[app => dbus] call StopListen";
        stt.StopListen(listen_task.current);
        listen_task.reset();
        keepalive_current_task_timer.stop();
    }
}

void dsnote_app::update_speech()
{
    if (listen_task != current_task_value) {
        qWarning() << "ignore update speech";
        return;
    }

    qDebug() << "[app => dbus] get Speech";
    const auto new_value = stt.speech();
    if (speech_value != new_value) {
        speech_value = new_value;
        emit speech_changed();
    }
}

int dsnote_app::active_lang_idx() const
{
    const auto it = available_langs_map.find(active_lang_value);
    if (it == available_langs_map.end()) {
        return -1;
    } else {
        return std::distance(available_langs_map.begin(), it);
    }
}

void dsnote_app::update_active_lang()
{
    qDebug() << "[app => dbus] get DefaultLang";

    auto new_lang = stt.defaultLang();
    if (active_lang_value != new_lang) {
        active_lang_value = std::move(new_lang);
        emit active_lang_changed();
    }
}

void dsnote_app::do_keepalive()
{
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
        keepalive_timer.start(time * 0.75);
        if (init_attempts > 0) {
            update_stt_state();
            init_attempts = 0;
        }
    }
}

void dsnote_app::handle_keepalive_task_timeout()
{
    int task;
    if (transcribe_task.current > INVALID_TASK) {
        qDebug() << "keepalive transcribe task timeout:" << transcribe_task.current;
        task = transcribe_task.current;
    } else if (listen_task.current > INVALID_TASK) {
        qDebug() << "keepalive listen task timeout:" << listen_task.current;
        task = listen_task.current;;
    } else {
        return;
    }

    qDebug() << "[app => dbus] call KeepAliveTask";
    const auto time = stt.KeepAliveTask(task);
    if (time > 0) {
        keepalive_current_task_timer.start(time * 0.75);
    }
}


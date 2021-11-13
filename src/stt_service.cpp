/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "stt_service.h"

#include <QDebug>
#include <QDBusConnection>
#include <QCoreApplication>
#include <functional>
#include <algorithm>

#include "settings.h"
#include "file_source.h"
#include "mic_source.h"

stt_service::stt_service(QObject *parent) :
    QObject{parent},
    stt_adaptor{this}
{
    connect(this, &stt_service::speech_clear, this, &stt_service::handle_speech_clear);
    connect(this, &stt_service::models_changed, this, &stt_service::refresh_status);
    connect(&manager, &models_manager::models_changed, this, &stt_service::handle_models_changed);
    connect(&manager, &models_manager::busy_changed, this, &stt_service::refresh_status);
    connect(&manager, &models_manager::download_progress, this, [this](const QString& id, double progress) {
        emit model_download_progress(id, progress);
    });
    connect(this, &stt_service::state_changed, this, [this]() {
        qDebug() << "[service => dbus] signal StatePropertyChanged:" << dbus_state();
        emit StatePropertyChanged(dbus_state());
    }, Qt::QueuedConnection);
    connect(this, &stt_service::current_task_changed, this, [this]() {
        qDebug() << "[service => dbus] signal CurrentTaskPropertyChanged:" << current_task_id();
        emit CurrentTaskPropertyChanged(current_task_id());
    }, Qt::QueuedConnection);
    connect(this, &stt_service::error, this, [this](error_type type) {
        qDebug() << "[service => dbus] signal ErrorOccured:" << static_cast<int>(type);
        emit ErrorOccured(static_cast<int>(type));
    });
    connect(this, &stt_service::file_transcribe_finished, this, [this](int task) {
        qDebug() << "[service => dbus] signal FileTranscribeFinished:" << task;
        emit FileTranscribeFinished(task);
    });
    connect(this, &stt_service::intermediate_text_decoded, this, [this](const QString &text, const QString &lang, int task) {
        qDebug() << "[service => dbus] signal IntermediateTextDecoded:" << text << lang << task;
        emit IntermediateTextDecoded(text, lang, task);
    });
    connect(this, &stt_service::text_decoded, this, [this](const QString &text, const QString &lang, int task) {
        qDebug() << "[service => dbus] signal TextDecoded:" << text << lang << task;
        emit TextDecoded(text, lang, task);
    });
    connect(this, &stt_service::speech_changed, this, [this]() {
        qDebug() << "[service => dbus] signal SpeechPropertyChanged:" << speech();
        emit SpeechPropertyChanged(speech());
    });
    connect(this, &stt_service::models_changed, this, [this]() {
        const auto models_list = available_models();
        qDebug() << "[service => dbus] signal ModelsPropertyChanged:" << models_list;
        emit ModelsPropertyChanged(models_list);

        const auto langs_list = available_langs();
        qDebug() << "[service => dbus] signal LangsPropertyChanged:" << langs_list;
        emit LangsPropertyChanged(langs_list);
    });
    connect(this, &stt_service::transcribe_file_progress_changed, this, [this](double progress, int task) {
        qDebug() << "[service => dbus] signal FileTranscribeProgress:" << progress << task;
        emit FileTranscribeProgress(progress, task);
    });
    connect(settings::instance(), &settings::default_model_changed, this, [this]() {
        const auto model = default_model();
        qDebug() << "[service => dbus] signal DefaultModelPropertyChanged:" << model;
        emit DefaultModelPropertyChanged(model);

        const auto lang = default_lang();
        qDebug() << "[service => dbus] signal DefaultLangPropertyChanged:" << lang;
        emit DefaultLangPropertyChanged(lang);
    });

    ds_reset_timer.setSingleShot(true);
    ds_reset_timer.setTimerType(Qt::VeryCoarseTimer);
    ds_reset_timer.setInterval(DS_RESTART_TIME);
    connect(&ds_reset_timer, &QTimer::timeout, this, &stt_service::reset_ds);

    keepalive_timer.setSingleShot(true);
    keepalive_timer.setTimerType(Qt::VeryCoarseTimer);
    keepalive_timer.setInterval(KEEPALIVE_TIME);
    connect(&keepalive_timer, &QTimer::timeout, this, &stt_service::handle_keepalive_timeout);
    keepalive_timer.start();

    keepalive_current_task_timer.setSingleShot(true);
    keepalive_current_task_timer.setTimerType(Qt::VeryCoarseTimer);
    keepalive_current_task_timer.setInterval(KEEPALIVE_TASK_TIME);
    connect(&keepalive_current_task_timer, &QTimer::timeout, this, &stt_service::handle_keepalive_task_timeout);

    // DBus
    auto con = QDBusConnection::sessionBus();
    if (!con.registerService(DBUS_SERVICE_NAME)) {
        qWarning() << "dbus service registration failed";
        throw std::runtime_error("dbus service registration failed");
    }
    if (!con.registerObject(DBUS_SERVICE_PATH, this)) {
        qWarning() << "dbus object registration failed";
        throw std::runtime_error("dbus object registration failed");
    }

    handle_models_changed();
}

stt_service::source_type stt_service::audio_source_type() const
{
    if (!source)
        return source_type::none;
    if (source->type() == audio_source::source_type::mic)
        return source_type::mic;
    if (source->type() == audio_source::source_type::file)
        return source_type::file;
    return source_type::none;
}

std::optional<stt_service::model_files_type> stt_service::choose_model_files(const QString &model_id)
{
    QString id{model_id};

    if (id.isEmpty()) id = settings::instance()->default_model();

    available_models_map.clear();

    const auto models = manager.available_models();
    if (models.empty()) return std::nullopt;

    model_files_type active_files;

    // search by model id
    for (const auto& model : models) {
        if (!id.compare(model.id, Qt::CaseInsensitive)) {
            active_files.model_id = model.id;
            active_files.model_file = model.model_file;
            active_files.scorer_file = model.scorer_file;
        }
        available_models_map.emplace(model.id, model_data_type{model.id, model.lang_id, model.name});
    }

    // search by lang id
    if (active_files.model_id.isEmpty()) {
        for (const auto& model : models) {
            if (!id.compare(model.lang_id, Qt::CaseInsensitive)) {
                active_files.model_id = model.id;
                active_files.model_file = model.model_file;
                active_files.scorer_file = model.scorer_file;
                break;
            }
        }
    }

    // fallback to first model
    if (active_files.model_id.isEmpty()) {
        const auto& model = models.front();
        active_files.model_id = model.id;
        active_files.model_file = model.model_file;
        active_files.scorer_file = model.scorer_file;
        qWarning() << "cannot find requested model, choosing:" << model.id;
    }

    return active_files;
}

void stt_service::handle_models_changed()
{
    choose_model_files();

    if (current_task && available_models_map.find(current_task->model_id) == available_models_map.end()) {
        stop();
    }

    emit models_changed();
}

QString stt_service::restart_ds(speech_mode_type speech_mode, const QString& model_id)
{
    if (const auto model_files = choose_model_files(model_id)) {
        ds_reset_timer.stop();

        const auto model_file_str = model_files->model_file.toStdString();
        const auto scorer_file_str = model_files->scorer_file.toStdString();
        const auto speech_mode_val = speech_mode == speech_mode_type::automatic ?
                    deepspeech_wrapper::speech_mode_type::automatic : deepspeech_wrapper::speech_mode_type::manual;

        if (ds && ds->model_file() == model_file_str && ds->scorer_file() == scorer_file_str) {
            qDebug() << "restart ds not required";
            ds->set_speech_mode(speech_mode_val);
        } else {
            qDebug() << "restart ds required";

            deepspeech_wrapper::callbacks_type call_backs{
                std::bind(&stt_service::handle_text_decoded, this, std::placeholders::_1),
                std::bind(&stt_service::handle_intermediate_text_decoded, this, std::placeholders::_1),
                std::bind(&stt_service::handle_speech_status_changed, this, std::placeholders::_1)
            };

            ds = std::make_unique<deepspeech_wrapper>(model_file_str, scorer_file_str, call_backs, speech_mode_val);
        }

        return model_files->model_id;
    }

    qWarning() << "cannot restart ds, no valid model";
    return {};
}

void stt_service::handle_intermediate_text_decoded(const std::string& text)
{
    if (current_task) {
        last_intermediate_text_task = current_task->id;
        emit intermediate_text_decoded(QString::fromStdString(text), current_task->model_id, current_task->id);
    } else {
        qWarning() << "current task does not exist";
    }
}

void stt_service::handle_text_decoded(const std::string &text)
{
    if (current_task) {
        if (previous_task && last_intermediate_text_task == previous_task->id) {
            emit text_decoded(QString::fromStdString(text), previous_task->model_id, previous_task->id);
        } else {
            emit text_decoded(QString::fromStdString(text), current_task->model_id, current_task->id);
        }
    } else {
        qWarning() << "current task does not exist";
    }

    previous_task.reset();
}

void stt_service::handle_speech_status_changed(bool speech_detected)
{
    if (!speech_detected) emit speech_clear();
    emit speech_changed();
}

void stt_service::handle_speech_clear()
{
    if (source && (state() == state_type::listening_auto || state() == state_type::transcribing_file)) {
        source->clear();
    }
}

QVariantMap stt_service::available_models() const
{
    QVariantMap map;

    std::for_each(available_models_map.cbegin(), available_models_map.cend(),
                   [&map](const auto& p) {
        map.insert(p.first, QStringList{
            p.second.model_id,
            QString{"%1 / %2"}.arg(p.second.name, p.second.lang_id)
        });
    });

    return map;
}

QVariantMap stt_service::available_langs() const
{
    QVariantMap map;

    std::for_each(available_models_map.cbegin(), available_models_map.cend(),
                  [&map](const auto& p) {
                      if (!map.contains(p.second.lang_id)) {
                          map.insert(p.second.lang_id,
                                         QStringList{
                                            p.second.model_id,
                                            QString{"%1 / %2"}.arg(p.second.name, p.second.lang_id)
                                         });
                      }
                  });

    return map;
}

void stt_service::download_model(const QString& id)
{
    manager.download_model(id);
}

void stt_service::delete_model(const QString& id)
{
    if (current_task && current_task->model_id == id) stop();
    manager.delete_model(id);
}

void stt_service::handle_audio_available()
{
    if (source && ds) {
        auto [buff, max_size] = ds->borrow_buff();
        if (buff) {
            ds->return_buff(buff, source->read_audio(buff, max_size));
            set_progress(source->progress());
        }
    }
}

void stt_service::set_progress(double p)
{
    if (audio_source_type() == source_type::file && current_task) {
        const auto delta = p - progress_value;
        if (delta > 0.01 || p < 0.0 || p >= 1) {
            progress_value = p;
            emit transcribe_file_progress_changed(progress_value, current_task->id);
        }
    }
}

double stt_service::transcribe_file_progress(int task) const
{
    if (audio_source_type() == source_type::file) {
        if (current_task && current_task->id == task) {
            return progress_value;
        } else {
            qWarning() << "invalid task id";
        }
    }

    return -1.0;
}

int stt_service::cancel_file(int task)
{
    if (state() == state_type::unknown || state() == state_type::not_configured || state() == state_type::busy) {
        qWarning() << "cannot cancel_file, invalid state";
        return FAILURE;
    }

    if (audio_source_type() == source_type::file) {
        if (current_task && current_task->id == task) {
            if (pending_task) {
                previous_task = current_task;
                restart_ds(pending_task->speech_mode, pending_task->model_id);
                restart_audio_source();
                current_task = pending_task;
                pending_task.reset();
                keepalive_current_task_timer.start();
                emit current_task_changed();
            } else {
                reset_ds();
                keepalive_current_task_timer.stop();
            }
        } else {
            qWarning() << "invalid task id";
            return FAILURE;
        }
    }

    refresh_status();
    return SUCCESS;
}

int stt_service::next_task_id()
{
    last_task_id = (last_task_id + 1) % std::numeric_limits<int>::max();
    return last_task_id;
}

int stt_service::transcribe_file(const QString &file, const QString &lang)
{
    if (state() == state_type::unknown || state() == state_type::not_configured || state() == state_type::busy) {
        qWarning() << "cannot transcribe_file, invalid state";
        return INVALID_TASK;
    }

    if (current_task && audio_source_type() == source_type::mic) {
        pending_task = current_task;
    }

    current_task = {next_task_id(),
                    speech_mode_type::automatic,
                    restart_ds(speech_mode_type::automatic, lang)};

    if (QFileInfo::exists(file)) {
        restart_audio_source(file);
    } else {
        restart_audio_source(QUrl{file}.toLocalFile());
    }

    keepalive_current_task_timer.start();

    emit current_task_changed();

    refresh_status();

    return current_task->id;
}

int stt_service::start_listen(speech_mode_type mode, const QString &lang)
{
    if (state() == state_type::unknown || state() == state_type::not_configured || state() == state_type::busy) {
        qWarning() << "cannot start_listen, invalid state";
        return INVALID_TASK;
    }

    if (audio_source_type() == source_type::file) {
        pending_task = {next_task_id(), mode, lang};
        return pending_task->id;
    }

    current_task = {next_task_id(), mode, restart_ds(mode, lang)};
    restart_audio_source();
    if (ds) ds->set_speech_started(true);

    keepalive_current_task_timer.start();

    emit current_task_changed();

    refresh_status();

    return current_task->id;
}

int stt_service::stop_listen(int task)
{
    if (state() == state_type::unknown || state() == state_type::not_configured || state() == state_type::busy) {
        qWarning() << "cannot stop_listen, invalid state";
        return FAILURE;
    }

    if (audio_source_type() == source_type::file) {
        if (pending_task && pending_task->id == task) pending_task.reset();
        else qWarning() << "invalid task id";
    } else if (audio_source_type() == source_type::mic) {
        if (current_task && current_task->id == task) {
            reset_ds_gracefully();
            keepalive_current_task_timer.stop();
        } else {
            qWarning() << "invalid task id";
            return FAILURE;
        }
    }

    return SUCCESS;
}

void stt_service::reset_ds_gracefully()
{
    if (ds) {
        ds->set_speech_started(false);
        ds_reset_timer.start();
        refresh_status();
    }
}

void stt_service::reset_ds()
{
    ds.reset();
    restart_audio_source();
    pending_task.reset();
    if (current_task) {
        current_task.reset();
        keepalive_current_task_timer.stop();
        emit current_task_changed();
    }
    refresh_status();
}

void stt_service::stop()
{
    qDebug() << "stop";
    reset_ds();
}

void stt_service::handle_audio_error()
{
    if (audio_source_type() == source_type::file && current_task) {
        qWarning() << "cannot start file audio source";
        emit error(error_type::file_source);
        cancel_file(current_task->id);
    } else {
        qWarning() << "cannot start mic audio source";
        emit error(error_type::mic_source);
        stop();
    }
}

void stt_service::handle_audio_ended()
{
    if (audio_source_type() == source_type::file && current_task) {
        qDebug() << "file audio source ended successfuly";
        emit file_transcribe_finished(current_task->id);
        cancel_file(current_task->id);
    } else {
        qDebug() << "audio ended";
    }
}

void stt_service::restart_audio_source(const QString &source_file)
{
    if (ds) {
        ds->restart();

        if (source) source.get()->disconnect();

        if (source_file.isEmpty()) source = std::make_unique<mic_source>();
        else source = std::make_unique<file_source>(source_file);

        set_progress(source->progress());
        connect(source.get(), &audio_source::audio_available, this, &stt_service::handle_audio_available, Qt::QueuedConnection);
        connect(source.get(), &audio_source::error, this, &stt_service::handle_audio_error, Qt::QueuedConnection);
        connect(source.get(), &audio_source::ended, this, &stt_service::handle_audio_ended, Qt::QueuedConnection);
    } else if (source) {
        source.reset();
        set_progress(-1.0);
    }
}

void stt_service::handle_keepalive_timeout()
{
    qWarning() << "keepalive timeout => shutting down";
    QCoreApplication::instance()->quit();
}

void stt_service::handle_keepalive_task_timeout()
{
    if (current_task) {
        qWarning() << "keepalive task timeout:" << current_task->id;
        if (audio_source_type() == source_type::file) {
            cancel_file(current_task->id);
        } else if (audio_source_type() == source_type::mic) {
            stop_listen(current_task->id);
        } else {
            current_task.reset();
            emit current_task_changed();
        }
    }
}

bool stt_service::speech() const
{
    if (ds) return ds->speech_detected();
    return false;
}

void stt_service::refresh_status()
{
    state_type new_state;
    if (manager.busy()) {
        new_state = state_type::busy;
    } else if (manager.available_models().empty()) {
        new_state = state_type::not_configured;
    } else if (audio_source_type() == source_type::file) {
        new_state = state_type::transcribing_file;
    } else if (audio_source_type() == source_type::mic) {
        if (!current_task) {
            qWarning() << "no current task but source is mic";
            return;
        }
        if (current_task->speech_mode == speech_mode_type::manual) {
            new_state = ds && ds->speech_status() ? state_type::listening_manual : state_type::idle;
        } else {
            new_state = state_type::listening_auto;
        }
    } else {
        new_state = state_type::idle;
    }

    if (new_state != state_value) {
        qDebug() << "state changed:" << state_str(state_value) << "=>" << state_str(new_state);
        state_value = new_state;
        emit state_changed();
    }
}

QString stt_service::state_str(state_type state_value)
{
    switch (state_value) {
    case state_type::busy: return "busy";
    case state_type::idle: return "idle";
    case state_type::listening_manual: return "listening_manual";
    case state_type::listening_auto: return "listening_auto";
    case state_type::not_configured: return "not_configured";
    case state_type::transcribing_file: return "transcribing_file";
    default: return "unknown";
    }
}

QString stt_service::default_model() const
{
    return test_default_model(settings::instance()->default_model());
}

QString stt_service::default_lang() const
{
    return available_models_map.at(test_default_model(settings::instance()->default_model())).lang_id;
}

QString stt_service::test_default_model(const QString &lang) const
{
    if (available_models_map.empty()) return {};

    const auto it = available_models_map.find(lang);

    if (it == available_models_map.cend()) {
        const auto it = std::find_if(available_models_map.cbegin(), available_models_map.cend(),
                                     [&lang](const auto& p){ return p.second.lang_id == lang; });
        if (it != available_models_map.cend()) return it->first;
    } else {
        return it->first;
    }

    return available_models_map.begin()->first;
}

void stt_service::set_default_model(const QString &model_id) const
{
    if (test_default_model(model_id) == model_id) {
        settings::instance()->set_default_model(model_id);
    } else {
        qWarning() << "invalid default model";
    }
}

void stt_service::set_default_lang(const QString &lang_id) const
{
    settings::instance()->set_default_model(test_default_model(lang_id));
}

QVariantMap stt_service::translations() const
{
    QVariantMap map;

    map.insert("lang_not_conf", tr("Language is not configured"));
    map.insert("say_smth", tr("Say something..."));
    map.insert("press_say_smth", tr("Press and say something..."));
    map.insert("busy_stt", tr("Busy..."));

    return map;
}

// DBus

int stt_service::StartListen(int mode, const QString &lang)
{
    qDebug() << "[dbus => service] called StartListen:" << lang << mode;
    keepalive_timer.start();

    return start_listen(static_cast<speech_mode_type>(mode), lang);
}

int stt_service::StopListen(int task)
{
    qDebug() << "[dbus => service] called StopListen:" << task;
    keepalive_timer.start();

    return stop_listen(task);
}

int stt_service::CancelTranscribeFile(int task)
{
    qDebug() << "[dbus => service] called CancelTranscribeFile:" << task;
    keepalive_timer.start();

    return cancel_file(task);
}

int stt_service::TranscribeFile(const QString& file, const QString& lang)
{
    qDebug() << "[dbus => service] called TranscribeFile:" << file << lang;
    keepalive_timer.start();

    return transcribe_file(file, lang);
}

double stt_service::GetFileTranscribeProgress(int task)
{
    qDebug() << "[dbus => service] called GetFileTranscribeProgress:" << task;
    keepalive_timer.start();

    return transcribe_file_progress(task);
}

int stt_service::KeepAliveService()
{
    qDebug() << "[dbus => service] called KeepAliveService";
    keepalive_timer.start();

    return keepalive_timer.remainingTime();
}

int stt_service::KeepAliveTask(int task)
{
    qDebug() << "[dbus => service] called KeepAliveTask:" << task;
    keepalive_timer.start();

    if (current_task && current_task->id == task) {
        keepalive_current_task_timer.start();
        return keepalive_current_task_timer.remainingTime();
    } else if (pending_task && pending_task->id == task) {
        qDebug() << "pending:" << task;
        return KEEPALIVE_TASK_TIME;
    }

    qWarning() << "invalid task:" << task;

    return 0;
}

int stt_service::Reload()
{
    qDebug() << "[dbus => service] called Reload";
    keepalive_timer.start();

    manager.reload();
    return SUCCESS;
}

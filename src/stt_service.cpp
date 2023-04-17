/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "stt_service.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <algorithm>
#include <functional>

#include "ds_engine.hpp"
#include "file_source.h"
#include "mic_source.h"
#include "settings.h"
#include "vosk_engine.hpp"
#include "whisper_engine.hpp"

stt_service::stt_service(QObject *parent)
    : QObject{parent}, m_stt_adaptor{this} {
    qDebug() << "starting service:" << settings::instance()->launch_mode();

    connect(this, &stt_service::models_changed, this,
            &stt_service::refresh_status);
    connect(models_manager::instance(), &models_manager::models_changed, this,
            &stt_service::handle_models_changed);
    connect(models_manager::instance(), &models_manager::busy_changed, this,
            &stt_service::refresh_status);
    connect(models_manager::instance(), &models_manager::download_progress,
            this, [this](const QString &id, double progress) {
                emit model_download_progress(id, progress);
            });
    connect(this, &stt_service::text_decoded, this,
            static_cast<void (stt_service::*)(const QString &, const QString &,
                                              int)>(
                &stt_service::handle_text_decoded),
            Qt::QueuedConnection);
    connect(this, &stt_service::sentence_timeout, this,
            static_cast<void (stt_service::*)(int)>(
                &stt_service::handle_sentence_timeout),
            Qt::QueuedConnection);
    connect(this, &stt_service::engine_eof, this,
            static_cast<void (stt_service::*)(int)>(
                &stt_service::handle_engine_eof),
            Qt::QueuedConnection);
    connect(this, &stt_service::engine_error, this,
            static_cast<void (stt_service::*)(int)>(
                &stt_service::handle_engine_error),
            Qt::QueuedConnection);
    connect(this, &stt_service::engine_shutdown, this,
            [this] { stop_engine(); });
    connect(
        settings::instance(), &settings::default_model_changed, this,
        [this]() {
            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::app) {
                auto model = default_model();
                qDebug()
                    << "[service => dbus] signal DefaultModelPropertyChanged:"
                    << model;
                emit DefaultModelPropertyChanged(model);
            }

            emit default_model_changed();

            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::app) {
                auto lang = default_lang();
                qDebug()
                    << "[service => dbus] signal DefaultLangPropertyChanged:"
                    << lang;
                emit DefaultLangPropertyChanged(lang);
            }

            emit default_lang_changed();
        },
        Qt::QueuedConnection);

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::stt_service) {
        connect(
            this, &stt_service::state_changed, this,
            [this]() {
                qDebug() << "[service => dbus] signal StatePropertyChanged:"
                         << dbus_state();
                emit StatePropertyChanged(dbus_state());
            },
            Qt::QueuedConnection);
        connect(
            this, &stt_service::current_task_changed, this,
            [this]() {
                qDebug()
                    << "[service => dbus] signal CurrentTaskPropertyChanged:"
                    << current_task_id();
                emit CurrentTaskPropertyChanged(current_task_id());
            },
            Qt::QueuedConnection);
        connect(this, &stt_service::error, this, [this](error_t type) {
            qDebug() << "[service => dbus] signal ErrorOccured:"
                     << static_cast<int>(type);
            emit ErrorOccured(static_cast<int>(type));
        });
        connect(
            this, &stt_service::file_transcribe_finished, this,
            [this](int task) {
                qDebug() << "[service => dbus] signal FileTranscribeFinished:"
                         << task;
                emit FileTranscribeFinished(task);
            });
        connect(
            this, &stt_service::intermediate_text_decoded, this,
            [this](const QString &text, const QString &lang, int task) {
                qDebug() << "[service => dbus] signal IntermediateTextDecoded:"
                         << lang << task;
                emit IntermediateTextDecoded(text, lang, task);
            });
        connect(this, &stt_service::text_decoded, this,
                [this](const QString &text, const QString &lang, int task) {
                    qDebug() << "[service => dbus] signal TextDecoded:" << lang
                             << task;
                    emit TextDecoded(text, lang, task);
                });
        connect(
            this, &stt_service::speech_changed, this,
            [this]() {
                qDebug() << "[service => dbus] signal SpeechPropertyChanged:"
                         << speech();
                emit SpeechPropertyChanged(speech());
            },
            Qt::QueuedConnection);
        connect(this, &stt_service::models_changed, this, [this]() {
            auto models_list = available_models();
            qDebug() << "[service => dbus] signal ModelsPropertyChanged:"
                     << models_list;
            emit ModelsPropertyChanged(models_list);

            auto langs_list = available_langs();
            qDebug() << "[service => dbus] signal LangsPropertyChanged:"
                     << langs_list;
            emit LangsPropertyChanged(langs_list);
        });
        connect(this, &stt_service::transcribe_file_progress_changed, this,
                [this](double progress, int task) {
                    qDebug()
                        << "[service => dbus] signal FileTranscribeProgress:"
                        << progress << task;
                    emit FileTranscribeProgress(progress, task);
                });

        m_keepalive_timer.setSingleShot(true);
        m_keepalive_timer.setTimerType(Qt::VeryCoarseTimer);
        m_keepalive_timer.setInterval(KEEPALIVE_TIME);
        connect(&m_keepalive_timer, &QTimer::timeout, this,
                &stt_service::handle_keepalive_timeout);
        m_keepalive_timer.start();

        m_keepalive_current_task_timer.setSingleShot(true);
        m_keepalive_current_task_timer.setTimerType(Qt::VeryCoarseTimer);
        m_keepalive_current_task_timer.setInterval(KEEPALIVE_TASK_TIME);
        connect(&m_keepalive_current_task_timer, &QTimer::timeout, this,
                &stt_service::handle_task_timeout);

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
    }

    handle_models_changed();
}

stt_service::source_t stt_service::audio_source_type() const {
    if (!m_source) return source_t::none;
    if (m_source->type() == audio_source::source_type::mic)
        return source_t::mic;
    if (m_source->type() == audio_source::source_type::file)
        return source_t::file;
    return source_t::none;
}

std::optional<stt_service::model_files_t> stt_service::choose_model_files(QString model_id) {
    if (model_id.isEmpty()) model_id = settings::instance()->default_model();

    m_available_models_map.clear();

    auto models = models_manager::instance()->available_models();
    if (models.empty()) return std::nullopt;

    model_files_t active_files;

    // search by model id
    for (const auto &model : models) {
        if (model_id.compare(model.id, Qt::CaseInsensitive) == 0) {
            active_files.model_id = model.id;
            active_files.engine = model.engine;
            active_files.lang_id = model.lang_id;
            active_files.model_file = model.model_file;
            active_files.scorer_file = model.scorer_file;
        }
        m_available_models_map.emplace(
            model.id,
            model_data_t{model.id, model.lang_id, model.engine, model.name});
    }

    // search by lang id
    if (active_files.model_id.isEmpty()) {
        int best_score = -1;
        const decltype(models)::value_type *best_model = nullptr;

        for (const auto &model : models) {
            if (model_id.compare(model.lang_id, Qt::CaseInsensitive) == 0) {
                if (model.default_for_lang) {
                    best_model = &model;
                    qDebug() << "best model is default model for lang:"
                             << model.lang_id << model.id;
                    break;
                }

                if (model.score > best_score) {
                    best_model = &model;
                    best_score = model.score;
                }
            }
        }

        if (best_model) {
            active_files.model_id = best_model->id;
            active_files.engine = best_model->engine;
            active_files.lang_id = best_model->lang_id;
            active_files.model_file = best_model->model_file;
            active_files.scorer_file = best_model->scorer_file;
        }
    }

    // fallback to first model
    if (active_files.model_id.isEmpty()) {
        const auto &model = models.front();
        active_files.model_id = model.id;
        active_files.engine = model.engine;
        active_files.lang_id = model.lang_id;
        active_files.model_file = model.model_file;
        active_files.scorer_file = model.scorer_file;
        qWarning() << "cannot find requested model, choosing:" << model.id;
    }

    return active_files;
}

void stt_service::handle_models_changed() {
    choose_model_files();

    if (m_current_task &&
        m_available_models_map.find(m_current_task->model_id) ==
            m_available_models_map.end()) {
        stop();
    }

    emit models_changed();
}

QString stt_service::lang_from_model_id(const QString &model_id) {
    auto l = model_id.split('_');

    if (l.empty()) {
        qDebug() << "invalid model id:" << model_id;
        return {};
    }

    return l.first();
}

QString stt_service::restart_engine(speech_mode_t speech_mode,
                                    const QString &model_id, bool translate) {
    if (auto model_files = choose_model_files(model_id)) {
        stt_engine::config_t config;

        config.model_file = {model_files->model_file.toStdString(),
                             model_files->scorer_file.toStdString()};
        config.lang = model_files->lang_id.toStdString();
        config.speech_mode =
            static_cast<stt_engine::speech_mode_t>(speech_mode);
        config.translate = translate;

        bool new_engine_required = [&] {
            if (!m_engine) return true;
            if (translate != m_engine->translate()) return true;

            const auto &type = typeid(*m_engine);
            if (model_files->engine == models_manager::model_engine::stt_ds &&
                type != typeid(ds_engine))
                return true;
            if (model_files->engine == models_manager::model_engine::stt_vosk &&
                type != typeid(vosk_engine))
                return true;
            if (model_files->engine ==
                    models_manager::model_engine::stt_whisper &&
                type != typeid(whisper_engine))
                return true;

            if (m_engine->model_file() != config.model_file) return true;
            if (m_engine->lang() != config.lang) return true;

            return false;
        }();

        if (new_engine_required) {
            qDebug() << "new engine required";

            stt_engine::callbacks_t call_backs{
                /*text_decoded=*/[this](const std::string &text) {
                    handle_text_decoded(text);
                },
                /*intermediate_text_decoded=*/
                [this](const std::string &text) {
                    handle_intermediate_text_decoded(text);
                },
                /*speech_detection_status_changed=*/
                [this](stt_engine::speech_detection_status_t status) {
                    handle_speech_detection_status_changed(status);
                },
                /*sentence_timeout=*/
                [this]() { handle_sentence_timeout(); },
                /*eof=*/
                [this]() { handle_engine_eof(); },
                /*stopped=*/
                [this]() { handle_engine_error(); }};

            switch (model_files->engine) {
                case models_manager::model_engine::stt_ds:
                    m_engine = std::make_unique<ds_engine>(
                        std::move(config), std::move(call_backs));
                    break;
                case models_manager::model_engine::stt_vosk:
                    m_engine = std::make_unique<vosk_engine>(
                        std::move(config), std::move(call_backs));
                    break;
                case models_manager::model_engine::stt_whisper:
                    m_engine = std::make_unique<whisper_engine>(
                        std::move(config), std::move(call_backs));
                    break;
            }

            m_engine->start();
        } else {
            qDebug() << "new engine not required, only restart";
            m_engine->stop();
            m_engine->start();
            m_engine->set_speech_mode(
                static_cast<stt_engine::speech_mode_t>(speech_mode));
        }

        return model_files->model_id;
    }

    qWarning() << "failed to restart engine, no valid model";
    return {};
}

void stt_service::handle_intermediate_text_decoded(const std::string &text) {
    if (m_current_task) {
        m_last_intermediate_text_task = m_current_task->id;
        emit intermediate_text_decoded(QString::fromStdString(text),
                                       m_current_task->model_id,
                                       m_current_task->id);
    } else {
        qWarning() << "current task does not exist";
    }
}

void stt_service::handle_text_decoded(const QString &, const QString &,
                                      int task_id) {
    if (m_current_task && m_current_task->id == task_id &&
        m_current_task->speech_mode == speech_mode_t::single_sentence) {
        stop_listen(m_current_task->id);
    }
}

void stt_service::handle_sentence_timeout(int task_id) { stop_listen(task_id); }

void stt_service::handle_sentence_timeout() {
    if (m_current_task &&
        m_current_task->speech_mode == speech_mode_t::single_sentence) {
        emit sentence_timeout(m_current_task->id);
    }
}

void stt_service::handle_engine_eof(int task_id) {
    qDebug() << "engine eof";
    emit file_transcribe_finished(task_id);
    cancel(task_id);
}

void stt_service::handle_engine_eof() {
    if (m_current_task) emit engine_eof(m_current_task->id);
}

void stt_service::handle_engine_error(int task_id) {
    qDebug() << "engine error";

    if (current_task_id() == task_id) cancel(task_id);
}

void stt_service::handle_engine_error() {
    if (m_current_task) emit engine_error(m_current_task->id);
}

void stt_service::handle_text_decoded(const std::string &text) {
    if (m_current_task) {
        if (m_previous_task &&
            m_last_intermediate_text_task == m_previous_task->id) {
            emit text_decoded(QString::fromStdString(text),
                              m_previous_task->model_id, m_previous_task->id);
        } else {
            emit text_decoded(QString::fromStdString(text),
                              m_current_task->model_id, m_current_task->id);
        }
    } else {
        qWarning() << "current task does not exist";
    }

    m_previous_task.reset();
}

void stt_service::handle_speech_detection_status_changed(
    [[maybe_unused]] stt_engine::speech_detection_status_t status) {
    emit speech_changed();
}

QVariantMap stt_service::available_models() const {
    QVariantMap map;

    std::for_each(m_available_models_map.cbegin(),
                  m_available_models_map.cend(), [&map](const auto &p) {
                      map.insert(
                          p.first,
                          QStringList{p.second.model_id,
                                      QStringLiteral("%1 / %2").arg(
                                          p.second.name, p.second.lang_id)});
                  });

    return map;
}

QVariantMap stt_service::available_langs() const {
    QVariantMap map;

    std::for_each(
        m_available_models_map.cbegin(), m_available_models_map.cend(),
        [&map](const auto &p) {
            if (!map.contains(p.second.lang_id)) {
                map.insert(p.second.lang_id,
                           QStringList{p.second.model_id,
                                       QStringLiteral("%1 / %2").arg(
                                           p.second.name, p.second.lang_id)});
            }
        });

    return map;
}

void stt_service::download_model(const QString &id) {
    models_manager::instance()->download_model(id);
}

void stt_service::delete_model(const QString &id) {
    if (m_current_task && m_current_task->model_id == id) stop();
    models_manager::instance()->delete_model(id);
}

void stt_service::handle_audio_available() {
    if (m_source && m_engine && m_engine->started()) {
        if (m_engine->speech_detection_status() ==
            stt_engine::speech_detection_status_t::initializing) {
            if (m_source->type() == audio_source::source_type::mic)
                m_source->clear();
            return;
        }

        auto [buf, max_size] = m_engine->borrow_buf();

        if (buf) {
            auto audio_data = m_source->read_audio(buf, max_size);

            if (audio_data.eof) qDebug() << "audio eof";

            m_engine->return_buf(buf, audio_data.size, audio_data.sof,
                                 audio_data.eof);
            set_progress(m_source->progress());
        }
    }
}

void stt_service::set_progress(double p) {
    if (audio_source_type() == source_t::file && m_current_task) {
        const auto delta = p - m_progress;
        if (delta > 0.01 || p < 0.0 || p >= 1) {
            m_progress = p;
            emit transcribe_file_progress_changed(m_progress,
                                                  m_current_task->id);
        }
    }
}

double stt_service::transcribe_file_progress(int task) const {
    if (audio_source_type() == source_t::file) {
        if (m_current_task && m_current_task->id == task) {
            return m_progress;
        }
        qWarning() << "invalid task id";
    }

    return -1.0;
}

int stt_service::next_task_id() {
    m_last_task_id = (m_last_task_id + 1) % std::numeric_limits<int>::max();
    return m_last_task_id;
}

int stt_service::transcribe_file(const QString &file, const QString &lang,
                                 bool translate) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot transcribe_file, invalid state";
        return INVALID_TASK;
    }

    if (m_current_task &&
        m_current_task->speech_mode != speech_mode_t::single_sentence &&
        audio_source_type() == source_t::mic) {
        m_pending_task = m_current_task;
    }

    m_current_task = {next_task_id(), speech_mode_t::automatic,
                      restart_engine(speech_mode_t::automatic, lang, translate),
                      translate};

    if (QFileInfo::exists(file)) {
        restart_audio_source(file);
    } else {
        restart_audio_source(QUrl{file}.toLocalFile());
    }

    start_keepalive_current_task();

    emit current_task_changed();

    refresh_status();

    return m_current_task->id;
}

int stt_service::start_listen(speech_mode_t mode, const QString &lang,
                              bool translate) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot start_listen, invalid state";
        return INVALID_TASK;
    }

    if (audio_source_type() == source_t::file) {
        m_pending_task = {next_task_id(), mode, lang};
        return m_pending_task->id;
    }

    m_current_task = {next_task_id(), mode,
                      restart_engine(mode, lang, translate), translate};
    restart_audio_source();
    if (m_engine) m_engine->set_speech_started(true);

    start_keepalive_current_task();

    emit current_task_changed();

    refresh_status();

    return m_current_task->id;
}

int stt_service::cancel(int task) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot cancel, invalid state";
        return FAILURE;
    }

    qDebug() << "cancel";

    if (audio_source_type() == source_t::file) {
        if (m_current_task && m_current_task->id == task) {
            if (m_pending_task) {
                m_previous_task = m_current_task;
                restart_engine(m_pending_task->speech_mode,
                               m_pending_task->model_id,
                               m_pending_task->translate);
                restart_audio_source();
                m_current_task = m_pending_task;
                start_keepalive_current_task();
                m_pending_task.reset();
                emit current_task_changed();
            } else { 
                stop_engine();
                stop_keepalive_current_task();
            }
        } else {
            qWarning() << "invalid task id";
            return FAILURE;
        }
    } else if (audio_source_type() == source_t::mic) {
        if (m_current_task && m_current_task->id == task) {
            if (m_current_task->speech_mode == speech_mode_t::automatic) {
                restart_engine(m_current_task->speech_mode,
                               m_current_task->model_id,
                               m_current_task->translate);
                restart_audio_source();
            } else {
                stop_keepalive_current_task();
                stop_engine();
            }
        } else {
            qWarning() << "invalid task id";
            return FAILURE;
        }
    }

    refresh_status();
    return SUCCESS;
}

int stt_service::stop_listen(int task) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot stop_listen, invalid state";
        return FAILURE;
    }

    if (audio_source_type() == source_t::file) {
        if (m_pending_task && m_pending_task->id == task)
            m_pending_task.reset();
        else
            qWarning() << "invalid task id";
    } else if (audio_source_type() == source_t::mic) {
        if (m_current_task && m_current_task->id == task) {
            stop_keepalive_current_task();
            if (m_current_task->speech_mode == speech_mode_t::single_sentence ||
                m_current_task->speech_mode == speech_mode_t::automatic) {
                stop_engine();
            } else if (m_engine && m_engine->started()) {
                stop_engine_gracefully();
            } else {
                stop_engine();
            }
        } else {
            qWarning() << "invalid task id";
            return FAILURE;
        }
    }

    return SUCCESS;
}

void stt_service::stop_engine_gracefully() {
    qDebug() << "stop engine gracefully";

    if (m_source) {
        if (m_engine) m_engine->set_speech_started(false);
        m_source->stop();
    } else {
        stop_engine();
    }
}

void stt_service::stop_engine() {
    qDebug() << "stop engine";

    if (m_engine) m_engine->stop();

    restart_audio_source();

    m_pending_task.reset();

    if (m_current_task) {
        m_current_task.reset();
        stop_keepalive_current_task();
        emit current_task_changed();
    }

    refresh_status();
}

void stt_service::stop() {
    qDebug() << "stop";
    stop_engine();
}

void stt_service::handle_audio_error() {
    if (audio_source_type() == source_t::file && m_current_task) {
        qWarning() << "file audio source error";
        emit error(error_t::file_source);
        cancel(m_current_task->id);
    } else {
        qWarning() << "audio source error";
        emit error(error_t::mic_source);
        stop();
    }
}

void stt_service::handle_audio_ended() {
    if (audio_source_type() == source_t::file && m_current_task) {
        qDebug() << "file audio source ended successfuly";
    } else {
        qDebug() << "audio source ended successfuly";
    }
}

void stt_service::restart_audio_source(const QString &source_file) {
    if (m_engine && m_engine->started()) {
        qDebug() << "creating audio source";

        if (m_source) m_source->disconnect();

        if (source_file.isEmpty())
            m_source = std::make_unique<mic_source>();
        else
            m_source = std::make_unique<file_source>(source_file);

        set_progress(m_source->progress());
        connect(m_source.get(), &audio_source::audio_available, this,
                &stt_service::handle_audio_available, Qt::QueuedConnection);
        connect(m_source.get(), &audio_source::error, this,
                &stt_service::handle_audio_error, Qt::QueuedConnection);
        connect(m_source.get(), &audio_source::ended, this,
                &stt_service::handle_audio_ended, Qt::QueuedConnection);
    } else if (m_source) {
        m_source.reset();
        set_progress(-1.0);
    }
}

void stt_service::handle_keepalive_timeout() {
    qWarning() << "keepalive timeout => shutting down";
    QCoreApplication::quit();
}

void stt_service::handle_task_timeout() {
    if (m_current_task) {
        qWarning() << "task timeout:" << m_current_task->id;
        if (m_current_task->speech_mode == speech_mode_t::single_sentence)
            stop_keepalive_current_task();
        if (audio_source_type() == source_t::file ||
            audio_source_type() == source_t::mic) {
            cancel(m_current_task->id);
        } else {
            m_current_task.reset();
            emit current_task_changed();
        }
    }
}

int stt_service::speech() const {
    // 0 = No Speech
    // 1 = Speech detected
    // 2 = Speech decoding in progress
    // 3 = Speech model initialization

    if (m_engine && m_engine->started()) {
        switch (m_engine->speech_detection_status()) {
            case stt_engine::speech_detection_status_t::no_speech:
                return 0;
            case stt_engine::speech_detection_status_t::speech_detected:
                return 1;
            case stt_engine::speech_detection_status_t::decoding:
                return 2;
            case stt_engine::speech_detection_status_t::initializing:
                return 3;
        }
    }
    return 0;
}

void stt_service::refresh_status() {
    state_t new_state;
    if (models_manager::instance()->busy()) {
        new_state = state_t::busy;
    } else if (models_manager::instance()->available_models().empty()) {
        new_state = state_t::not_configured;
    } else if (audio_source_type() == source_t::file) {
        new_state = state_t::transcribing_file;
    } else if (audio_source_type() == source_t::mic) {
        if (!m_current_task) {
            qWarning() << "no current task but source is mic";
            return;
        }
        if (m_current_task->speech_mode == speech_mode_t::manual) {
            new_state =
                m_engine && m_engine->started() && m_engine->speech_status()
                    ? state_t::listening_manual
                    : state_t::idle;
        } else if (m_current_task->speech_mode == speech_mode_t::automatic) {
            new_state = state_t::listening_auto;
        } else if (m_current_task->speech_mode ==
                   speech_mode_t::single_sentence) {
            new_state = state_t::listening_single_sentence;
        } else {
            qWarning() << "unknown speech mode";
            new_state = state_t::unknown;
        }
    } else {
        new_state = state_t::idle;
    }

    if (new_state != m_state) {
        qDebug() << "state changed:" << state_str(m_state) << "=>"
                 << state_str(new_state);
        m_state = new_state;
        emit state_changed();
    }
}

QString stt_service::state_str(state_t state_value) {
    switch (state_value) {
        case state_t::busy:
            return QStringLiteral("busy");
        case state_t::idle:
            return QStringLiteral("idle");
        case state_t::listening_manual:
            return QStringLiteral("listening_manual");
        case state_t::listening_auto:
            return QStringLiteral("listening_auto");
        case state_t::not_configured:
            return QStringLiteral("not_configured");
        case state_t::transcribing_file:
            return QStringLiteral("transcribing_file");
        case state_t::listening_single_sentence:
            return QStringLiteral("listening_single_sentence");
        default:
            return QStringLiteral("unknown");
    }
}

QString stt_service::default_model() const {
    return test_default_model(settings::instance()->default_model());
}

QString stt_service::default_lang() const {
    return m_available_models_map
        .at(test_default_model(settings::instance()->default_model()))
        .lang_id;
}

QString stt_service::test_default_model(const QString &lang) const {
    if (m_available_models_map.empty()) return {};

    auto it = m_available_models_map.find(lang);

    if (it == m_available_models_map.cend()) {
        it = std::find_if(
            m_available_models_map.cbegin(), m_available_models_map.cend(),
            [&lang](const auto &p) { return p.second.lang_id == lang; });
        if (it != m_available_models_map.cend()) return it->first;
    } else {
        return it->first;
    }

    return m_available_models_map.begin()->first;
}

void stt_service::set_default_model(const QString &model_id) const {
    if (test_default_model(model_id) == model_id) {
        settings::instance()->set_default_model(model_id);
    } else {
        qWarning() << "invalid default model";
    }
}

void stt_service::set_default_lang(const QString &lang_id) const {
    settings::instance()->set_default_model(test_default_model(lang_id));
}

QVariantMap stt_service::translations() const {
    QVariantMap map;

    map.insert(QStringLiteral("lang_not_conf"),
               tr("Language is not configured"));
    map.insert(QStringLiteral("say_smth"), tr("Say something..."));
    map.insert(QStringLiteral("press_say_smth"),
               tr("Press and say something..."));
    map.insert(QStringLiteral("click_say_smth"),
               tr("Click and say something..."));
    map.insert(QStringLiteral("busy_stt"), tr("Busy..."));
    map.insert(QStringLiteral("decoding"), tr("Decoding, please wait..."));
    map.insert(QStringLiteral("initializing"),
               tr("Getting ready, please wait..."));

    return map;
}

stt_service::state_t stt_service::state() const { return m_state; };

int stt_service::current_task_id() const {
    return m_current_task ? m_current_task->id : INVALID_TASK;
}

int stt_service::dbus_state() const { return static_cast<int>(state()); };

void stt_service::start_keepalive_current_task() {
    if (settings::instance()->launch_mode() !=
        settings::launch_mode_t::stt_service)
        return;

    m_keepalive_current_task_timer.start();
}

void stt_service::stop_keepalive_current_task() {
    if (settings::instance()->launch_mode() !=
        settings::launch_mode_t::stt_service)
        return;

    m_keepalive_current_task_timer.stop();
}

// DBus

int stt_service::StartListen(int mode, const QString &lang, bool translate) {
    qDebug() << "[dbus => service] called StartListen:" << lang << mode;
    m_keepalive_timer.start();

    return start_listen(static_cast<speech_mode_t>(mode), lang, translate);
}

int stt_service::StopListen(int task) {
    qDebug() << "[dbus => service] called StopListen:" << task;
    m_keepalive_timer.start();

    return stop_listen(task);
}

int stt_service::Cancel(int task) {
    qDebug() << "[dbus => service] called Cancel:" << task;
    m_keepalive_timer.start();

    return cancel(task);
}

int stt_service::TranscribeFile(const QString &file, const QString &lang,
                                bool translate) {
    qDebug() << "[dbus => service] called TranscribeFile:" << file << lang;
    start_keepalive_current_task();

    return transcribe_file(file, lang, translate);
}

double stt_service::GetFileTranscribeProgress(int task) {
    qDebug() << "[dbus => service] called GetFileTranscribeProgress:" << task;
    start_keepalive_current_task();

    return transcribe_file_progress(task);
}

int stt_service::KeepAliveService() {
    qDebug() << "[dbus => service] called KeepAliveService";
    m_keepalive_timer.start();

    return m_keepalive_timer.remainingTime();
}

int stt_service::KeepAliveTask(int task) {
    qDebug() << "[dbus => service] called KeepAliveTask:" << task;
    m_keepalive_timer.start();

    if (m_current_task && m_current_task->id == task) {
        m_keepalive_current_task_timer.start();
        return m_keepalive_current_task_timer.remainingTime();
    }
    if (m_pending_task && m_pending_task->id == task) {
        qDebug() << "pending:" << task;
        return KEEPALIVE_TASK_TIME;
    }

    qWarning() << "invalid task:" << task;

    return 0;
}

int stt_service::Reload() {
    qDebug() << "[dbus => service] called Reload";
    m_keepalive_timer.start();

    models_manager::instance()->reload();
    return SUCCESS;
}

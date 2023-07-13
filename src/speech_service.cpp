/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "speech_service.h"

#include <fmt/format.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QEventLoop>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <optional>
#include <set>

#include "coqui_engine.hpp"
#include "ds_engine.hpp"
#include "espeak_engine.hpp"
#include "file_source.h"
#include "mic_source.h"
#include "module_tools.hpp"
#include "piper_engine.hpp"
#include "rhvoice_engine.hpp"
#include "settings.h"
#include "vosk_engine.hpp"
#include "whisper_engine.hpp"

QDebug operator<<(QDebug d, const stt_engine::config_t &config) {
    std::stringstream ss;
    ss << config;

    d << QString::fromStdString(ss.str());

    return d;
}

QDebug operator<<(QDebug d, const tts_engine::config_t &config) {
    std::stringstream ss;
    ss << config;

    d << QString::fromStdString(ss.str());

    return d;
}

QDebug operator<<(QDebug d, const mnt_engine::config_t &config) {
    std::stringstream ss;
    ss << config;

    d << QString::fromStdString(ss.str());

    return d;
}

QDebug operator<<(QDebug d, speech_service::state_t state_value) {
    switch (state_value) {
        case speech_service::state_t::busy:
            d << "busy";
            break;
        case speech_service::state_t::idle:
            d << "idle";
            break;
        case speech_service::state_t::listening_manual:
            d << "listening-manual";
            break;
        case speech_service::state_t::listening_auto:
            d << "listening-auto";
            break;
        case speech_service::state_t::not_configured:
            d << "not-configured";
            break;
        case speech_service::state_t::transcribing_file:
            d << "transcribing-file";
            break;
        case speech_service::state_t::listening_single_sentence:
            d << "listening-single-sentence";
            break;
        case speech_service::state_t::playing_speech:
            d << "playing-speech";
            break;
        case speech_service::state_t::writing_speech_to_file:
            d << "writing-speech-to-file";
            break;
        case speech_service::state_t::translating:
            d << "translating";
            break;
        case speech_service::state_t::unknown:
            d << "unknown";
            break;
    }

    return d;
}

speech_service::speech_service(QObject *parent)
    : QObject{parent}, m_dbus_service_adaptor{this} {
    qDebug() << "starting service:" << settings::instance()->launch_mode();

    connect(this, &speech_service::models_changed, this,
            &speech_service::refresh_status);
    connect(models_manager::instance(), &models_manager::models_changed, this,
            &speech_service::handle_models_changed);
    connect(models_manager::instance(), &models_manager::busy_changed, this,
            &speech_service::refresh_status);
    connect(models_manager::instance(), &models_manager::download_progress,
            this, [this](const QString &id, double progress) {
                emit model_download_progress(id, progress);
            });
    connect(this, &speech_service::stt_text_decoded, this,
            static_cast<void (speech_service::*)(const QString &,
                                                 const QString &, int)>(
                &speech_service::handle_stt_text_decoded),
            Qt::QueuedConnection);
    connect(this, &speech_service::sentence_timeout, this,
            static_cast<void (speech_service::*)(int)>(
                &speech_service::handle_stt_sentence_timeout),
            Qt::QueuedConnection);
    connect(this, &speech_service::stt_engine_eof, this,
            static_cast<void (speech_service::*)(int)>(
                &speech_service::handle_stt_engine_eof),
            Qt::QueuedConnection);
    connect(this, &speech_service::stt_engine_error, this,
            static_cast<void (speech_service::*)(int)>(
                &speech_service::handle_stt_engine_error),
            Qt::QueuedConnection);
    connect(this, &speech_service::tts_engine_error, this,
            static_cast<void (speech_service::*)(int)>(
                &speech_service::handle_tts_engine_error),
            Qt::QueuedConnection);
    connect(this, &speech_service::mnt_engine_error, this,
            static_cast<void (speech_service::*)(int)>(
                &speech_service::handle_mnt_engine_error),
            Qt::QueuedConnection);
    connect(this, &speech_service::mnt_engine_state_changed, this,
            &speech_service::handle_mnt_engine_state_changed,
            Qt::QueuedConnection);
    connect(this, &speech_service::tts_speech_encoded, this,
            static_cast<void (speech_service::*)(const tts_partial_result_t &)>(
                &speech_service::handle_tts_speech_encoded),
            Qt::QueuedConnection);
    connect(this, &speech_service::stt_engine_shutdown, this,
            [this] { stop_stt_engine(); });
    connect(
        this, &speech_service::requet_update_task_state, this,
        [this] { update_task_state(); }, Qt::QueuedConnection);
    connect(&m_player, &QMediaPlayer::stateChanged, this,
            &speech_service::handle_player_state_changed, Qt::QueuedConnection);
    connect(
        settings::instance(), &settings::default_stt_model_changed, this,
        [this]() {
            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::service) {
                auto model = default_stt_model();
                qDebug() << "[service => dbus] signal "
                            "DefaultSttModelPropertyChanged:"
                         << model;
                emit DefaultSttModelPropertyChanged(model);
            }

            emit default_stt_model_changed();

            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::service) {
                auto lang = default_stt_lang();
                qDebug()
                    << "[service => dbus] signal DefaultSttLangPropertyChanged:"
                    << lang;
                emit DefaultSttLangPropertyChanged(lang);
            }

            emit default_stt_lang_changed();
        },
        Qt::QueuedConnection);
    connect(
        settings::instance(), &settings::default_tts_model_changed, this,
        [this]() {
            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::service) {
                auto model = default_tts_model();
                qDebug() << "[service => dbus] signal "
                            "DefaultTtsModelPropertyChanged:"
                         << model;
                emit DefaultTtsModelPropertyChanged(model);
            }

            emit default_tts_model_changed();

            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::service) {
                auto lang = default_tts_lang();
                qDebug()
                    << "[service => dbus] signal DefaultTtsLangPropertyChanged:"
                    << lang;
                emit DefaultTtsLangPropertyChanged(lang);
            }

            emit default_tts_lang_changed();
        },
        Qt::QueuedConnection);
    connect(
        settings::instance(), &settings::default_mnt_lang_changed, this,
        [this]() {
            qDebug() << "default_mnt_lang_changed:"
                     << settings::instance()->default_mnt_lang();
            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::service) {
                auto lang = default_mnt_lang();
                qDebug() << "[service => dbus] signal "
                            "DefaultMntLangPropertyChanged:"
                         << lang;
                emit DefaultMntLangPropertyChanged(lang);
            }

            emit default_mnt_lang_changed();

            settings::instance()->set_default_mnt_out_lang(
                default_mnt_out_lang());
        },
        Qt::QueuedConnection);
    connect(
        settings::instance(), &settings::default_mnt_out_lang_changed, this,
        [this]() {
            if (settings::instance()->launch_mode() ==
                settings::launch_mode_t::service) {
                auto lang = default_mnt_out_lang();
                qDebug() << "[service => dbus] signal "
                            "DefaultMntOutLangPropertyChanged:"
                         << lang;
                emit DefaultMntOutLangPropertyChanged(lang);
            }

            emit default_mnt_out_lang_changed();
        },
        Qt::QueuedConnection);

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::service) {
        connect(
            this, &speech_service::state_changed, this,
            [this]() {
                qDebug() << "[service => dbus] signal StatePropertyChanged:"
                         << dbus_state();
                emit StatePropertyChanged(dbus_state());
            },
            Qt::QueuedConnection);
        connect(
            this, &speech_service::current_task_changed, this,
            [this]() {
                qDebug()
                    << "[service => dbus] signal CurrentTaskPropertyChanged:"
                    << current_task_id();
                emit CurrentTaskPropertyChanged(current_task_id());
            },
            Qt::QueuedConnection);
        connect(this, &speech_service::error, this, [this](error_t type) {
            qDebug() << "[service => dbus] signal ErrorOccured:"
                     << static_cast<int>(type);
            emit ErrorOccured(static_cast<int>(type));
        });
        connect(this, &speech_service::stt_file_transcribe_finished, this,
                [this](int task) {
                    qDebug()
                        << "[service => dbus] signal SttFileTranscribeFinished:"
                        << task;
                    emit SttFileTranscribeFinished(task);
                });
        connect(
            this, &speech_service::stt_intermediate_text_decoded, this,
            [this](const QString &text, const QString &lang, int task) {
                qDebug()
                    << "[service => dbus] signal SttIntermediateTextDecoded:"
                    << lang << task;
                emit SttIntermediateTextDecoded(text, lang, task);
            });
        connect(this, &speech_service::stt_text_decoded, this,
                [this](const QString &text, const QString &lang, int task) {
                    qDebug()
                        << "[service => dbus] signal SttTextDecoded:" << lang
                        << task;
                    emit SttTextDecoded(text, lang, task);
                });
        connect(this, &speech_service::tts_partial_speech_playing, this,
                [this](const QString &text, int task) {
                    qDebug()
                        << "[service => dbus] signal TtsPartialSpeechPlaying:"
                        << task;
                    emit TtsPartialSpeechPlaying(text, task);
                });
        connect(this, &speech_service::tts_speech_to_file_finished, this,
                [this](const QString &file, int task) {
                    qDebug()
                        << "[service => dbus] signal TtsSpeechToFileFinished:"
                        << task;
                    emit TtsSpeechToFileFinished(file, task);
                });
        connect(this, &speech_service::tts_speech_to_file_progress_changed,
                this, [this](double progress, int task) {
                    qDebug()
                        << "[service => dbus] signal TtsSpeechToFileProgress:"
                        << progress << task;
                    emit TtsSpeechToFileProgress(progress, task);
                });
        connect(
            this, &speech_service::mnt_translate_finished, this,
            [this](const QString &in_text, const QString &in_lang,
                   const QString &out_text, const QString &out_lang, int task) {
                qDebug() << "[service => dbus] signal MntTranslateFinished:"
                         << task;
                emit MntTranslateFinished(in_text, in_lang, out_text, out_lang,
                                          task);
            });
        connect(
            this, &speech_service::task_state_changed, this,
            [this]() {
                qDebug() << "[service => dbus] signal TaskStatePropertyChanged:"
                         << task_state();
                emit TaskStatePropertyChanged(task_state());
            },
            Qt::QueuedConnection);
        connect(this, &speech_service::models_changed, this, [this]() {
            // stt

            auto models_list = available_stt_models();
            qDebug() << "[service => dbus] signal SttModelsPropertyChanged:"
                     << models_list;
            emit SttModelsPropertyChanged(models_list);

            auto langs_list = available_stt_langs();
            qDebug() << "[service => dbus] signal SttLangsPropertyChanged:"
                     << langs_list;
            emit SttLangsPropertyChanged(langs_list);

            auto lang_list = available_stt_lang_list();
            qDebug() << "[service => dbus] signal SttLangListPropertyChanged:"
                     << lang_list;
            emit SttLangListPropertyChanged(lang_list);

            // tts

            models_list = available_tts_models();
            qDebug() << "[service => dbus] signal TtsModelsPropertyChanged:"
                     << models_list;
            emit TtsModelsPropertyChanged(models_list);

            langs_list = available_tts_langs();
            qDebug() << "[service => dbus] signal TtsLangsPropertyChanged:"
                     << langs_list;
            emit TtsLangsPropertyChanged(langs_list);

            lang_list = available_tts_lang_list();
            qDebug() << "[service => dbus] signal TtsLangListPropertyChanged:"
                     << lang_list;
            emit TtsLangListPropertyChanged(lang_list);

            // stt + tts

            lang_list = available_stt_tts_lang_list();
            qDebug()
                << "[service => dbus] signal SttTtsLangListPropertyChanged:"
                << lang_list;
            emit SttTtsLangListPropertyChanged(lang_list);

            // ttt

            models_list = available_ttt_models();
            qDebug() << "[service => dbus] signal TttModelsPropertyChanged:"
                     << models_list;
            emit TttModelsPropertyChanged(models_list);

            langs_list = available_ttt_langs();
            qDebug() << "[service => dbus] signal TttLangsPropertyChanged:"
                     << langs_list;
            emit TttLangsPropertyChanged(langs_list);

            // mnt
            
            langs_list = available_mnt_langs();
            qDebug() << "[service => dbus] signal MntLangsPropertyChanged:"
                     << langs_list;
            emit MntLangsPropertyChanged(langs_list);

            lang_list = available_mnt_lang_list();
            qDebug() << "[service => dbus] signal MntLangListPropertyChanged:"
                     << lang_list;
            emit MntLangListPropertyChanged(lang_list);
        });
        connect(this, &speech_service::stt_transcribe_file_progress_changed,
                this, [this](double progress, int task) {
                    qDebug()
                        << "[service => dbus] signal SttFileTranscribeProgress:"
                        << progress << task;
                    emit SttFileTranscribeProgress(progress, task);
                });
        connect(this, &speech_service::tts_play_speech_finished, this,
                [this](int task) {
                    qDebug()
                        << "[service => dbus] signal TtsPlaySpeechFinished:"
                        << task;
                    emit TtsPlaySpeechFinished(task);
                });

        m_keepalive_timer.setSingleShot(true);
        m_keepalive_timer.setTimerType(Qt::VeryCoarseTimer);
        m_keepalive_timer.setInterval(KEEPALIVE_TIME);
        connect(&m_keepalive_timer, &QTimer::timeout, this,
                &speech_service::handle_keepalive_timeout);

        m_keepalive_current_task_timer.setSingleShot(true);
        m_keepalive_current_task_timer.setTimerType(Qt::VeryCoarseTimer);
        m_keepalive_current_task_timer.setInterval(KEEPALIVE_TASK_TIME);
        connect(&m_keepalive_current_task_timer, &QTimer::timeout, this,
                &speech_service::handle_task_timeout);

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

        emit current_task_changed();
        emit task_state_changed();
        emit state_changed();
    }

    if (settings::instance()->launch_mode() ==
        settings::launch_mode_t::service) {
        m_keepalive_timer.start();
    }

    setup_modules();

    remove_cached_wavs();

    handle_models_changed();
}

speech_service::~speech_service() { qDebug() << "speech service dtor"; }

speech_service::source_t speech_service::audio_source_type() const {
    if (!m_source) return source_t::none;
    if (m_source->type() == audio_source::source_type::mic)
        return source_t::mic;
    if (m_source->type() == audio_source::source_type::file)
        return source_t::file;
    return source_t::none;
}

void speech_service::fill_available_models_map(
    const std::vector<models_manager::model_t> &models) {
    lang_to_model_map_t lang_to_model_map;

    m_available_stt_models_map.clear();
    m_available_tts_models_map.clear();
    m_available_ttt_models_map.clear();
    m_available_mnt_models_map.clear();

    auto default_stt_model = settings::instance()->default_stt_model();
    auto default_tts_model = settings::instance()->default_tts_model();
    auto default_mnt_lang = settings::instance()->default_mnt_lang();

    for (const auto &model : models) {
        auto role = models_manager::role_of_engine(model.engine);

        switch (role) {
            case models_manager::model_role::stt:
                m_available_stt_models_map.emplace(
                    model.id,
                    model_data_t{model.id, model.lang_id, model.trg_lang_id,
                                 model.engine, model.name});
                lang_to_model_map.stt.emplace(model.lang_id, model.id);
                if (model.id == default_stt_model)
                    lang_to_model_map.found_default_stt = true;
                break;
            case models_manager::model_role::ttt:
                m_available_ttt_models_map.emplace(
                    model.id,
                    model_data_t{model.id, model.lang_id, model.trg_lang_id,
                                 model.engine, model.name});
                break;
            case models_manager::model_role::tts:
                m_available_tts_models_map.emplace(
                    model.id,
                    model_data_t{model.id, model.lang_id, model.trg_lang_id,
                                 model.engine, model.name});
                lang_to_model_map.tts.emplace(model.lang_id, model.id);
                if (model.id == default_tts_model)
                    lang_to_model_map.found_default_tts = true;
                break;
            case models_manager::model_role::mnt:
                m_available_mnt_models_map.emplace(
                    model.id,
                    model_data_t{model.id, model.lang_id, model.trg_lang_id,
                                 model.engine, model.name});
                lang_to_model_map.mnt.emplace(model.lang_id, model.id);
                if (model.lang_id == default_mnt_lang)
                    lang_to_model_map.found_default_mnt = true;
                break;
        }
    }

    if (!lang_to_model_map.found_default_stt) {
        qDebug() << "default stt model not found:" << default_stt_model;
        QString lang;

        if (!default_stt_model.contains('_'))
            lang = default_stt_model.split('_').first();

        if (auto it = lang_to_model_map.stt.find(lang);
            it != lang_to_model_map.stt.end()) {
            default_stt_model = it->second;
            qDebug() << "new default stt model by lang:" << default_stt_model;
        } else if (auto it = lang_to_model_map.stt.find("en");
                   it != lang_to_model_map.stt.end()) {
            default_stt_model = it->second;
            qDebug() << "new default stt model by en:" << default_stt_model;
        } else if (!lang_to_model_map.stt.empty()) {
            default_stt_model = lang_to_model_map.stt.cbegin()->second;
            qDebug() << "new default stt model by first:" << default_stt_model;
        }

        settings::instance()->set_default_stt_model(default_stt_model);
    }

    if (!lang_to_model_map.found_default_tts) {
        qDebug() << "default tts model not found:" << default_tts_model;
        QString lang;

        if (!default_tts_model.contains('_'))
            lang = default_tts_model.split('_').first();

        if (auto it = lang_to_model_map.tts.find(lang);
            it != lang_to_model_map.tts.end()) {
            default_tts_model = it->second;
            qDebug() << "new default tts model by lang:" << default_tts_model;
        } else if (auto it = lang_to_model_map.tts.find(QStringLiteral("en"));
                   it != lang_to_model_map.tts.end()) {
            default_tts_model = it->second;
            qDebug() << "new default tts model by en:" << default_tts_model;
        } else if (!lang_to_model_map.tts.empty()) {
            default_tts_model = lang_to_model_map.tts.cbegin()->second;
            qDebug() << "new default tts model by first:" << default_tts_model;
        }

        settings::instance()->set_default_tts_model(default_tts_model);
    }

    if (!lang_to_model_map.found_default_mnt) {
        qDebug() << "default mnt lang not found:" << default_mnt_lang;

        if (lang_to_model_map.mnt.count(QStringLiteral("en")) != 0) {
            default_mnt_lang = "en";
            qDebug() << "new default mnt lang by en:" << default_mnt_lang;
        } else if (!lang_to_model_map.mnt.empty()) {
            default_mnt_lang = lang_to_model_map.mnt.cbegin()->first;
            qDebug() << "new default mnt lang by first:" << default_mnt_lang;
        }

        qDebug() << "new default mnt lang:" << default_mnt_lang;
        settings::instance()->set_default_mnt_lang(default_mnt_lang);
    }

    m_available_mnt_lang_to_model_id_map = std::move(lang_to_model_map.mnt);
}

bool speech_service::matched_engine_type(engine_t engine_type,
                                         models_manager::model_engine engine) {
    auto role = models_manager::role_of_engine(engine);
    if (engine_type == engine_t::stt && role == models_manager::model_role::stt)
        return true;
    if (engine_type == engine_t::tts && role == models_manager::model_role::tts)
        return true;
    if (engine_type == engine_t::mnt && role == models_manager::model_role::mnt)
        return true;
    return false;
}

std::optional<speech_service::model_config_t>
speech_service::choose_model_config_by_id(
    const std::vector<models_manager::model_t> &models, engine_t engine_type,
    const QString &model_or_lang_id, const QString &out_lang_id) {
    std::optional<speech_service::model_config_t> config;

    auto it =
        std::find_if(models.cbegin(), models.cend(), [&](const auto &model) {
            if (!matched_engine_type(engine_type, model.engine)) return false;
            return engine_type == engine_t::mnt
                       ? (model_or_lang_id == model.lang_id &&
                          out_lang_id == model.trg_lang_id)
                       : model_or_lang_id == model.id;
        });

    if (it != models.cend()) {
        const auto model = *it;
        config.emplace();
        switch (engine_type) {
            case engine_t::stt:
                config->stt = stt_model_config_t{
                    model.lang_id,    model.id,          model.engine,
                    model.model_file, model.scorer_file, /*ttt=*/{}};
                break;
            case engine_t::tts:
                config->tts =
                    tts_model_config_t{model.lang_id,       model.id,
                                       model.engine,        model.model_file,
                                       /*vocoder_file=*/{}, model.speaker};
                break;
            case engine_t::mnt:
                config->mnt = mnt_model_config_t{
                    model.lang_id, model.id, model.model_file,
                    /*model_id_second=*/{}, /*model_file_second=*/{}};
                break;
        }
    }

    if (!config && engine_type == engine_t::mnt && model_or_lang_id != "en" &&
        out_lang_id != "en") {
        // search for lang_id=>en and en=>out_lang_id

        auto it_model_to_en = std::find_if(
            models.cbegin(), models.cend(), [&](const auto &model) {
                if (models_manager::role_of_engine(model.engine) !=
                    models_manager::model_role::mnt)
                    return false;
                return model_or_lang_id == model.lang_id &&
                       model.trg_lang_id == "en";
            });

        if (it_model_to_en != models.cend()) {
            auto it_model_from_en = std::find_if(
                models.cbegin(), models.cend(), [&](const auto &model) {
                    if (models_manager::role_of_engine(model.engine) !=
                        models_manager::model_role::mnt)
                        return false;
                    return model.lang_id == "en" &&
                           model.trg_lang_id == out_lang_id;
                });

            if (it_model_from_en != models.cend()) {
                const auto model_first = *it_model_to_en;
                const auto model_second = *it_model_from_en;
                config.emplace();
                config->mnt = mnt_model_config_t{
                    model_first.lang_id, model_first.id, model_first.model_file,
                    model_second.id, model_second.model_file};
                qDebug() << "mnt both models found";
            }
        }
    }

    return config;
}

std::optional<speech_service::model_config_t>
speech_service::choose_model_config_by_lang(
    const std::vector<models_manager::model_t> &models, engine_t engine_type,
    const QString &model_or_lang_id) {
    if (engine_type != engine_t::stt && engine_type != engine_t::tts)
        throw std::runtime_error("invalid engine type");

    std::optional<speech_service::model_config_t> config;

    int best_score = -1;
    const models_manager::model_t *best_model = nullptr;

    for (const auto &model : models) {
        if (!matched_engine_type(engine_type, model.engine)) continue;
        if (model_or_lang_id == model.lang_id) {
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
        config.emplace();
        switch (engine_type) {
            case engine_t::stt:
                config->stt = stt_model_config_t{
                    best_model->lang_id,     best_model->id,
                    best_model->engine,      best_model->model_file,
                    best_model->scorer_file, /*ttt=*/{}};
                break;
            case engine_t::tts:
                config->tts = tts_model_config_t{
                    best_model->lang_id, best_model->id,
                    best_model->engine,  best_model->model_file,
                    /*vocoder_file=*/{}, best_model->speaker};
                break;
            case engine_t::mnt:
                break;
        }
    }

    return config;
}

std::optional<speech_service::model_config_t>
speech_service::choose_model_config_by_first(
    const std::vector<models_manager::model_t> &models, engine_t engine_type) {
    if (engine_type != engine_t::stt && engine_type != engine_t::tts)
        throw std::runtime_error("invalid engine type");

    std::optional<speech_service::model_config_t> config;

    auto it =
        std::find_if(models.cbegin(), models.cend(), [&](const auto &model) {
            return matched_engine_type(engine_type, model.engine);
        });

    if (it != models.cend()) {
        const auto model = *it;
        config.emplace();
        switch (engine_type) {
            case engine_t::stt:
                config->stt = stt_model_config_t{
                    model.lang_id,    model.id,          model.engine,
                    model.model_file, model.scorer_file, /*ttt=*/{}};
                break;
            case engine_t::tts:
                config->tts =
                    tts_model_config_t{model.lang_id,       model.id,
                                       model.engine,        model.model_file,
                                       /*vocoder_file=*/{}, model.speaker};
                break;
            case engine_t::mnt:
                break;
        }
    }

    return config;
}

std::optional<speech_service::model_config_t>
speech_service::choose_model_config(engine_t engine_type,
                                    QString model_or_lang_id,
                                    QString out_lang_id) {
    auto models = models_manager::instance()->available_models();

    fill_available_models_map(models);

    if (models.empty()) {
        qDebug() << "no models available";
        return std::nullopt;
    }

    if (model_or_lang_id.isEmpty()) {
        switch (engine_type) {
            case engine_t::stt:
                model_or_lang_id = settings::instance()->default_stt_model();
                break;
            case engine_t::tts:
                model_or_lang_id = settings::instance()->default_tts_model();
                break;
            case engine_t::mnt:
                model_or_lang_id = settings::instance()->default_mnt_lang();
                break;
        }
    }

    if (out_lang_id.isEmpty()) {
        out_lang_id = settings::instance()->default_mnt_out_lang();
    }

    model_or_lang_id = model_or_lang_id.toLower();
    out_lang_id = out_lang_id.toLower();

    qDebug() << "choosing model for id:" << model_or_lang_id << out_lang_id;

    auto active_config = choose_model_config_by_id(
        models, engine_type, model_or_lang_id, out_lang_id);

    if (!active_config && engine_type == engine_t::mnt) {
        qDebug() << "no mnt model for lang available";
        return std::nullopt;
    }

    if (!active_config) {
        active_config =
            choose_model_config_by_lang(models, engine_type, model_or_lang_id);
    }

    if (!active_config) {
        active_config = choose_model_config_by_first(models, engine_type);

        if (active_config) {
            QString model_id;
            switch (engine_type) {
                case engine_t::stt:
                    model_id = active_config->stt->model_id;
                    break;
                case engine_t::tts:
                    model_id = active_config->tts->model_id;
                case engine_t::mnt:
                    break;
            }
            qWarning() << "cannot find requested model, choosing:" << model_id;
        }
    }

    if (!active_config) {
        qWarning() << "cannot find any model";
        return active_config;
    }

    // search for ttt model for stt lang
    // only when restore punctuation is enabled
    if (engine_type == engine_t::stt && active_config->stt &&
        settings::instance()->restore_punctuation()) {
        auto it = std::find_if(
            models.cbegin(), models.cend(), [&](const auto &model) {
                if (models_manager::role_of_engine(model.engine) !=
                    models_manager::model_role::ttt)
                    return false;
                return model.lang_id == active_config->stt->lang_id;
            });

        if (it != models.cend()) {
            qDebug() << "found ttt model for stt:" << it->id;

            active_config->stt->ttt =
                ttt_model_config_t{it->id, it->model_file};
        }
    }

    return active_config;
}

void speech_service::handle_models_changed() {
    fill_available_models_map(models_manager::instance()->available_models());

    if (m_current_task &&
        (m_available_stt_models_map.find(m_current_task->model_id) ==
             m_available_stt_models_map.end() ||
         m_available_tts_models_map.find(m_current_task->model_id) ==
             m_available_tts_models_map.end() ||
         m_available_mnt_models_map.find(m_current_task->model_id) ==
             m_available_mnt_models_map.end())) {
        stop_stt();
    }

    emit models_changed();
}

QString speech_service::lang_from_model_id(const QString &model_id) {
    auto l = model_id.split('_');

    if (l.empty()) {
        qDebug() << "invalid model id:" << model_id;
        return {};
    }

    return l.first();
}

QString speech_service::restart_stt_engine(speech_mode_t speech_mode,
                                           const QString &model_id,
                                           bool translate) {
    auto model_files = choose_model_config(engine_t::stt, model_id);
    if (model_files && model_files->stt) {
        stt_engine::config_t config;

        config.model_files.model_file =
            model_files->stt->model_file.toStdString();
        config.model_files.scorer_file =
            model_files->stt->scorer_file.toStdString();
        if (model_files->stt->ttt)
            config.model_files.ttt_model_file =
                model_files->stt->ttt->model_file.toStdString();
        config.lang = model_files->stt->lang_id.toStdString();
        config.speech_mode =
            static_cast<stt_engine::speech_mode_t>(speech_mode);
        config.translate = translate;

        bool new_engine_required = [&] {
            if (!m_stt_engine) return true;
            if (translate != m_stt_engine->translate()) return true;

            const auto &type = typeid(*m_stt_engine);
            if (model_files->stt->engine ==
                    models_manager::model_engine::stt_ds &&
                type != typeid(ds_engine))
                return true;
            if (model_files->stt->engine ==
                    models_manager::model_engine::stt_vosk &&
                type != typeid(vosk_engine))
                return true;
            if (model_files->stt->engine ==
                    models_manager::model_engine::stt_whisper &&
                type != typeid(whisper_engine))
                return true;

            if (m_stt_engine->model_files() != config.model_files) return true;
            if (m_stt_engine->lang() != config.lang) return true;

            return false;
        }();

        qDebug() << "restart stt engine config:" << config;

        if (new_engine_required) {
            qDebug() << "new stt engine required";

            if (m_stt_engine) {
                m_stt_engine.reset();
                qDebug() << "stt engine destroyed successfully";
            }

            stt_engine::callbacks_t call_backs{
                /*text_decoded=*/[this](const std::string &text) {
                    handle_stt_text_decoded(text);
                },
                /*intermediate_text_decoded=*/
                [this](const std::string &text) {
                    handle_stt_intermediate_text_decoded(text);
                },
                /*speech_detection_status_changed=*/
                [this](stt_engine::speech_detection_status_t status) {
                    handle_stt_speech_detection_status_changed(status);
                },
                /*sentence_timeout=*/
                [this]() { handle_stt_sentence_timeout(); },
                /*eof=*/
                [this]() { handle_stt_engine_eof(); },
                /*stopped=*/
                [this]() { handle_stt_engine_error(); }};

            try {
                switch (model_files->stt->engine) {
                    case models_manager::model_engine::stt_ds:
                        m_stt_engine = std::make_unique<ds_engine>(
                            std::move(config), std::move(call_backs));
                        break;
                    case models_manager::model_engine::stt_vosk:
                        m_stt_engine = std::make_unique<vosk_engine>(
                            std::move(config), std::move(call_backs));
                        break;
                    case models_manager::model_engine::stt_whisper:
                        m_stt_engine = std::make_unique<whisper_engine>(
                            std::move(config), std::move(call_backs));
                        break;
                    case models_manager::model_engine::ttt_hftc:
                    case models_manager::model_engine::tts_coqui:
                    case models_manager::model_engine::tts_piper:
                    case models_manager::model_engine::tts_espeak:
                    case models_manager::model_engine::tts_rhvoice:
                    case models_manager::model_engine::mnt_bergamot:
                        throw std::runtime_error{
                            "invalid model engine, expected stt"};
                }
            } catch (const std::runtime_error &error) {
                qWarning() << "failed to create stt engine:" << error.what();
                return {};
            }

            m_stt_engine->start();
        } else {
            qDebug() << "new stt engine not required, only restart";
            m_stt_engine->stop();
            m_stt_engine->start();
            m_stt_engine->set_speech_mode(
                static_cast<stt_engine::speech_mode_t>(speech_mode));
        }

        return model_files->stt->model_id;
    }

    qWarning() << "failed to restart stt engine, no valid model";
    return {};
}

QString speech_service::restart_tts_engine(const QString &model_id) {
    auto model_config = choose_model_config(engine_t::tts, model_id);
    if (model_config && model_config->tts) {
        tts_engine::config_t config;

        config.model_files.model_path =
            model_config->tts->model_file.toStdString();
        config.model_files.vocoder_path =
            model_config->tts->vocoder_file.toStdString();
        config.lang = model_config->tts->lang_id.toStdString();
        config.cache_dir = settings::instance()->cache_dir().toStdString();
        config.speaker = model_config->tts->speaker.toStdString();

        QFile nb_file{QStringLiteral(":/nonbreaking_prefixes/%1.txt")
                          .arg(model_config->tts->lang_id.split('-').first())};
        if (nb_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            config.nb_data = nb_file.readAll().toStdString();
        } else {  // fallback to en
            QFile nb_file_en{QStringLiteral(":/nonbreaking_prefixes/en.txt")};
            if (nb_file_en.open(QIODevice::ReadOnly | QIODevice::Text)) {
                config.nb_data = nb_file_en.readAll().toStdString();
            }
        }

        bool new_engine_required = [&] {
            if (!m_tts_engine) return true;

            const auto &type = typeid(*m_tts_engine);
            if (model_config->tts->engine ==
                    models_manager::model_engine::tts_coqui &&
                type != typeid(coqui_engine))
                return true;
            if (model_config->tts->engine ==
                    models_manager::model_engine::tts_piper &&
                type != typeid(piper_engine))
                return true;

            if (m_tts_engine->model_files() != config.model_files) return true;
            if (m_tts_engine->lang() != config.lang) return true;
            if (m_tts_engine->speaker() != config.speaker) return true;

            return false;
        }();

        qDebug() << "restart tts engine config:" << config;

        if (new_engine_required) {
            qDebug() << "new tts engine required";

            if (m_tts_engine) {
                m_tts_engine.reset();
                qDebug() << "tts engine destroyed successfully";
            }

            tts_engine::callbacks_t call_backs{
                /*speech_encoded=*/[this](const std::string &text,
                                          const std::string &wav_file_path,
                                          bool last) {
                    handle_tts_speech_encoded(text, wav_file_path, last);
                },
                /*state_changed=*/
                [this](tts_engine::state_t state) {
                    handle_tts_engine_state_changed(state);
                },
                /*error=*/
                [this]() { handle_tts_engine_error(); }};

            try {
                switch (model_config->tts->engine) {
                    case models_manager::model_engine::tts_coqui:
                        m_tts_engine = std::make_unique<coqui_engine>(
                            std::move(config), std::move(call_backs));
                        break;
                    case models_manager::model_engine::tts_piper:
                        config.data_dir =
                            module_tools::unpacked_dir("espeakdata")
                                .toStdString();
                        m_tts_engine = std::make_unique<piper_engine>(
                            std::move(config), std::move(call_backs));
                        break;
                    case models_manager::model_engine::tts_espeak:
                        config.data_dir =
                            module_tools::unpacked_dir("espeakdata")
                                .toStdString();
                        m_tts_engine = std::make_unique<espeak_engine>(
                            std::move(config), std::move(call_backs));
                        break;
                    case models_manager::model_engine::tts_rhvoice:
                        config.data_dir =
                            module_tools::unpacked_dir("rhvoicedata")
                                .toStdString();
                        config.config_dir =
                            module_tools::unpacked_dir("rhvoiceconfig")
                                .toStdString();
                        m_tts_engine = std::make_unique<rhvoice_engine>(
                            std::move(config), std::move(call_backs));
                        break;
                    case models_manager::model_engine::ttt_hftc:
                    case models_manager::model_engine::stt_ds:
                    case models_manager::model_engine::stt_vosk:
                    case models_manager::model_engine::stt_whisper:
                    case models_manager::model_engine::mnt_bergamot:
                        throw std::runtime_error{
                            "invalid model engine, expected tts"};
                }
            } catch (const std::runtime_error &error) {
                qWarning() << "failed to create tts engine:" << error.what();
                return {};
            }
        } else {
            qDebug() << "new tts engine not required";
        }

        m_tts_engine->start();

        return model_config->tts->model_id;
    }

    qWarning() << "failed to restart tts engine, no valid model";
    return {};
}

QString speech_service::restart_mnt_engine(const QString &model_or_lang_id,
                                           const QString &out_lang_id) {
    auto model_config =
        choose_model_config(engine_t::mnt, model_or_lang_id, out_lang_id);
    if (model_config && model_config->mnt) {
        mnt_engine::config_t config;

        config.model_files.model_path_first =
            model_config->mnt->model_file_first.toStdString();
        config.model_files.model_path_second =
            model_config->mnt->model_file_second.toStdString();
        config.lang = model_config->mnt->lang_id.toStdString();

        QFile nb_file{QStringLiteral(":/nonbreaking_prefixes/%1.txt")
                          .arg(model_config->mnt->lang_id.split('-').first())};
        if (nb_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            config.nb_data = nb_file.readAll().toStdString();
        } else {  // fallback to en
            QFile nb_file_en{QStringLiteral(":/nonbreaking_prefixes/en.txt")};
            if (nb_file_en.open(QIODevice::ReadOnly | QIODevice::Text)) {
                config.nb_data = nb_file_en.readAll().toStdString();
            }
        }

        bool new_engine_required = [&] {
            if (!m_mnt_engine) return true;
            if (m_mnt_engine->model_files() != config.model_files) return true;
            if (m_mnt_engine->lang() != config.lang) return true;

            return false;
        }();

        qDebug() << "restart mnt engine config:" << config;

        if (new_engine_required) {
            qDebug() << "new mnt engine required";

            if (m_mnt_engine) {
                m_tts_engine.reset();
                qDebug() << "mnt engine destroyed successfully";
            }

            mnt_engine::callbacks_t call_backs{
                /*text_translated=*/
                [this](const std::string &in_text, const std::string &in_lang,
                       std::string &&out_text, const std::string &out_lang) {
                    handle_mnt_translate_finished(
                        in_text, in_lang, std::move(out_text), out_lang);
                },
                /*state_changed=*/
                [this](mnt_engine::state_t state) {
                    if (m_current_task) {
                        qDebug() << "mnt_engine_state_changed:"
                                 << static_cast<int>(state);
                        emit mnt_engine_state_changed(state,
                                                      m_current_task->id);
                    }
                },
                /*error=*/
                [this]() { handle_mnt_engine_error(); }};

            try {
                m_mnt_engine = std::make_unique<mnt_engine>(
                    std::move(config), std::move(call_backs));
            } catch (const std::runtime_error &error) {
                qWarning() << "failed to create mnt engine:" << error.what();
                return {};
            }
        } else {
            qDebug() << "new mnt engine not required";
        }

        m_mnt_engine->start();

        return model_config->mnt->model_id_first;
    }

    qWarning() << "failed to restart mnt engine, no valid model";
    return {};
}

void speech_service::handle_stt_intermediate_text_decoded(
    const std::string &text) {
    if (m_current_task) {
        m_last_intermediate_text_task = m_current_task->id;
        emit stt_intermediate_text_decoded(QString::fromStdString(text),
                                           m_current_task->model_id,
                                           m_current_task->id);
    } else {
        qWarning() << "current task does not exist";
    }
}

void speech_service::handle_stt_text_decoded(const QString &, const QString &,
                                             int task_id) {
    if (m_current_task && m_current_task->id == task_id &&
        m_current_task->speech_mode == speech_mode_t::single_sentence) {
        stt_stop_listen(m_current_task->id);
    }
}

void speech_service::handle_stt_sentence_timeout(int task_id) {
    stt_stop_listen(task_id);
}

void speech_service::handle_stt_sentence_timeout() {
    if (m_current_task &&
        m_current_task->speech_mode == speech_mode_t::single_sentence) {
        emit sentence_timeout(m_current_task->id);
    }
}

void speech_service::handle_stt_engine_eof(int task_id) {
    qDebug() << "engine eof";
    if (audio_source_type() == source_t::file)
        emit stt_file_transcribe_finished(task_id);
    cancel(task_id);
}

void speech_service::handle_stt_engine_eof() {
    if (m_current_task) emit stt_engine_eof(m_current_task->id);
}

void speech_service::handle_stt_engine_error(int task_id) {
    qDebug() << "stt engine error";

    emit error(error_t::stt_engine);

    if (current_task_id() == task_id) {
        cancel(task_id);
        if (m_stt_engine) {
            m_stt_engine.reset();
            qDebug() << "stt engine destroyed successfully";
        }
    }
}

void speech_service::handle_stt_engine_error() {
    if (m_current_task) emit stt_engine_error(m_current_task->id);
}

void speech_service::handle_tts_engine_error(int task_id) {
    qDebug() << "tts engine error";

    emit error(error_t::tts_engine);

    if (current_task_id() == task_id) {
        cancel(task_id);
        if (m_stt_engine) {
            m_stt_engine.reset();
            qDebug() << "tts engine destroyed successfully";
        }
    }
}

void speech_service::handle_tts_engine_state_changed(
    [[maybe_unused]] tts_engine::state_t state) {
    qDebug() << "tts engine state changed";
    emit requet_update_task_state();
}

void speech_service::handle_mnt_engine_state_changed(mnt_engine::state_t state,
                                                     int task_id) {
    qDebug() << "mnt engine state changed:" << task_id;

    if ((state == mnt_engine::state_t::idle ||
         state == mnt_engine::state_t::error) &&
        m_current_task && m_current_task->id == task_id) {
        stop_mnt_engine();
    }

    emit requet_update_task_state();
}

void speech_service::handle_mnt_translate_finished(
    const std::string &in_text, const std::string &in_lang,
    std::string &&out_text, const std::string &out_lang) {
    if (m_current_task) {
        emit mnt_translate_finished(
            QString::fromStdString(in_text), QString::fromStdString(in_lang),
            QString::fromStdString(out_text), QString::fromStdString(out_lang),
            m_current_task->id);
    }
}

void speech_service::handle_tts_speech_encoded(const std::string &text,
                                               const std::string &wav_file_path,
                                               bool last) {
    if (m_current_task) {
        emit tts_speech_encoded({QString::fromStdString(text),
                                 QString::fromStdString(wav_file_path), last,
                                 m_current_task->id});
    }
}

void speech_service::handle_speech_to_file(const tts_partial_result_t &result) {
    if (m_current_task->id != result.task_id) {
        qWarning() << "invalid task:" << result.task_id;
        return;
    }

    m_current_task->counter.value += result.text.size();
    m_current_task->files.push_back(result.wav_file_path);

    qDebug() << "partial speech to file progress:"
             << m_current_task->counter.progress();

    emit tts_speech_to_file_progress_changed(m_current_task->counter.progress(),
                                             result.task_id);

    if (result.last) {
        qDebug() << "speech to file finished";

        auto merged_file = merge_wav_files(m_current_task->files);

        if (merged_file.isEmpty()) return;

        qDebug() << "wav file:" << merged_file;

        emit tts_speech_to_file_finished(merged_file, result.task_id);
        cancel(result.task_id);
    }
}

void speech_service::handle_tts_speech_encoded(
    const tts_partial_result_t &result) {
    if (m_current_task && m_current_task->id == result.task_id) {
        if (m_current_task->speech_mode == speech_mode_t::play_speech) {
            m_tts_queue.push(result);
            handle_tts_queue();
        } else {
            handle_speech_to_file(result);
        }
    } else {
        qWarning() << "unknown task in tts speech encoded";
    }
}

void speech_service::handle_tts_queue() {
    if (m_tts_queue.empty()) return;

    if (m_player.state() == QMediaPlayer::State::PlayingState) return;

    auto &result = m_tts_queue.front();

    m_player.setMedia(QMediaContent{QUrl::fromLocalFile(result.wav_file_path)});

    m_player.play();

    emit tts_partial_speech_playing(result.text, result.task_id);
}

void speech_service::handle_tts_engine_error() {
    if (m_current_task) emit tts_engine_error(m_current_task->id);
}

void speech_service::handle_mnt_engine_error() {
    if (m_current_task) emit mnt_engine_error(m_current_task->id);
}

void speech_service::handle_mnt_engine_error(int task_id) {
    qDebug() << "mnt engine error";

    emit error(error_t::mnt_engine);

    if (current_task_id() == task_id) {
        cancel(task_id);
        if (m_mnt_engine) {
            m_mnt_engine.reset();
            qDebug() << "mnt engine destroyed successfully";
        }
    }
}

void speech_service::handle_stt_text_decoded(const std::string &text) {
    if (m_current_task) {
        if (m_previous_task &&
            m_last_intermediate_text_task == m_previous_task->id) {
            emit stt_text_decoded(QString::fromStdString(text),
                                  m_previous_task->model_id,
                                  m_previous_task->id);
        } else {
            emit stt_text_decoded(QString::fromStdString(text),
                                  m_current_task->model_id, m_current_task->id);
        }
    } else {
        qWarning() << "current task does not exist";
    }

    m_previous_task.reset();
}

void speech_service::handle_stt_speech_detection_status_changed(
    [[maybe_unused]] stt_engine::speech_detection_status_t status) {
    update_task_state();
}

void speech_service::handle_player_state_changed(
    QMediaPlayer::State new_state) {
    qDebug() << "player new state:" << new_state;

    update_task_state();

    if (new_state == QMediaPlayer::State::StoppedState && m_current_task &&
        m_current_task->engine == engine_t::tts && !m_tts_queue.empty()) {
        const auto &result = m_tts_queue.front();

        auto task = result.task_id;

        if (result.last) {
            tts_stop_speech(task);
            if (m_tts_queue.empty()) {
                emit tts_partial_speech_playing("", task);
            } else {
                m_tts_queue.pop();
            }
            emit tts_play_speech_finished(task);
        } else {
            m_tts_queue.pop();
            if (m_tts_queue.empty()) {
                emit tts_partial_speech_playing("", task);
            }
        }

        handle_tts_queue();
    }
}

QVariantMap speech_service::available_models(
    const std::map<QString, model_data_t> &available_models_map) const {
    QVariantMap map;

    std::for_each(available_models_map.cbegin(), available_models_map.cend(),
                  [&map](const auto &p) {
                      map.insert(
                          p.first,
                          QStringList{p.second.model_id,
                                      QStringLiteral("%1 / %2").arg(
                                          p.second.name, p.second.lang_id)});
                  });

    return map;
}

QVariantMap speech_service::available_langs(
    const std::map<QString, model_data_t> &available_models_map) const {
    QVariantMap map;

    auto langs_map = models_manager::instance()->available_langs_map();

    std::for_each(
        available_models_map.cbegin(), available_models_map.cend(),
        [&](const auto &p) {
            const auto &lang_id = p.second.lang_id;
            if (!map.contains(lang_id) && langs_map.count(lang_id) != 0) {
                const auto &lang = langs_map.at(lang_id);
                if (lang_id == QStringLiteral("en")) {
                    map.insert(lang_id,
                               QStringList{p.second.model_id,
                                           QStringLiteral("%1 / %2").arg(
                                               lang.name, lang.id)});
                } else {
                    map.insert(lang_id,
                               QStringList{
                                   p.second.model_id,
                                   QStringLiteral("%1 (%2) / %3")
                                       .arg(lang.name, lang.name_en, lang.id)});
                }
            }
        });

    return map;
}

QVariantMap speech_service::available_trg_langs(
    const std::map<QString, model_data_t> &available_models_map) const {
    QVariantMap map;

    auto langs_map = models_manager::instance()->langs_map();

    std::for_each(
        available_models_map.cbegin(), available_models_map.cend(),
        [&](const auto &p) {
            const auto &lang_id = p.second.trg_lang_id;
            if (!map.contains(lang_id) && langs_map.count(lang_id) != 0) {
                const auto &lang = langs_map.at(lang_id);
                if (lang_id == QStringLiteral("en")) {
                    map.insert(lang_id,
                               QStringList{p.second.model_id,
                                           QStringLiteral("%1 / %2").arg(
                                               lang.name, lang.id)});
                } else {
                    map.insert(lang_id,
                               QStringList{
                                   p.second.model_id,
                                   QStringLiteral("%1 (%2) / %3")
                                       .arg(lang.name, lang.name_en, lang.id)});
                }
            }
        });

    return map;
}

QVariantList speech_service::available_lang_list(
    const std::map<QString, model_data_t> &available_models_map) const {
    QVariantList list;

    auto all_langs = models_manager::instance()->langs();

    std::set<QString> available_langs;

    std::for_each(
        available_models_map.cbegin(), available_models_map.cend(),
        [&](const auto &p) {
            if (available_langs.count(p.second.lang_id) == 0) {
                available_langs.insert(p.second.lang_id);

                auto it = std::find_if(
                    all_langs.cbegin(), all_langs.cend(),
                    [&](const auto &l) { return l.id == p.second.lang_id; });

                if (it == all_langs.cend()) return;

                if (models_manager::role_of_engine(p.second.engine) ==
                    models_manager::model_role::mnt) {
                    list.push_back(
                        QStringList{p.second.lang_id,
                                    QStringLiteral("%1 (%2) / %3")
                                        .arg(it->name, it->name_en, it->id)});
                } else {
                    list.push_back(QStringList{
                        p.second.lang_id,
                        QStringLiteral("%1 / %2").arg(it->name, it->id)});
                }
            }
        });

    return list;
}

std::set<QString> speech_service::available_lang_set(
    const std::map<QString, model_data_t> &available_models_map) const {
    std::set<QString> available_langs;

    std::for_each(available_models_map.cbegin(), available_models_map.cend(),
                  [&](const auto &p) {
                      if (available_langs.count(p.second.lang_id) == 0) {
                          available_langs.insert(p.second.lang_id);
                      }
                  });

    return available_langs;
}

QVariantList speech_service::available_stt_tts_lang_list() const {
    QVariantList list;

    auto stt_langs = available_lang_set(m_available_stt_models_map);

    if (stt_langs.empty()) return list;

    auto tts_langs = available_lang_set(m_available_tts_models_map);

    if (tts_langs.empty()) return list;

    std::vector<QString> stt_tts_langs;
    std::set_intersection(stt_langs.cbegin(), stt_langs.cend(),
                          tts_langs.cbegin(), tts_langs.cend(),
                          std::back_inserter(stt_tts_langs));

    if (stt_langs.empty()) return list;

    auto all_langs = models_manager::instance()->langs();

    list.reserve(stt_tts_langs.size());

    for (auto &id : stt_tts_langs) {
        auto it = std::find_if(all_langs.cbegin(), all_langs.cend(),
                               [&](const auto &l) { return l.id == id; });

        if (it == all_langs.cend()) continue;

        list.push_back(
            QStringList{id, QStringLiteral("%1 / %2").arg(it->name, id)});
    }

    return list;
}

QVariantMap speech_service::available_stt_models() const {
    return available_models(m_available_stt_models_map);
}

QVariantMap speech_service::available_mnt_models() const {
    return available_models(m_available_mnt_models_map);
}

QVariantMap speech_service::available_tts_models() const {
    return available_models(m_available_tts_models_map);
}

QVariantMap speech_service::available_ttt_models() const {
    return available_models(m_available_ttt_models_map);
}

QVariantMap speech_service::available_stt_langs() const {
    return available_langs(m_available_stt_models_map);
}

QVariantList speech_service::available_stt_lang_list() const {
    return available_lang_list(m_available_stt_models_map);
}

QVariantMap speech_service::available_mnt_langs() const {
    return available_langs(m_available_mnt_models_map);
}

QVariantList speech_service::available_mnt_lang_list() const {
    return available_lang_list(m_available_mnt_models_map);
}

QVariantMap speech_service::available_tts_langs() const {
    return available_langs(m_available_tts_models_map);
}

QVariantList speech_service::available_tts_lang_list() const {
    return available_lang_list(m_available_tts_models_map);
}

QVariantMap speech_service::available_ttt_langs() const {
    return available_langs(m_available_ttt_models_map);
}

void speech_service::download_model(const QString &id) {
    models_manager::instance()->download_model(id);
}

void speech_service::delete_model(const QString &id) {
    if (m_current_task && m_current_task->model_id == id) stop_stt();
    models_manager::instance()->delete_model(id);
}

void speech_service::handle_audio_available() {
    if (m_source && m_stt_engine && m_stt_engine->started()) {
        if (m_stt_engine->speech_detection_status() ==
            stt_engine::speech_detection_status_t::initializing) {
            if (m_source->type() == audio_source::source_type::mic)
                m_source->clear();
            return;
        }

        auto [buf, max_size] = m_stt_engine->borrow_buf();

        if (buf) {
            auto audio_data = m_source->read_audio(buf, max_size);

            if (audio_data.eof) qDebug() << "audio eof";

            m_stt_engine->return_buf(buf, audio_data.size, audio_data.sof,
                                     audio_data.eof);
            set_progress(m_source->progress());
        }
    }
}

void speech_service::set_progress(double p) {
    if (audio_source_type() == source_t::file && m_current_task) {
        const auto delta = p - m_progress;
        if (delta > 0.01 || p < 0.0 || p >= 1) {
            m_progress = p;
            emit stt_transcribe_file_progress_changed(m_progress,
                                                      m_current_task->id);
        }
    }
}

double speech_service::stt_transcribe_file_progress(int task) const {
    if (audio_source_type() == source_t::file) {
        if (m_current_task && m_current_task->id == task) {
            return m_progress;
        }
        qWarning() << "invalid task id";
    }

    return -1.0;
}

double speech_service::tts_speech_to_file_progress(int task) const {
    if (m_current_task && m_current_task->id == task &&
        m_current_task->speech_mode == speech_mode_t::speech_to_file) {
        return m_progress;
    }

    qWarning() << "invalid task id";

    return -1.0;
}

QVariantMap speech_service::mnt_out_langs(QString in_lang) const {
    QVariantMap map;

    in_lang = in_lang.toLower();

    std::map<QString, model_data_t> available_models_map;

    bool search_for_en_trg = in_lang != QStringLiteral("en");
    bool found_en_trg = false;

    std::for_each(m_available_mnt_models_map.cbegin(),
                  m_available_mnt_models_map.cend(), [&](const auto &pair) {
                      if (pair.second.lang_id == in_lang) {
                          available_models_map.insert(pair);

                          if (search_for_en_trg && !found_en_trg)
                              found_en_trg = pair.second.trg_lang_id ==
                                             QStringLiteral("en");
                      }
                  });

    if (found_en_trg) {
        std::for_each(m_available_mnt_models_map.cbegin(),
                      m_available_mnt_models_map.cend(), [&](const auto &pair) {
                          if (pair.second.lang_id == QStringLiteral("en") &&
                              pair.second.trg_lang_id != in_lang) {
                              available_models_map.insert(pair);
                          }
                      });
    }

    if (available_models_map.empty()) return map;

    map = available_trg_langs(available_models_map);

    return map;
}

int speech_service::next_task_id() {
    m_last_task_id = (m_last_task_id + 1) % std::numeric_limits<int>::max();
    return m_last_task_id;
}

int speech_service::stt_transcribe_file(const QString &file, QString lang,
                                        bool translate) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot transcribe_file, invalid state";
        return INVALID_TASK;
    }

    if (lang.contains('-')) lang = lang.split('-').first();

    if (m_current_task &&
        m_current_task->speech_mode != speech_mode_t::single_sentence &&
        audio_source_type() == source_t::mic) {
        m_pending_task = m_current_task;
    }

    m_current_task = {
        next_task_id(),
        engine_t::stt,
        restart_stt_engine(speech_mode_t::automatic, lang, translate),
        speech_mode_t::automatic,
        translate,
        {},
        {}};

    if (m_current_task->model_id.isEmpty()) {
        m_current_task.reset();

        qWarning() << "failed to restart engine";

        emit current_task_changed();

        refresh_status();

        return INVALID_TASK;
    }

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

int speech_service::mnt_translate(const QString &text, QString lang,
                                  QString out_lang) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot mnt translate, invalid state";
        return INVALID_TASK;
    }

    if (lang.contains('-')) lang = lang.split('-').first();
    if (lang.contains('-')) out_lang = out_lang.split('-').first();

    if (m_current_task) {
        if (m_current_task->engine == engine_t::stt)
            stt_stop_listen(m_current_task->id);
        else if (m_current_task->engine == engine_t::tts)
            tts_stop_speech(m_current_task->id);
    }

    qDebug() << "mnt translate";

    m_current_task = {next_task_id(),
                      engine_t::mnt,
                      restart_mnt_engine(lang, out_lang),
                      speech_mode_t::translate,
                      false,
                      {},
                      {}};

    if (m_current_task->model_id.isEmpty()) {
        m_current_task.reset();

        qWarning() << "failed to restart engine";

        emit current_task_changed();

        refresh_status();

        return INVALID_TASK;
    }

    if (m_mnt_engine) m_mnt_engine->translate(text.toStdString());

    start_keepalive_current_task();

    emit current_task_changed();

    refresh_status();

    return m_current_task->id;
}

int speech_service::stt_start_listen(speech_mode_t mode, QString lang,
                                     bool translate) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot stt start listen, invalid state";
        return INVALID_TASK;
    }

    if (lang.contains('-')) lang = lang.split('-').first();

    qDebug() << "stt start listen";

    bool set_pending_stt_task =
        m_current_task &&
        ((m_current_task->engine == engine_t::stt &&
          audio_source_type() == source_t::file) ||
         (m_current_task->engine == engine_t::tts &&
          m_current_task->speech_mode == speech_mode_t::speech_to_file));

    if (set_pending_stt_task) {
        qDebug() << "setting pending stt task";
        m_pending_task = {
            next_task_id(), engine_t::stt, lang, mode, translate, {}, {}};
        return m_pending_task->id;
    }

    m_current_task = {next_task_id(),
                      engine_t::stt,
                      restart_stt_engine(mode, lang, translate),
                      mode,
                      translate,
                      {},
                      {}};

    if (m_current_task->model_id.isEmpty()) {
        m_current_task.reset();

        qWarning() << "failed to restart engine";

        emit current_task_changed();

        refresh_status();

        return INVALID_TASK;
    }

    restart_audio_source();
    if (m_stt_engine) m_stt_engine->set_speech_started(true);

    start_keepalive_current_task();

    emit current_task_changed();

    refresh_status();

    return m_current_task->id;
}

int speech_service::tts_play_speech(const QString &text, QString lang) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot tts play speech, invalid state";
        return INVALID_TASK;
    }

    if (lang.contains('-')) lang = lang.split('-').first();

    if (m_current_task) {
        if (m_current_task->engine == engine_t::stt) {
            if (m_current_task->speech_mode != speech_mode_t::single_sentence) {
                qDebug() << "moving current task to pending";
                auto pending_task = *m_current_task;
                stt_stop_listen(m_current_task->id);
                m_pending_task.emplace(pending_task);
            } else {
                stt_stop_listen(m_current_task->id);
            }
        } else if (m_current_task->engine == engine_t::tts)
            tts_stop_speech(m_current_task->id);
    }

    qDebug() << "tts play speech";

    m_current_task = {next_task_id(),
                      engine_t::tts,
                      restart_tts_engine(lang),
                      speech_mode_t::play_speech,
                      false,
                      {},
                      {}};

    if (m_current_task->model_id.isEmpty()) {
        m_current_task.reset();

        qWarning() << "failed to restart engine";

        emit current_task_changed();

        refresh_status();

        return INVALID_TASK;
    }

    if (m_tts_engine) m_tts_engine->encode_speech(text.toStdString());

    start_keepalive_current_task();

    emit current_task_changed();

    refresh_status();

    return m_current_task->id;
}

int speech_service::tts_speech_to_file(const QString &text, QString lang) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot tts speech to file, invalid state";
        return INVALID_TASK;
    }

    if (lang.contains('-')) lang = lang.split('-').first();

    if (m_current_task) {
        if (m_current_task->engine == engine_t::stt) {
            if (m_current_task->speech_mode != speech_mode_t::single_sentence) {
                qDebug() << "moving current task to pending";
                auto pending_task = *m_current_task;
                stt_stop_listen(m_current_task->id);
                m_pending_task.emplace(pending_task);
            } else {
                stt_stop_listen(m_current_task->id);
            }
        } else if (m_current_task->engine == engine_t::tts)
            tts_stop_speech(m_current_task->id);
    }

    qDebug() << "tts speech to file";

    m_current_task = {next_task_id(),
                      engine_t::tts,
                      restart_tts_engine(lang),
                      speech_mode_t::speech_to_file,
                      false,
                      {0, static_cast<size_t>(text.size())},
                      {}};

    if (m_current_task->model_id.isEmpty()) {
        m_current_task.reset();

        qWarning() << "failed to restart engine";

        emit current_task_changed();

        refresh_status();

        return INVALID_TASK;
    }

    if (m_tts_engine) m_tts_engine->encode_speech(text.toStdString());

    start_keepalive_current_task();

    emit current_task_changed();

    refresh_status();

    return m_current_task->id;
}

int speech_service::cancel(int task) {
    if (state() == state_t::unknown) {
        qWarning() << "cannot cancel, invalid state";
        return FAILURE;
    }

    qDebug() << "cancel";

    if (!m_current_task) {
        qWarning() << "no current task";
        return FAILURE;
    }

    if (m_current_task->id != task) {
        qWarning() << "invalid task id";
    }

    stop_keepalive_current_task();

    m_player.pause();

    if (m_pending_task) {
        qDebug() << "retriving pending task:" << m_pending_task->id;

        m_previous_task = m_current_task;

        auto next_task = *m_pending_task;

        if (m_pending_task->engine == engine_t::stt) {
            if (m_current_task->engine == engine_t::tts) stop_tts_engine();
            restart_stt_engine(next_task.speech_mode, next_task.model_id,
                               next_task.translate);
        } else if (next_task.engine == engine_t::tts) {
            if (m_current_task->engine == engine_t::stt) stop_stt_engine();
            restart_tts_engine(m_pending_task->model_id);
        }

        restart_audio_source();
        m_current_task.emplace(next_task);
        start_keepalive_current_task();
        m_pending_task.reset();

        emit current_task_changed();
    } else {
        if (m_current_task->engine == engine_t::tts)
            stop_tts_engine();
        else if (m_current_task->engine == engine_t::stt)
            stop_stt_engine();
        else if (m_current_task->engine == engine_t::mnt)
            stop_mnt_engine();
    }

    m_tts_queue = decltype(m_tts_queue){};
    m_player.stop();

    refresh_status();

    return SUCCESS;
}

int speech_service::stt_stop_listen(int task) {
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
            if (m_current_task->engine != engine_t::stt) {
                qWarning() << "valid task id but invalid engine";
                return FAILURE;
            }

            stop_keepalive_current_task();
            if (m_current_task->speech_mode == speech_mode_t::single_sentence ||
                m_current_task->speech_mode == speech_mode_t::automatic) {
                stop_stt_engine();
            } else if (m_stt_engine && m_stt_engine->started()) {
                stop_stt_engine_gracefully();
            } else {
                stop_stt_engine();
            }
        } else {
            qWarning() << "invalid task id";
            return FAILURE;
        }
    } else {
        if (m_current_task && m_current_task->id == task &&
            m_current_task->engine != engine_t::stt) {
            qWarning() << "valid task id but invalid engine";
            return FAILURE;
        }
    }

    return SUCCESS;
}

int speech_service::tts_stop_speech(int task) {
    if (state() == state_t::unknown || state() == state_t::not_configured ||
        state() == state_t::busy) {
        qWarning() << "cannot stop_listen, invalid state";
        return FAILURE;
    }

    if (!m_current_task || m_current_task->id != task) {
        qWarning() << "invalid task id";
        return FAILURE;
    }

    if (m_current_task->engine != engine_t::tts) {
        qWarning() << "valid task id but invalid engine";
        return FAILURE;
    }

    stop_keepalive_current_task();

    m_player.pause();
    stop_tts_engine();
    m_tts_queue = decltype(m_tts_queue){};
    m_player.stop();

    return SUCCESS;
}

void speech_service::stop_stt_engine_gracefully() {
    qDebug() << "stop stt engine gracefully";

    if (m_source) {
        if (m_stt_engine) m_stt_engine->set_speech_started(false);
        m_source->stop();
    } else {
        stop_stt_engine();
    }
}

void speech_service::stop_tts_engine() {
    qDebug() << "stop tts engine";

    m_pending_task.reset();

    if (m_current_task) {
        m_current_task.reset();
        stop_keepalive_current_task();
        emit current_task_changed();
    }

    if (m_tts_engine) m_tts_engine->request_stop();

    refresh_status();
}

void speech_service::stop_stt_engine() {
    qDebug() << "stop stt engine";

    if (m_stt_engine) m_stt_engine->stop();

    restart_audio_source();

    m_pending_task.reset();

    if (m_current_task) {
        m_current_task.reset();
        stop_keepalive_current_task();
        emit current_task_changed();
    }

    refresh_status();
}

void speech_service::stop_mnt_engine() {
    qDebug() << "stop mnt engine";

    if (m_mnt_engine) m_mnt_engine->stop();

    m_pending_task.reset();

    if (m_current_task) {
        m_current_task.reset();
        stop_keepalive_current_task();
        emit current_task_changed();
    }

    refresh_status();
}

void speech_service::stop_stt() {
    qDebug() << "stop stt";
    stop_stt_engine();
}

void speech_service::handle_audio_error() {
    if (audio_source_type() == source_t::file && m_current_task) {
        qWarning() << "file audio source error";
        emit error(error_t::file_source);
        cancel(m_current_task->id);
    } else {
        qWarning() << "audio source error";
        emit error(error_t::mic_source);
        stop_stt();
    }
}

void speech_service::handle_audio_ended() {
    if (audio_source_type() == source_t::file && m_current_task) {
        qDebug() << "file audio source ended successfuly";
    } else {
        qDebug() << "audio source ended successfuly";
    }
}

void speech_service::restart_audio_source(const QString &source_file) {
    if (m_stt_engine && m_stt_engine->started()) {
        qDebug() << "creating audio source";

        if (m_source) m_source->disconnect();

        if (source_file.isEmpty())
            m_source = std::make_unique<mic_source>();
        else
            m_source = std::make_unique<file_source>(source_file);

        set_progress(m_source->progress());
        connect(m_source.get(), &audio_source::audio_available, this,
                &speech_service::handle_audio_available, Qt::QueuedConnection);
        connect(m_source.get(), &audio_source::error, this,
                &speech_service::handle_audio_error, Qt::QueuedConnection);
        connect(m_source.get(), &audio_source::ended, this,
                &speech_service::handle_audio_ended, Qt::QueuedConnection);
    } else if (m_source) {
        m_source.reset();
        set_progress(-1.0);
    }
}

void speech_service::handle_keepalive_timeout() {
    qWarning() << "keepalive timeout => shutting down";
    QCoreApplication::quit();
}

void speech_service::handle_task_timeout() {
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

void speech_service::update_task_state() {
    refresh_status();

    // 0 = Idle
    // 1 = Speech detected
    // 2 = Processing
    // 3 = Model initialization
    // 4 = Playing Speech

    auto new_task_state = [&] {
        if (m_stt_engine && m_stt_engine->started()) {
            switch (m_stt_engine->speech_detection_status()) {
                case stt_engine::speech_detection_status_t::speech_detected:
                    return 1;
                case stt_engine::speech_detection_status_t::decoding:
                    return 2;
                case stt_engine::speech_detection_status_t::initializing:
                    return 3;
                case stt_engine::speech_detection_status_t::no_speech:
                    break;
            }
        } else if (m_player.state() == QMediaPlayer::State::PlayingState) {
            return 4;
        } else if (m_player.state() == QMediaPlayer::State::PausedState) {
            return 0;
        } else if (m_tts_engine &&
                   m_tts_engine->state() != tts_engine::state_t::idle) {
            switch (m_tts_engine->state()) {
                case tts_engine::state_t::encoding:
                    return 2;
                case tts_engine::state_t::initializing:
                    return 3;
                case tts_engine::state_t::error:
                case tts_engine::state_t::idle:
                    break;
            }
        } else if (m_mnt_engine &&
                   m_mnt_engine->state() != mnt_engine::state_t::idle) {
            switch (m_mnt_engine->state()) {
                case mnt_engine::state_t::translating:
                    return 2;
                case mnt_engine::state_t::initializing:
                    return 3;
                case mnt_engine::state_t::error:
                case mnt_engine::state_t::idle:
                    break;
            }
        }

        return 0;
    }();

    if (m_task_state != new_task_state) {
        qDebug() << "task state changed:" << m_task_state << "=>"
                 << new_task_state;
        m_task_state = new_task_state;
        emit task_state_changed();
    }
}

void speech_service::set_state(state_t new_state) {
    if (new_state != m_state) {
        qDebug() << "service state changed:" << m_state << "=>" << new_state;
        m_state = new_state;
        emit state_changed();
    }
}

void speech_service::refresh_status() {
    state_t new_state;

    if (models_manager::instance()->busy()) {
        new_state = state_t::busy;
    } else if (!models_manager::instance()->has_model_of_role(
                   models_manager::model_role::stt) &&
               !models_manager::instance()->has_model_of_role(
                   models_manager::model_role::tts) &&
               !models_manager::instance()->has_model_of_role(
                   models_manager::model_role::mnt)) {
        new_state = state_t::not_configured;
    } else if (audio_source_type() == source_t::file) {
        new_state = state_t::transcribing_file;
    } else if (audio_source_type() == source_t::mic) {
        if (!m_current_task) {
            qWarning() << "no current task but source is mic";
            return;
        }

        if (m_current_task->engine == engine_t::tts) {
            new_state = state_t::playing_speech;
        } else if (m_current_task->engine == engine_t::mnt) {
            new_state = state_t::translating;
        } else if (m_current_task->speech_mode == speech_mode_t::manual) {
            new_state = m_stt_engine && m_stt_engine->started() &&
                                m_stt_engine->speech_status()
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
    } else if (m_current_task && m_current_task->engine == engine_t::tts) {
        if (m_current_task->speech_mode == speech_mode_t::play_speech)
            new_state = state_t::playing_speech;
        else
            new_state = state_t::writing_speech_to_file;
    } else if (m_current_task && m_current_task->engine == engine_t::mnt) {
        if (m_mnt_engine &&
            m_mnt_engine->state() != mnt_engine::state_t::idle &&
            m_mnt_engine->state() != mnt_engine::state_t::error)
            new_state = state_t::translating;
        else
            new_state = state_t::idle;
    } else {
        new_state = state_t::idle;
    }

    qDebug() << "service refresh status, new state:" << new_state;

    set_state(new_state);
}

QString speech_service::default_stt_model() const {
    return test_default_stt_model(settings::instance()->default_stt_model());
}

QString speech_service::default_stt_lang() const {
    return m_available_stt_models_map
        .at(test_default_stt_model(settings::instance()->default_stt_model()))
        .lang_id;
}

QString speech_service::default_tts_model() const {
    return test_default_tts_model(settings::instance()->default_tts_model());
}

QString speech_service::default_tts_lang() const {
    return m_available_tts_models_map
        .at(test_default_tts_model(settings::instance()->default_tts_model()))
        .lang_id;
}

QString speech_service::default_ttt_model() const {
    return test_default_ttt_model({});
}

QString speech_service::default_ttt_lang() const {
    return m_available_ttt_models_map.at(test_default_ttt_model({})).lang_id;
}

QString speech_service::default_mnt_lang() const {
    return test_default_mnt_lang(settings::instance()->default_mnt_lang());
}

std::vector<std::reference_wrapper<const speech_service::model_data_t>>
speech_service::model_data_for_lang(
    const QString &lang_id,
    const std::map<QString, model_data_t> &available_models_map) {
    std::vector<std::reference_wrapper<const speech_service::model_data_t>>
        list;

    std::for_each(available_models_map.cbegin(), available_models_map.cend(),
                  [&](const auto &p) {
                      if (p.second.lang_id == lang_id)
                          list.push_back(std::cref(p.second));
                  });

    return list;
}

QString speech_service::default_mnt_out_lang() const {
    return test_default_mnt_out_lang(
        settings::instance()->default_mnt_out_lang());
}

QString speech_service::test_default_model(
    const QString &lang,
    const std::map<QString, model_data_t> &available_models_map) {
    if (available_models_map.empty()) return {};

    auto it = available_models_map.find(lang);

    if (it == available_models_map.cend()) {
        it = std::find_if(
            available_models_map.cbegin(), available_models_map.cend(),
            [&lang](const auto &p) { return p.second.lang_id == lang; });
        if (it != available_models_map.cend()) return it->first;
    } else {
        return it->first;
    }

    return available_models_map.cbegin()->first;
}

QString speech_service::test_default_stt_model(const QString &lang) const {
    return test_default_model(lang, m_available_stt_models_map);
}

QString speech_service::test_default_tts_model(const QString &lang) const {
    return test_default_model(lang, m_available_tts_models_map);
}

QString speech_service::test_default_mnt_model(const QString &lang) const {
    auto it = m_available_mnt_lang_to_model_id_map.find(lang);
    if (it == m_available_mnt_lang_to_model_id_map.end()) return {};
    return it->second;
}

QString speech_service::test_default_mnt_lang(const QString &lang) const {
    auto it = m_available_mnt_lang_to_model_id_map.find(lang);
    if (it == m_available_mnt_lang_to_model_id_map.end()) return {};
    return it->first;
}

QString speech_service::test_mnt_out_lang(const QString &in_lang,
                                          const QString &out_lang) const {
    if (in_lang.isEmpty() || in_lang == out_lang) return {};

    auto models = model_data_for_lang(in_lang, m_available_mnt_models_map);
    if (models.empty()) return {};

    if (out_lang.isEmpty()) return models.front().get().trg_lang_id;

    auto it = std::find_if(models.cbegin(), models.cend(), [&](const auto &wr) {
        return wr.get().trg_lang_id == out_lang;
    });

    if (it != models.cend()) return out_lang;  // direct match

    if (in_lang != QStringLiteral("en") && out_lang != QStringLiteral("en")) {
        // try to find in_lang=>en
        if (std::find_if(models.cbegin(), models.cend(), [&](const auto &wr) {
                return wr.get().trg_lang_id == QStringLiteral("en");
            }) != models.cend()) {  // in_lang=>en exists

            // try to find en=>out_lang
            auto models = model_data_for_lang(QStringLiteral("en"),
                                              m_available_mnt_models_map);

            if (std::find_if(models.cbegin(), models.cend(),
                             [&](const auto &wr) {
                                 return wr.get().trg_lang_id == out_lang;
                             }) != models.cend())
                return out_lang;  // indirect match
        }
    }

    return models.front().get().trg_lang_id;  // fallback
}

QString speech_service::test_default_mnt_out_lang(const QString &lang) const {
    return test_mnt_out_lang(settings::instance()->default_mnt_lang(), lang);
}

QString speech_service::test_default_ttt_model(const QString &lang) const {
    return test_default_model(lang, m_available_ttt_models_map);
}

void speech_service::set_default_stt_model(const QString &model_id) const {
    if (test_default_stt_model(model_id) == model_id) {
        settings::instance()->set_default_stt_model(model_id);
    } else {
        qWarning() << "invalid default stt model";
    }
}

void speech_service::set_default_stt_lang(const QString &lang_id) const {
    settings::instance()->set_default_stt_model(
        test_default_stt_model(lang_id));
}

void speech_service::set_default_mnt_lang(const QString &lang_id) const {
    settings::instance()->set_default_mnt_lang(test_default_mnt_lang(lang_id));
}

void speech_service::set_default_mnt_out_lang(const QString &lang_id) const {
    settings::instance()->set_default_mnt_out_lang(
        test_default_mnt_out_lang(lang_id));
}

void speech_service::set_default_tts_model(const QString &model_id) const {
    if (test_default_tts_model(model_id) == model_id) {
        settings::instance()->set_default_tts_model(model_id);
    } else {
        qWarning() << "invalid default tts model";
    }
}

void speech_service::set_default_tts_lang(const QString &lang_id) const {
    settings::instance()->set_default_tts_model(
        test_default_tts_model(lang_id));
}

QVariantMap speech_service::translations() const {
    QVariantMap map;

    map.insert(QStringLiteral("lang_not_conf"),
               tr("No language has been set."));
    map.insert(QStringLiteral("say_smth"), tr("Say something..."));
    map.insert(QStringLiteral("press_say_smth"),
               tr("Press and say something..."));
    map.insert(QStringLiteral("click_say_smth"),
               tr("Click and say something..."));
    map.insert(QStringLiteral("busy_stt"), tr("Busy..."));
    map.insert(QStringLiteral("decoding"), tr("Processing, please wait..."));
    map.insert(QStringLiteral("initializing"),
               tr("Getting ready, please wait..."));

    return map;
}

speech_service::state_t speech_service::state() const { return m_state; }

int speech_service::current_task_id() const {
    return m_current_task ? m_current_task->id : INVALID_TASK;
}

int speech_service::dbus_state() const { return static_cast<int>(state()); }

void speech_service::start_keepalive_current_task() {
    if (settings::instance()->launch_mode() != settings::launch_mode_t::service)
        return;

    m_keepalive_current_task_timer.start();
}

void speech_service::stop_keepalive_current_task() {
    if (settings::instance()->launch_mode() != settings::launch_mode_t::service)
        return;

    m_keepalive_current_task_timer.stop();
}

void speech_service::remove_cached_wavs() {
    QDir dir{settings::instance()->cache_dir()};

    dir.setNameFilters(QStringList{} << "*.wav");
    dir.setFilter(QDir::Files);

    for (const auto &file : std::as_const(dir).entryList()) dir.remove(file);
}

void speech_service::setup_modules() {
    module_tools::init_module(QStringLiteral("rhvoicedata"));
    module_tools::init_module(QStringLiteral("rhvoiceconfig"));
    module_tools::init_module(QStringLiteral("espeakdata"));

#ifdef USE_SFOS
    // add mbrola bin to PATH

    auto bin_dir = QStringLiteral("/usr/share/%1/bin").arg(APP_BINARY_ID);
    if (!QFileInfo::exists(bin_dir)) {
        qWarning() << "no bin dir:" << bin_dir;
        return;
    }

    try {
        auto *old_path = getenv("PATH");
        if (old_path)
            setenv(
                "PATH",
                fmt::format("{}:{}", bin_dir.toStdString(), old_path).c_str(),
                true);
        else
            setenv("PATH", bin_dir.toStdString().c_str(), false);
    } catch (const std::runtime_error &err) {
        qWarning() << "error:" << err.what();
    }
#endif
}

QString speech_service::merge_wav_files(const std::vector<QString> &files) {
    QString out_file_path =
        QStringLiteral("%1/merged-%2.wav")
            .arg(settings::instance()->cache_dir(),
                 QString::number(qHash(
                     std::accumulate(files.cbegin(), files.cend(), QString{},
                                     [](auto new_name, const auto &file) {
                                         return std::move(new_name) +
                                                QFileInfo{file}.baseName();
                                     }))));

    QFile out_file{out_file_path};
    if (!out_file.open(QIODevice::WriteOnly)) {
        qWarning() << "failed to write to file:" << out_file_path;
        return {};
    }

    const auto header_size = sizeof(tts_engine::wav_header);

    out_file.seek(header_size);

    tts_engine::wav_header header;

    for (const auto &file_path : files) {
        QFile in_file{file_path};
        if (!in_file.open(QIODevice::ReadOnly)) {
            qWarning() << "failed to read file:" << file_path;
            continue;
        }

        if (auto size =
                in_file.read(reinterpret_cast<char *>(&header), header_size);
            size < 0 || size < static_cast<decltype(size)>(header_size)) {
            qWarning() << "invalid wav header:" << file_path;
            continue;
        }

        out_file.write(in_file.readAll());
    }

    if (out_file.pos() == header_size) return {};

    header.data_size = out_file.pos() - header_size;

    out_file.seek(0);

    out_file.write(reinterpret_cast<char *>(&header), header_size);

    return out_file_path;
}

// DBus

int speech_service::SttStartListen(int mode, const QString &lang,
                                   bool translate) {
    qDebug() << "[dbus => service] called StartListen:" << lang << mode;
    m_keepalive_timer.start();

    speech_mode_t speech_mode;

    if (mode == 0)
        speech_mode = speech_mode_t::automatic;
    else if (mode == 1)
        speech_mode = speech_mode_t::manual;
    else if (mode == 2)
        speech_mode = speech_mode_t::single_sentence;
    else {
        qWarning() << "invalid speech mode";
        return INVALID_TASK;
    }

    return stt_start_listen(speech_mode, lang, translate);
}

int speech_service::SttStopListen(int task) {
    qDebug() << "[dbus => service] called StopListen:" << task;
    m_keepalive_timer.start();

    return stt_stop_listen(task);
}

int speech_service::Cancel(int task) {
    qDebug() << "[dbus => service] called Cancel:" << task;
    m_keepalive_timer.start();

    return cancel(task);
}

int speech_service::SttTranscribeFile(const QString &file, const QString &lang,
                                      bool translate) {
    qDebug() << "[dbus => service] called TranscribeFile:" << file << lang;
    start_keepalive_current_task();

    return stt_transcribe_file(file, lang, translate);
}

double speech_service::SttGetFileTranscribeProgress(int task) {
    qDebug() << "[dbus => service] called GetFileTranscribeProgress:" << task;
    start_keepalive_current_task();

    return stt_transcribe_file_progress(task);
}

int speech_service::KeepAliveService() {
    qDebug() << "[dbus => service] called KeepAliveService";
    m_keepalive_timer.start();

    return m_keepalive_timer.remainingTime();
}

int speech_service::KeepAliveTask(int task) {
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

int speech_service::TtsPlaySpeech(const QString &text, const QString &lang) {
    qDebug() << "[dbus => service] called TtsPlaySpeech:" << lang;
    start_keepalive_current_task();

    return tts_play_speech(text, lang);
}

int speech_service::TtsSpeechToFile(const QString &text, const QString &lang) {
    qDebug() << "[dbus => service] called TtsSpeechToFile:" << lang;
    start_keepalive_current_task();

    return tts_speech_to_file(text, lang);
}

int speech_service::TtsStopSpeech(int task) {
    qDebug() << "[dbus => service] called TtsStopSpeech";
    start_keepalive_current_task();

    return tts_stop_speech(task);
}

int speech_service::MntTranslate(const QString &text, const QString &lang,
                                 const QString &out_lang) {
    qDebug() << "[dbus => service] called MntTranslate";
    start_keepalive_current_task();

    return mnt_translate(text, lang, out_lang);
}

QVariantMap speech_service::MntGetOutLangs(const QString &lang) {
    qDebug() << "[dbus => service] called MntGetOutLangs";
    return mnt_out_langs(lang);
}

int speech_service::Reload() {
    qDebug() << "[dbus => service] called Reload";
    m_keepalive_timer.start();

    models_manager::instance()->reload();
    return SUCCESS;
}

/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dsnote.h"

#include <functional>
#include <algorithm>

#include "settings.h"
#include "file_source.h"
#include "mic_source.h"

dsnote_app::dsnote_app() : QObject{}
{
    connect(this, &dsnote_app::speech_clear, this, &dsnote_app::handle_speech_clear);
    connect(settings::instance(), &settings::lang_changed, this, &dsnote_app::handle_models_changed);
    connect(settings::instance(), &settings::speech_mode_changed, this, &dsnote_app::handle_models_changed);
    connect(&manager, &models_manager::models_changed, this, &dsnote_app::handle_models_changed);
    connect(&manager, &models_manager::download_progress, this, [this](const QString& id, double progress) {
        emit lang_download_progress(id, progress);
    });
    connect(&manager, &models_manager::busy_changed, this, [this] { emit busy_changed(); });

    handle_models_changed();
}

dsnote_app::source_type dsnote_app::audio_source_type() const
{
    if (!source)
        return source_type::SourceNone;
    if (source->type() == audio_source::source_type::mic)
        return source_type::SourceMic;
    if (source->type() == audio_source::source_type::file)
        return source_type::SourceFile;
    return source_type::SourceNone;
}

std::optional<std::pair<QString, QString>> dsnote_app::setup_active_lang()
{
    const auto lang_in_settings = settings::instance()->lang();

    available_langs_list.clear();
    active_lang_value.clear();
    active_lang_idx_value = -1;

    std::pair<QString, QString> active_files; // model, scorer

    int idx = 0;
    for (const auto& lang : manager.available_langs()) {
        if (lang_in_settings == lang.id || active_lang_idx_value == -1) {
            active_lang_idx_value = idx;
            active_lang_value = lang.id;
            active_files.first = lang.model_file;
            active_files.second = lang.scorer_file;
        }

        available_langs_list.push_back(QString{"%1 (%2)"}.arg(lang.name, lang.lang_id));

        ++idx;
    }

    if (active_files.first.isEmpty()) {
        return std::nullopt;
    }

    return active_files;
}

void dsnote_app::stop()
{
    ds.reset();
    restart_audio_source();
}

void dsnote_app::handle_models_changed()
{
    if (auto active_files = setup_active_lang()) {
        restart_ds(active_files->first, active_files->second);
        if (ds->ok()) {
            restart_audio_source();
            emit langs_changed();
            return;
        }
    }

    stop();

    emit langs_changed();
}

void dsnote_app::restart_ds(speech_mode_req_type speech_mode_req)
{
    if (auto active_files = setup_active_lang()) {
        restart_ds(active_files->first, active_files->second, speech_mode_req);
    }
}

void dsnote_app::restart_ds(const QString& model_file, const QString& scorer_file, speech_mode_req_type speech_mode_req)
{
    deepspeech_wrapper::callbacks_type call_backs{
        std::bind(&dsnote_app::handle_text_decoded, this, std::placeholders::_1),
        std::bind(&dsnote_app::handle_intermediate_text_decoded, this, std::placeholders::_1),
        std::bind(&dsnote_app::handle_speech_status_changed, this, std::placeholders::_1)
    };

    ds = std::make_unique<deepspeech_wrapper>(
                model_file.toStdString(),
                scorer_file.toStdString(),
                call_backs,
                speech_mode_req == speech_mode_req_type::settings ?
                    settings::instance()->speech_mode() == settings::speech_mode_type::SpeechAutomatic ?
                              deepspeech_wrapper::speech_mode_type::automatic :
                              deepspeech_wrapper::speech_mode_type::manual :
                speech_mode_req == speech_mode_req_type::automatic ?
                        deepspeech_wrapper::speech_mode_type::automatic :
                        deepspeech_wrapper::speech_mode_type::manual);

    if (!ds->ok())
        qWarning() << "cannot init engine"; 
}

void dsnote_app::handle_intermediate_text_decoded(const std::string& text)
{
    qDebug() << "intermediate_text_decoded:" << QString::fromStdString(text);

    this->intermediate_text_value = QString::fromStdString(text);

    emit intermediate_text_changed();
}

void dsnote_app::handle_text_decoded(const std::string &text)
{
    qDebug() << "text_decoded:" << QString::fromStdString(text);

    auto note = settings::instance()->note();

    if (note.isEmpty())
        settings::instance()->set_note(QString::fromStdString(text));
    else
        settings::instance()->set_note(note + "\n" + QString::fromStdString(text));

    this->intermediate_text_value.clear();

    emit text_changed();
    emit intermediate_text_changed();
}

void dsnote_app::handle_speech_status_changed(bool speech_detected)
{
    if (!speech_detected)
        emit speech_clear();
    emit speech_changed();
}

void dsnote_app::handle_speech_clear()
{
    if (source && settings::instance()->speech_mode() == settings::speech_mode_type::SpeechAutomatic)
        source->clear();
}

QVariantList dsnote_app::langs() const
{
    QVariantList list;

    auto langs = manager.langs();
    std::transform(langs.cbegin(), langs.cend(), std::back_inserter(list), [](const auto& lang) {
        return QVariantList{
            lang.id,
            lang.lang_id,
            QString{"%1 (%2)"}.arg(lang.name, lang.lang_id),
            lang.available,
            lang.downloading,
            lang.download_progress};
    });

    return list;
}

QVariantList dsnote_app::available_langs() const
{
    return available_langs_list;
}

void dsnote_app::download_lang(const QString& id)
{
    manager.download_model(id);
}

void dsnote_app::delete_lang(const QString& id)
{
    stop();
    manager.delete_model(id);
}

void dsnote_app::set_active_lang_idx(int idx)
{
    if (active_lang_idx_value != idx) {
        const auto langs_list = manager.available_langs();
        if (static_cast<int>(langs_list.size()) > idx)
            settings::instance()->set_lang(langs_list.at(idx).id);
    }
}

void dsnote_app::handle_audio_available()
{
    //qDebug() << "handle_audio_available";
    if (source && ds && ds->ok()) {
        auto [buff, max_size] = ds->borrow_buff();
        if (buff && max_size > 0) {
            ds->return_buff(buff, source->read_audio(buff, max_size));

            set_progress(source->progress());
        }
    }
}

void dsnote_app::set_progress(double p)
{
    if (p != progress_value) {
        progress_value = p;
        emit progress_changed();
    }
}

void dsnote_app::cancel_file_source()
{
    if (source && source->type() == audio_source::source_type::file) {
        restart_ds(speech_mode_req_type::settings);
        restart_audio_source();
    }
}

void dsnote_app::set_file_source(const QString& source_file)
{
    restart_ds(speech_mode_req_type::automatic);
    restart_audio_source(source_file);
}

void dsnote_app::set_file_source(const QUrl& source_file)
{
    restart_ds(speech_mode_req_type::automatic);
    restart_audio_source(source_file.toLocalFile());
}

void dsnote_app::set_mic_source()
{
    restart_ds(speech_mode_req_type::automatic);
    restart_audio_source();
}

void dsnote_app::handle_audio_error()
{
    if (source->type() == audio_source::source_type::file) {
        qWarning() << "cannot start file audio source";
        emit error(error_type::ErrorFileSource);
        restart_ds(speech_mode_req_type::settings);
        restart_audio_source();
    } else {
        qWarning() << "cannot start mic audio source";
        emit error(error_type::ErrorMicSource);
        source.reset();
        emit audio_source_type_changed();
        if (ds) ds->restart();
    }
}

void dsnote_app::handle_audio_ended()
{
    if (source->type() == audio_source::source_type::file) {
        qDebug() << "file audio source ended successfuly";
        restart_ds(speech_mode_req_type::settings);
        restart_audio_source();
        emit transcribe_done();
    }
}

void dsnote_app::restart_audio_source(const QString& source_file)
{
    if (ds && ds->ok()) {
        ds->restart();
        if (source_file.isEmpty())
            source = std::make_unique<mic_source>();
        else
            source = std::make_unique<file_source>(source_file);
        set_progress(source->progress());
        connect(source.get(), &audio_source::audio_available, this, &dsnote_app::handle_audio_available, Qt::QueuedConnection);
        connect(source.get(), &audio_source::error, this, &dsnote_app::handle_audio_error, Qt::QueuedConnection);
        connect(source.get(), &audio_source::ended, this, &dsnote_app::handle_audio_ended, Qt::QueuedConnection);
    } else {
        qWarning() << "ds is not inited, cannot start audio source";
        source.reset();
        set_progress(-1);
    }

    emit audio_source_type_changed();
}

bool dsnote_app::speech() const
{
    if (ds && ds->ok())
        return ds->speech_detected();

    return false;
}


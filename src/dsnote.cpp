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

        available_langs_list.push_back(QString{"%1 (%2)"}.arg(lang.name, lang.id));

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
    restart_recording();
}

void dsnote_app::handle_models_changed()
{
    if (auto active_files = setup_active_lang()) {
        restart_ds(active_files->first, active_files->second);
        if (ds->ok()) {
            restart_recording();
            emit langs_changed();
            return;
        }
    }

    stop();

    emit langs_changed();
}

void dsnote_app::restart_ds(const QString& model_file, const QString& scorer_file)
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
                settings::instance()->speech_mode() == settings::speech_mode_type::SpeechAutomatic ?
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
    if (mic && settings::instance()->speech_mode() == settings::speech_mode_type::SpeechAutomatic)
        mic->clear();
}

QVariantList dsnote_app::langs() const
{
    QVariantList list;

    auto langs = manager.langs();
    std::transform(langs.cbegin(), langs.cend(), std::back_inserter(list), [](const auto& lang) {
        return QVariantList{
            lang.id,
            QString{"%1 (%2)"}.arg(lang.name, lang.id),
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
    if (mic && ds && ds->ok()) {
        auto [buff, max_size] = ds->borrow_buff();
        if (buff && max_size > 0)
            ds->return_buff(buff, mic->read_audio(buff, max_size));
    }
}

void dsnote_app::restart_recording()
{
    if (ds && ds->ok()) {
        mic = std::make_unique<mic_source>();
        connect(mic.get(), &mic_source::audio_available, this, &dsnote_app::handle_audio_available);
    } else {
        qWarning() << "ds is not inited, cannot start mic";
        mic.reset();
    }
}

bool dsnote_app::speech() const
{
    if (ds && ds->ok())
        return ds->speech_detected();

    return false;
}


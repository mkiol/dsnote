/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "settings.h"

#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#include <iostream>
#include <fstream>

#include "info.h"

settings* settings::m_instance = nullptr;

settings* settings::instance()
{
    if (settings::m_instance == nullptr) settings::m_instance = new settings{};
    return settings::m_instance;
}

QString settings::models_dir() const
{
    auto dir = value("service/models_dir", value("lang_models_dir")).toString();
    if (dir.isEmpty()) {
        dir = QDir{QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)}
                  .filePath("DeepSpeech models");
        QDir{}.mkpath(dir);
    }

    return dir;
}

QUrl settings::models_dir_url() const
{
    return QUrl::fromLocalFile(models_dir());
}

void settings::set_models_dir(const QString &value)
{
    if (models_dir() != value) {
        setValue("service/models_dir", value);
        emit models_dir_changed();
    }
}

void settings::set_models_dir_url(const QUrl &value)
{
    set_models_dir(value.toLocalFile());
}

QString settings::models_dir_name() const
{
    return models_dir_url().fileName();
}

QString settings::default_model() const
{
    return value("service/default_model", value("lang", "en")).toString(); // english is a default;
}

void settings::set_default_model(const QString &value)
{
    if (default_model() != value) {
        setValue("service/default_model", value);
        emit default_model_changed();
    }
}

settings::speech_mode_type settings::speech_mode() const
{
    return static_cast<speech_mode_type>(value("speech_mode2",
                      static_cast<int>(speech_mode_type::SpeechSingleSentence)).toInt());
}

void settings::set_speech_mode(speech_mode_type value)
{
    if (speech_mode() != value) {
        setValue("speech_mode2", static_cast<int>(value));
        emit speech_mode_changed();
    }
}

bool settings::show_experimental() const
{
    return value("show_experimental", true).toBool();
}

void settings::set_show_experimental(bool value)
{
    if (show_experimental() != value) {
        setValue("show_experimental", value);
        emit show_experimental_changed();
    }
}

QString settings::note() const
{
    return value("note", "").toString();
}

void settings::set_note(const QString& value)
{
    if (note() != value) {
        setValue("note", value);
        emit note_changed();
    }
}

QUrl settings::app_icon() const
{
    return QUrl::fromLocalFile(QString("/usr/share/icons/hicolor/172x172/apps/%1.png").arg(dsnote::APP_ID));
}

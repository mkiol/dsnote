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

settings::settings(QObject* parent)
    : QSettings{parent}
{}

settings* settings::instance()
{
    if (settings::m_instance == nullptr)
        settings::m_instance = new settings();

    return settings::m_instance;
}

QString settings::lang_models_dir() const
{
    auto dir = value("lang_models_dir", "").toString();
    if (dir.isEmpty()) {
        dir = QDir{QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)}
                  .filePath("DeepSpeech models");
        QDir{}.mkpath(dir);
    }

    return dir;
}

QUrl settings::lang_models_dir_url() const
{
    return QUrl::fromLocalFile(lang_models_dir());
}

void settings::set_lang_models_dir(const QString& value)
{
    if (lang_models_dir() != value) {
        setValue("lang_models_dir", value);
        emit lang_models_dir_changed();
    }
}

void settings::set_lang_models_dir_url(const QUrl& value)
{
    set_lang_models_dir(value.toLocalFile());
}

QString settings::lang_models_dir_name() const
{
    return lang_models_dir_url().fileName();
}

QString settings::lang() const
{
    return value("lang", "en").toString(); // english is a default;
}

void settings::set_lang(const QString& value)
{
    if (lang() != value) {
        setValue("lang", value);
        emit lang_changed();
    }
}

settings::speech_mode_type settings::speech_mode() const
{
    return static_cast<speech_mode_type>(value("speech_mode",
                      static_cast<int>(speech_mode_type::SpeechManual)).toInt());
}

void settings::set_speech_mode(speech_mode_type value)
{
    if (speech_mode() != value) {
        setValue("speech_mode", static_cast<int>(value));
        emit speech_mode_changed();
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

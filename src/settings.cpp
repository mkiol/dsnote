/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "settings.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <fstream>
#include <iostream>

#include "info.h"

settings* settings::m_instance = nullptr;

settings::settings() : QSettings{settings_filepath(), QSettings::NativeFormat} {
    qDebug() << "app:" << dsnote::ORG << dsnote::APP_ID;
    qDebug() << "config location:"
             << QStandardPaths::writableLocation(
                    QStandardPaths::ConfigLocation);
    qDebug() << "data location:"
             << QStandardPaths::writableLocation(
                    QStandardPaths::AppDataLocation);
    qDebug() << "cache location:"
             << QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    qDebug() << "settings file:" << fileName();
}

QString settings::settings_filepath() {
    QDir conf_dir{
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)};
    conf_dir.mkpath(QCoreApplication::organizationName() + QDir::separator() +
                    QCoreApplication::applicationName());
    return conf_dir.absolutePath() + QDir::separator() +
           QCoreApplication::organizationName() + QDir::separator() +
           QCoreApplication::applicationName() + QDir::separator() +
           settings_filename;
}

settings* settings::instance() {
    if (!settings::m_instance) settings::m_instance = new settings{};
    return settings::m_instance;
}

QString settings::models_dir() const {
    auto dir = value(QStringLiteral("service/models_dir"),
                     value(QStringLiteral("lang_models_dir")))
                   .toString();
    if (dir.isEmpty()) {
        dir = QDir{QStandardPaths::writableLocation(
                       QStandardPaths::DownloadLocation)}
                  .filePath(QStringLiteral("DeepSpeech models"));
        QDir{}.mkpath(dir);
    }

    return dir;
}

QUrl settings::models_dir_url() const {
    return QUrl::fromLocalFile(models_dir());
}

void settings::set_models_dir(const QString& value) {
    if (models_dir() != value) {
        setValue(QStringLiteral("service/models_dir"), value);
        emit models_dir_changed();
    }
}

void settings::set_models_dir_url(const QUrl& value) {
    set_models_dir(value.toLocalFile());
}

QString settings::models_dir_name() const {
    return models_dir_url().fileName();
}

QString settings::default_model() const {
    return value(QStringLiteral("service/default_model"),
                 value(QStringLiteral("lang"), QStringLiteral("en")))
        .toString();  // english is a default;
}

void settings::set_default_model(const QString& value) {
    if (default_model() != value) {
        setValue(QStringLiteral("service/default_model"), value);
        emit default_model_changed();
    }
}

settings::speech_mode_type settings::speech_mode() const {
    return static_cast<speech_mode_type>(
        value(QStringLiteral("speech_mode"),
              static_cast<int>(speech_mode_type::SpeechSingleSentence))
            .toInt());
}

void settings::set_speech_mode(speech_mode_type value) {
    if (speech_mode() != value) {
        setValue(QStringLiteral("speech_mode"), static_cast<int>(value));
        emit speech_mode_changed();
    }
}

QString settings::note() const {
    return value(QStringLiteral("note"), {}).toString();
}

void settings::set_note(const QString& value) {
    if (note() != value) {
        setValue(QStringLiteral("note"), value);
        emit note_changed();
    }
}

QUrl settings::app_icon() const {
    return QUrl::fromLocalFile(
        QString(QStringLiteral("/usr/share/icons/hicolor/172x172/apps/%1.png"))
            .arg(dsnote::APP_ID));
}

/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "settings.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <fstream>
#include <iostream>

#include "config.h"

QDebug operator<<(QDebug d, settings::mode_t mode) {
    switch (mode) {
        case settings::mode_t::Stt:
            d << "stt";
            break;
        case settings::mode_t::Tts:
            d << "tts";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, settings::launch_mode_t launch_mode) {
    switch (launch_mode) {
        case settings::launch_mode_t::service:
            d << "service";
            break;
        case settings::launch_mode_t::app_stanalone:
            d << "app-standalone";
            break;
        case settings::launch_mode_t::app:
            d << "app";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, settings::speech_mode_t speech_mode) {
    switch (speech_mode) {
        case settings::speech_mode_t::SpeechAutomatic:
            d << "automatic";
            break;
        case settings::speech_mode_t::SpeechManual:
            d << "manual";
            break;
        case settings::speech_mode_t::SpeechSingleSentence:
            d << "single-sentence";
            break;
    }

    return d;
}

settings::settings() : QSettings{settings_filepath(), QSettings::NativeFormat} {
    qDebug() << "app:" << APP_ORG << APP_ID;
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

QString settings::models_dir() const {
    auto dir = value(QStringLiteral("service/models_dir"),
                     value(QStringLiteral("lang_models_dir")))
                   .toString();
    if (dir.isEmpty()) {
#ifdef USE_SFOS
        dir = QDir{QStandardPaths::writableLocation(
                       QStandardPaths::DownloadLocation)}
                  .filePath(QStringLiteral("speech-models"));
#else
        dir = QDir{QStandardPaths::writableLocation(
                       QStandardPaths::CacheLocation)}
                  .filePath(QStringLiteral("speech-models"));
#endif
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

QString settings::cache_dir() const {
    auto dir = value(QStringLiteral("service/cache_dir"),
                     value(QStringLiteral("cache_dir")))
                   .toString();
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir{}.mkpath(dir);
    }

    return dir;
}

QUrl settings::cache_dir_url() const {
    return QUrl::fromLocalFile(cache_dir());
}

void settings::set_cache_dir(const QString& value) {
    if (cache_dir() != value) {
        setValue(QStringLiteral("service/cache_dir"), value);
        emit cache_dir_changed();
    }
}

void settings::set_cache_dir_url(const QUrl& value) {
    set_cache_dir(value.toLocalFile());
}

QString settings::default_stt_model() const {
    return value(QStringLiteral("service/default_model"),
                 QStringLiteral("en"))
        .toString();  // english is a default;
}

void settings::set_default_stt_model(const QString& value) {
    if (default_stt_model() != value) {
        setValue(QStringLiteral("service/default_model"), value);
        emit default_stt_model_changed();
    }
}

QString settings::default_tts_model() const {
    return value(QStringLiteral("service/default_tts_model"),
                 QStringLiteral("en"))
        .toString();  // english is a default;
}

void settings::set_default_tts_model(const QString& value) {
    if (default_tts_model() != value) {
        setValue(QStringLiteral("service/default_tts_model"), value);
        emit default_tts_model_changed();
    }
}

QStringList settings::enabled_models() {
    return value(QStringLiteral("service/enabled_models"), {}).toStringList();
}

void settings::set_enabled_models(const QStringList& value) {
    if (enabled_models() != value) {
        setValue(QStringLiteral("service/enabled_models"), value);
    }
}

QString settings::default_stt_model_for_lang(const QString& lang) {
    return value(QStringLiteral("service/default_model_%1").arg(lang), {})
        .toString();
}

void settings::set_default_stt_model_for_lang(const QString& lang,
                                              const QString& value) {
    if (default_stt_model_for_lang(lang) != value) {
        setValue(QStringLiteral("service/default_model_%1").arg(lang), value);
    }
}

QString settings::default_tts_model_for_lang(const QString& lang) {
    return value(QStringLiteral("service/default_tts_model_%1").arg(lang), {})
        .toString();
}

void settings::set_default_tts_model_for_lang(const QString& lang,
                                              const QString& value) {
    if (default_tts_model_for_lang(lang) != value) {
        setValue(QStringLiteral("service/default_tts_model_%1").arg(lang),
                 value);
    }
}

bool settings::restore_punctuation() const {
    return value(QStringLiteral("service/restore_punctuation"), false).toBool();
}

void settings::set_restore_punctuation(bool value) {
    if (restore_punctuation() != value) {
        setValue(QStringLiteral("service/restore_punctuation"), value);
        emit restore_punctuation_changed();
    }
}

QString settings::file_save_dir() const {
    auto dir = value(QStringLiteral("file_save_dir")).toString();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    return dir;
}

QUrl settings::file_save_dir_url() const {
    return QUrl::fromLocalFile(file_save_dir());
}

void settings::set_file_save_dir(const QString& value) {
    if (file_save_dir() != value) {
        setValue(QStringLiteral("file_save_dir"), value);
        emit file_save_dir_changed();
    }
}

void settings::set_file_save_dir_url(const QUrl& value) {
    set_file_save_dir(value.toLocalFile());
}

QString settings::file_save_dir_name() const {
    return file_save_dir_url().fileName();
}

QString settings::file_save_filename() const {
    auto dir = QDir{file_save_dir()};

    auto filename = QStringLiteral("speech-note-%1.wav");

    for (int i = 1; i <= 1000; ++i) {
        auto fn = filename.arg(i);
        if (!QFileInfo::exists(dir.filePath(fn))) return fn;
    }

    return filename.arg(1);
}

QString settings::file_open_dir() const {
    auto dir = value(QStringLiteral("file_open_dir")).toString();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    return dir;
}

QUrl settings::file_open_dir_url() const {
    return QUrl::fromLocalFile(file_open_dir());
}

void settings::set_file_open_dir(const QString& value) {
    if (file_open_dir() != value) {
        setValue(QStringLiteral("file_open_dir"), value);
        emit file_open_dir_changed();
    }
}

void settings::set_file_open_dir_url(const QUrl& value) {
    set_file_open_dir(value.toLocalFile());
}

QString settings::file_open_dir_name() const {
    return file_open_dir_url().fileName();
}

settings::mode_t settings::mode() const {
    return static_cast<mode_t>(
        value(QStringLiteral("mode"), static_cast<int>(mode_t::Stt)).toInt());
}

void settings::set_mode(mode_t value) {
    if (mode() != value) {
        setValue(QStringLiteral("mode"), static_cast<int>(value));
        emit mode_changed();
    }
}

settings::speech_mode_t settings::speech_mode() const {
    return static_cast<speech_mode_t>(
        value(QStringLiteral("speech_mode"),
              static_cast<int>(speech_mode_t::SpeechSingleSentence))
            .toInt());
}

void settings::set_speech_mode(speech_mode_t value) {
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

bool settings::translate() const {
    return value(QStringLiteral("translate"), false).toBool();
}

void settings::set_translate(bool value) {
    if (translate() != value) {
        setValue(QStringLiteral("translate"), value);
        emit translate_changed();
    }
}

settings::insert_mode_t settings::insert_mode() const {
    return static_cast<insert_mode_t>(
        value(QStringLiteral("insert_mode"),
              static_cast<int>(insert_mode_t::InsertInLine))
            .toInt());
}

void settings::set_insert_mode(insert_mode_t value) {
    if (insert_mode() != value) {
        setValue(QStringLiteral("insert_mode"), static_cast<int>(value));
        emit insert_mode_changed();
    }
}

QUrl settings::app_icon() const {
#ifdef USE_SFOS
    return QUrl::fromLocalFile(
        QStringLiteral("/usr/share/icons/hicolor/172x172/apps/%1.png")
            .arg(QLatin1String{APP_BINARY_ID}));
#else
    return QUrl{QStringLiteral("qrc:/app_icon.svg")};
#endif
}

bool settings::py_supported() const {
#ifdef USE_PY
    return true;
#else
    return false;
#endif
}

settings::launch_mode_t settings::launch_mode() const { return m_launch_mode; }

void settings::set_launch_mode(launch_mode_t launch_mode) {
    m_launch_mode = launch_mode;
}

QString settings::module_checksum(const QString& name) const {
    return value(QStringLiteral("service/module_%1_checksum").arg(name))
        .toString();
}

void settings::set_module_checksum(const QString& name, const QString& value) {
    if (value != module_checksum(name)) {
        setValue(QStringLiteral("service/module_%1_checksum").arg(name), value);
    }
}

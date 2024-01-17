/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "settings.h"

#ifdef USE_DESKTOP
#include <QQuickStyle>
#endif
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QRegExp>
#include <QStandardPaths>
#include <QVariant>
#include <QVariantList>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

#include "config.h"
#include "mic_source.h"
#ifdef ARCH_X86_64
#include "gpu_tools.hpp"
#endif

#include "module_tools.hpp"

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

static QString audio_format_to_ext(settings::audio_format_t format) {
    switch (format) {
        case settings::audio_format_t::AudioFormatWav:
            return QStringLiteral("wav");
        case settings::audio_format_t::AudioFormatMp3:
            return QStringLiteral("mp3");
        case settings::audio_format_t::AudioFormatOggVorbis:
            return QStringLiteral("ogg");
        case settings::audio_format_t::AudioFormatOggOpus:
            return QStringLiteral("opus");
        case settings::audio_format_t::AudioFormatAuto:
            break;
    }

    return QStringLiteral("mp3");
}

static QString audio_format_to_str(settings::audio_format_t format) {
    switch (format) {
        case settings::audio_format_t::AudioFormatWav:
            return QStringLiteral("wav");
        case settings::audio_format_t::AudioFormatMp3:
            return QStringLiteral("mp3");
        case settings::audio_format_t::AudioFormatOggVorbis:
            return QStringLiteral("ogg_vorbis");
        case settings::audio_format_t::AudioFormatOggOpus:
            return QStringLiteral("ogg_opus");
        case settings::audio_format_t::AudioFormatAuto:
            break;
    }

    return QStringLiteral("mp3");
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
    qDebug() << "platform:" << QGuiApplication::platformName();

    update_addon_flags();
    enforce_num_threads();
    update_audio_inputs();

    // remove qml cache
    QDir{QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
         "/qmlcache"}
        .removeRecursively();
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

QString settings::default_mnt_lang() const {
    return value(QStringLiteral("service/default_mnt_lang"),
                 QStringLiteral("en"))
        .toString();  // english is a default;
}

void settings::set_default_mnt_lang(const QString& value) {
    if (default_mnt_lang() != value) {
        setValue(QStringLiteral("service/default_mnt_lang"), value);
        emit default_mnt_lang_changed();
    }
}

QString settings::default_mnt_out_lang() const {
    return value(QStringLiteral("service/default_mnt_out_lang"),
                 QStringLiteral("en"))
        .toString();  // english is a default;
}

void settings::set_default_mnt_out_lang(const QString& value) {
    if (default_mnt_out_lang() != value) {
        setValue(QStringLiteral("service/default_mnt_out_lang"), value);
        emit default_mnt_out_lang_changed();
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
        sync();
        emit default_stt_models_changed(lang);
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
        sync();
        emit default_tts_models_changed(lang);
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

QString settings::py_path() const {
    return value(QStringLiteral("service/py_path"), {}).toString();
}

void settings::set_py_path(const QString& value) {
    if (py_path() != value) {
        setValue(QStringLiteral("service/py_path"), value);
        emit py_path_changed();
        set_restart_required(true);
    }
}

bool settings::stt_use_gpu() const {
    return value(QStringLiteral("stt_use_gpu"), false).toBool();
}

void settings::set_stt_use_gpu(bool value) {
    if (stt_use_gpu() != value) {
        setValue(QStringLiteral("stt_use_gpu"), value);
        emit stt_use_gpu_changed();
    }
}

bool settings::tts_use_gpu() const {
    return value(QStringLiteral("tts_use_gpu"), false).toBool();
}

void settings::set_tts_use_gpu(bool value) {
    if (tts_use_gpu() != value) {
        setValue(QStringLiteral("tts_use_gpu"), value);
        emit tts_use_gpu_changed();
    }
}

QString settings::default_tts_model_for_mnt_lang(const QString& lang) {
    return value(QStringLiteral("default_tts_model_for_mnt_%1").arg(lang), {})
        .toString();
}

void settings::set_default_tts_model_for_mnt_lang(const QString& lang,
                                                  const QString& value) {
    if (default_tts_model_for_mnt_lang(lang) != value) {
        setValue(QStringLiteral("default_tts_model_for_mnt_%1").arg(lang),
                 value);
        emit default_tts_models_for_mnt_changed(lang);
    }
}

QString settings::active_tts_ref_voice() const {
    return value(QStringLiteral("active_tts_ref_voice"), {}).toString();
}

void settings::set_active_tts_ref_voice(const QString& value) {
    if (active_tts_ref_voice() != value) {
        setValue(QStringLiteral("active_tts_ref_voice"), value);
        emit active_tts_ref_voice_changed();
    }
}

QString settings::active_tts_for_in_mnt_ref_voice() const {
    return value(QStringLiteral("active_tts_for_in_mnt_ref_voice"), {})
        .toString();
}

void settings::set_active_tts_for_in_mnt_ref_voice(const QString& value) {
    if (active_tts_for_in_mnt_ref_voice() != value) {
        setValue(QStringLiteral("active_tts_for_in_mnt_ref_voice"), value);
        emit active_tts_for_in_mnt_ref_voice_changed();
    }
}

QString settings::active_tts_for_out_mnt_ref_voice() const {
    return value(QStringLiteral("active_tts_for_out_mnt_ref_voice"), {})
        .toString();
}

void settings::set_active_tts_for_out_mnt_ref_voice(const QString& value) {
    if (active_tts_for_out_mnt_ref_voice() != value) {
        setValue(QStringLiteral("active_tts_for_out_mnt_ref_voice"), value);
        emit active_tts_for_out_mnt_ref_voice_changed();
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

void settings::update_file_save_path(const QString& path) {
    QFileInfo fi{path};

    if (!fi.baseName().isEmpty()) {
        setValue(QStringLiteral("file_save_last_filename"), fi.fileName());
    }

    set_file_save_dir(fi.absoluteDir().absolutePath());
}

void settings::set_file_save_dir_url(const QUrl& value) {
    set_file_save_dir(value.toLocalFile());
}

QString settings::file_save_dir_name() const {
    return file_save_dir_url().fileName();
}

QString settings::file_save_filename() const {
    auto dir = QDir{file_save_dir()};

    auto filename = value(QStringLiteral("file_save_last_filename")).toString();

    const int max_i = 99999;
    int i = 1;

    if (filename.isEmpty()) {
        filename = QStringLiteral("speech-note-%1.") +
                   audio_format_to_ext(audio_format());
    } else {
        auto ext = audio_ext_from_filename(filename);

        filename = QFileInfo{filename}.baseName();

        if (!QFileInfo::exists(dir.filePath(filename + '.' + ext))) {
            return filename + '.' + ext;
        }

        QRegExp rx{"\\d+$"};
        if (auto idx = rx.indexIn(filename); idx >= 0) {
            bool ok = false;
            auto ii = filename.midRef(idx).toInt(&ok);
            if (ok && ii < max_i) {
                i = ii;
                filename = filename.mid(0, idx) + "%1." + ext;
            } else {
                filename += "-%1." + ext;
            }
        } else if (filename.endsWith('-')) {
            filename += "%1." + ext;
        } else {
            filename += "-%1." + ext;
        }
    }

    for (; i <= max_i; ++i) {
        auto fn = filename.arg(i);
        if (!QFileInfo::exists(dir.filePath(fn))) return fn;
    }

    return filename.arg(1);
}

QString settings::file_open_dir() const {
    auto dir = value(QStringLiteral("file_open_dir")).toString();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        dir =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
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

QString settings::file_audio_open_dir() const {
    auto dir = value(QStringLiteral("file_audio_open_dir")).toString();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    return dir;
}

QUrl settings::file_audio_open_dir_url() const {
    return QUrl::fromLocalFile(file_audio_open_dir());
}

void settings::set_file_audio_open_dir(const QString& value) {
    if (file_audio_open_dir() != value) {
        setValue(QStringLiteral("file_audio_open_dir"), value);
        emit file_audio_open_dir_changed();
    }
}

void settings::set_file_audio_open_dir_url(const QUrl& value) {
    set_file_audio_open_dir(value.toLocalFile());
}

QString settings::file_audio_open_dir_name() const {
    return file_audio_open_dir_url().fileName();
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

unsigned int settings::speech_speed() const {
    return std::clamp(value(QStringLiteral("speech_speed2"), 10u).toUInt(), 1u,
                      20u);
}

void settings::set_speech_speed(unsigned int value) {
    value = std::clamp(value, 1u, 20u);

    if (speech_speed() != value) {
        setValue(QStringLiteral("speech_speed2"), static_cast<int>(value));
        emit speech_speed_changed();
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

int settings::font_size() const {
    return value(QStringLiteral("font_size"), 0).toInt();
}

void settings::set_font_size(int value) {
    if (font_size() != value) {
        setValue(QStringLiteral("font_size"), value);
        emit font_size_changed();
    }
}

void settings::set_audio_format(audio_format_t value) {
    if (audio_format() != value) {
        setValue(QStringLiteral("audio_format"), static_cast<int>(value));
        emit audio_format_changed();
    }
}

settings::audio_format_t settings::audio_format() const {
    return static_cast<audio_format_t>(
        value(QStringLiteral("audio_format"),
              static_cast<int>(audio_format_t::AudioFormatAuto))
            .toInt());
}

void settings::set_audio_quality(audio_quality_t value) {
    if (audio_quality() != value) {
        setValue(QStringLiteral("audio_quality"), static_cast<int>(value));
        emit audio_quality_changed();
        emit file_save_dir_changed();
    }
}

settings::audio_quality_t settings::audio_quality() const {
    return static_cast<audio_quality_t>(
        value(QStringLiteral("audio_quality"),
              static_cast<int>(audio_quality_t::AudioQualityVbrMedium))
            .toInt());
}

void settings::set_mtag_album_name(const QString& value) {
    if (mtag_album_name() != value) {
        setValue(QStringLiteral("mtag_album_name"), value);
        emit mtag_album_name_changed();
    }
}

QString settings::mtag_album_name() const {
    return value(QStringLiteral("mtag_album_name"), tr("Speech notes"))
        .toString();
}

void settings::set_mtag_artist_name(const QString& value) {
    if (mtag_artist_name() != value) {
        setValue(QStringLiteral("mtag_artist_name"), value);
        emit mtag_artist_name_changed();
    }
}

QString settings::mtag_artist_name() const {
    return value(QStringLiteral("mtag_artist_name"), "Speech Note").toString();
}

bool settings::mtag() const {
    return value(QStringLiteral("mtag"), true).toBool();
}

void settings::set_mtag(bool value) {
    if (mtag() != value) {
        setValue(QStringLiteral("mtag"), value);
        emit mtag_changed();
    }
}

bool settings::translator_mode() const {
    return value(QStringLiteral("translator_mode"), false).toBool();
}

void settings::set_translator_mode(bool value) {
    if (translator_mode() != value) {
        setValue(QStringLiteral("translator_mode"), value);
        emit translator_mode_changed();
    }
}

bool settings::translate_when_typing() const {
    return value(QStringLiteral("translate_when_typing"), false).toBool();
}

void settings::set_translate_when_typing(bool value) {
    if (translate_when_typing() != value) {
        setValue(QStringLiteral("translate_when_typing"), value);
        emit translate_when_typing_changed();
    }
}

settings::text_format_t settings::mnt_text_format() const {
    return static_cast<text_format_t>(
        value(QStringLiteral("mnt_text_format"),
              static_cast<int>(text_format_t::TextFormatRaw))
            .toInt());
}

void settings::set_mnt_text_format(text_format_t value) {
    if (mnt_text_format() != value) {
        setValue(QStringLiteral("mnt_text_format"), static_cast<int>(value));
        emit mnt_text_format_changed();
    }
}

settings::text_format_t settings::stt_tts_text_format() const {
    return static_cast<text_format_t>(
        value(QStringLiteral("stt_tts_text_format"),
              static_cast<int>(text_format_t::TextFormatRaw))
            .toInt());
}

void settings::set_stt_tts_text_format(text_format_t value) {
    if (stt_tts_text_format() != value) {
        setValue(QStringLiteral("stt_tts_text_format"),
                 static_cast<int>(value));
        emit stt_tts_text_format_changed();
    }
}

bool settings::hint_translator() const {
    return value(QStringLiteral("hint_translator"), true).toBool();
}

void settings::set_hint_translator(bool value) {
    if (hint_translator() != value) {
        setValue(QStringLiteral("hint_translator"), value);
        emit hint_translator_changed();
    }
}

bool settings::hint_addons() const {
    if (gpu_supported() && is_flatpak() &&
        addon_flags() == addon_flags_t::AddonNone)
        return value(QStringLiteral("hint_addons"), true).toBool();
    else
        return false;
}

void settings::set_hint_addons(bool value) {
    if (hint_addons() != value) {
        setValue(QStringLiteral("hint_addons"), value);
        emit hint_addons_changed();
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

bool settings::qt_style_auto() const {
    return value(QStringLiteral("qt_style_auto"), true).toBool();
}

void settings::set_qt_style_auto(bool value) {
    if (qt_style_auto() != value) {
        setValue(QStringLiteral("qt_style_auto"), value);
        emit qt_style_changed();
        set_restart_required(true);
    }
}

int settings::qt_style_idx() const {
#ifdef USE_DESKTOP
    auto name = qt_style_name();

    auto styles = QQuickStyle::availableStyles();

    if (name.isEmpty()) return styles.size();

    return styles.indexOf(name);
#endif
    return -1;
}

void settings::set_qt_style_idx([[maybe_unused]] int value) {
#ifdef USE_DESKTOP
    auto styles = QQuickStyle::availableStyles();

    if (value < 0 || value >= styles.size()) {
        set_qt_style_name({});
        return;
    }

    set_qt_style_name(styles.at(value));
#endif
}

QString settings::qt_style_name() const {
#ifdef USE_DESKTOP
    auto name =
        value(QStringLiteral("qt_style_name"), default_qt_style).toString();

    if (!QQuickStyle::availableStyles().contains(name)) return {};

    return name;
#else
    return {};
#endif
}

void settings::set_qt_style_name([[maybe_unused]] QString name) {
#ifdef USE_DESKTOP
    if (!QQuickStyle::availableStyles().contains(name)) name.clear();

    if (qt_style_name() != name) {
        setValue(QStringLiteral("qt_style_name"), name);
        emit qt_style_changed();
        set_restart_required(true);
    }
#endif
}

QUrl settings::app_icon() const {
#ifdef USE_SFOS
    return QUrl::fromLocalFile(
        QStringLiteral("/usr/share/icons/hicolor/172x172/apps/%1.png")
            .arg(QLatin1String{APP_BINARY_ID}));
#else
    return QUrl{QStringLiteral("qrc:/app_icon.png")};
#endif
}

bool settings::py_supported() const {
#ifdef USE_PY
    return true;
#else
    return false;
#endif
}

bool settings::gpu_supported() const {
#ifdef ARCH_X86_64
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

QString settings::prev_app_ver() const {
    return value(QStringLiteral("prev_app_ver")).toString();
}

void settings::set_prev_app_ver(const QString& value) {
    if (prev_app_ver() != value) {
        setValue(QStringLiteral("prev_app_ver"), value);
        emit prev_app_ver_changed();
    }
}

bool settings::restart_required() const { return m_restart_required; }

void settings::set_restart_required(bool value) {
    if (restart_required() != value) {
        m_restart_required = value;
        emit restart_required_changed();
    }
}

settings::audio_format_t settings::audio_format_from_filename(
    const QString& filename) {
    if (settings::instance()->audio_format() ==
        settings::audio_format_t::AudioFormatAuto) {
        return filename_to_audio_format_static(filename);
    } else {
        return settings::instance()->audio_format();
    }
}

QString settings::audio_format_str_from_filename(const QString& filename) {
    if (settings::instance()->audio_format() ==
        settings::audio_format_t::AudioFormatAuto) {
        return audio_format_to_str(filename_to_audio_format_static(filename));
    } else {
        return audio_format_to_str(settings::instance()->audio_format());
    }
}

QString settings::audio_ext_from_filename(const QString& filename) {
    if (settings::instance()->audio_format() ==
        settings::audio_format_t::AudioFormatAuto) {
        return audio_format_to_ext(filename_to_audio_format_static(filename));
    } else {
        return audio_format_to_ext(settings::instance()->audio_format());
    }
}

settings::audio_format_t settings::filename_to_audio_format(
    const QString& filename) const {
    return filename_to_audio_format_static(QFileInfo{filename}.fileName());
}

settings::audio_format_t settings::filename_to_audio_format_static(
    const QString& filename) {
    if (filename.endsWith(QLatin1String(".wav"), Qt::CaseInsensitive))
        return audio_format_t::AudioFormatWav;
    if (filename.endsWith(QLatin1String(".mp3"), Qt::CaseInsensitive))
        return audio_format_t::AudioFormatMp3;
    if (filename.endsWith(QLatin1String(".ogg"), Qt::CaseInsensitive) ||
        filename.endsWith(QLatin1String(".oga"), Qt::CaseInsensitive) ||
        filename.endsWith(QLatin1String(".ogx"), Qt::CaseInsensitive))
        return audio_format_t::AudioFormatOggVorbis;
    if (filename.endsWith(QLatin1String(".opus"), Qt::CaseInsensitive))
        return audio_format_t::AudioFormatOggOpus;
    return audio_format_t::AudioFormatAuto;
}

bool settings::file_exists(const QString& file_path) const {
    return QFileInfo::exists(file_path.trimmed());
}

QString settings::add_ext_to_audio_filename(const QString& filename) const {
    QString new_filename = filename.trimmed();

    auto audio_ext = settings::audio_ext_from_filename(filename.trimmed());

    auto sf = new_filename.split('.');

    if (sf.last().toLower() != audio_ext) {
        if (sf.size() > 1) {
            if (sf.last().isEmpty())
                sf.last() = audio_ext;
            else
                sf.last() = std::move(audio_ext);
        } else {
            sf.push_back(std::move(audio_ext));
        }
    }

    new_filename = sf.join('.');

    return new_filename;
}

QString settings::add_ext_to_audio_file_path(const QString& file_path) const {
    QFileInfo fi{file_path.trimmed()};
    return fi.absoluteDir().absoluteFilePath(
        add_ext_to_audio_filename(fi.fileName()));
}

QString settings::base_name_from_file_path(const QString& file_path) const {
    QFileInfo fi{file_path.trimmed()};
    return fi.baseName();
}

QString settings::file_path_from_url(const QUrl& file_url) const {
    return file_url.toLocalFile();
}

QString settings::dir_of_file(const QString& file_path) const {
    return QFileInfo{file_path}.absoluteDir().absolutePath();
}

QString settings::audio_format_str() const {
    return audio_format_to_str(audio_format());
}

QStringList settings::qt_styles() const {
#ifdef USE_DESKTOP
    auto styles = QQuickStyle::availableStyles();
    styles.append(tr("Don't force any style"));
    return styles;
#else
    return {};
#endif
}

#ifdef USE_DESKTOP
static bool use_default_qt_style() {
    const auto* desk_name_str = getenv("XDG_CURRENT_DESKTOP");
    if (!desk_name_str) {
        qDebug() << "no XDG_CURRENT_DESKTOP";
        return false;
    }

    qDebug() << "XDG_CURRENT_DESKTOP:" << desk_name_str;

    QString desk_name{desk_name_str};

    return desk_name.contains("KDE") || desk_name.contains("XFCE");
}

void settings::update_qt_style(QQmlApplicationEngine* engine) const {
    if (auto prefix = module_tools::path_to_dir_for_path(
            QStringLiteral("lib"), QStringLiteral("plugins"));
        !prefix.isEmpty()) {
        QCoreApplication::addLibraryPath(
            QStringLiteral("%1/plugins").arg(prefix));
    }

    if (auto prefix = module_tools::path_to_dir_for_path(QStringLiteral("lib"),
                                                         QStringLiteral("qml"));
        !prefix.isEmpty()) {
        engine->addImportPath(QStringLiteral("%1/qml").arg(prefix));
    }

    if (auto prefix = module_tools::path_to_dir_for_path(
            QStringLiteral("lib"), QStringLiteral("qml/QtQuick/Controls.2"));
        !prefix.isEmpty()) {
        QQuickStyle::addStylePath(
            QStringLiteral("%1/qml/QtQuick/Controls.2").arg(prefix));
    }

    auto styles = QQuickStyle::availableStyles();

    qDebug() << "available styles:" << styles;
    qDebug() << "style paths:" << QQuickStyle::stylePathList();
    qDebug() << "import paths:" << engine->importPathList();
    qDebug() << "library paths:" << QCoreApplication::libraryPaths();

    QString style;

    if (qt_style_auto()) {
        qDebug() << "using auto qt style";

        if (styles.contains(default_qt_style_fallback)) {
            style = use_default_qt_style() && styles.contains(default_qt_style)
                        ? default_qt_style
                        : default_qt_style_fallback;
        } else if (styles.contains(default_qt_style)) {
            style = default_qt_style;
        } else {
            qWarning() << "default qt style not found";
        }
    } else {
        auto idx = qt_style_idx();

        if (idx >= 0 && idx < styles.size()) style = styles.at(idx);

        if (!styles.contains(style)) {
            qWarning() << "qt style not found:" << style;
            style.clear();
        }
    }

    if (style.isEmpty()) {
        qDebug() << "don't forcing any qt style";
    } else {
        qDebug() << "switching to style:" << style;

        QQuickStyle::setStyle(style);
    }

    setenv("QT_QUICK_CONTROLS_HOVER_ENABLED", "1", 1);
}
#endif

void settings::enforce_num_threads() const {
    unsigned int conf_num_threads = num_threads();

    unsigned int num_threads =
        conf_num_threads > 0
            ? std::min(conf_num_threads,
                       std::max(std::thread::hardware_concurrency(), 2u) - 1)
            : 0;

    qDebug() << "enforcing num threads:" << num_threads;

    if (num_threads > 0) {
        setenv("OPENBLAS_NUM_THREADS", std::to_string(num_threads).c_str(), 1);
        setenv("OMP_NUM_THREADS", std::to_string(num_threads).c_str(), 1);
    }
}

QStringList settings::gpu_devices_stt() const { return m_gpu_devices_stt; }

QStringList settings::gpu_devices_tts() const { return m_gpu_devices_tts; }

bool settings::has_gpu_device_stt() const {
    return m_gpu_devices_stt.size() > 1;
}

bool settings::has_gpu_device_tts() const {
    return m_gpu_devices_tts.size() > 1;
}

void settings::scan_gpu_devices() {
#ifdef ARCH_X86_64
    m_gpu_devices_stt.clear();
    m_gpu_devices_tts.clear();
    m_gpu_devices_stt.push_back(tr("Auto"));
    m_gpu_devices_tts.push_back(tr("Auto"));
    m_rocm_gpu_versions.clear();

    qDebug() << "scan cuda:" << gpu_scan_cuda();
    qDebug() << "scan hip:" << gpu_scan_hip();
    qDebug() << "scan opencl:" << gpu_scan_opencl() << gpu_scan_opencl_always();

    auto devices = gpu_tools::available_devices(
        /*cuda=*/gpu_scan_cuda(),
        /*hip=*/gpu_scan_hip(),
        /*opencl=*/gpu_scan_opencl(),
        /*opencl_always=*/gpu_scan_opencl_always());

    std::for_each(devices.cbegin(), devices.cend(), [&](const auto& device) {
        switch (device.api) {
            case gpu_tools::api_t::opencl:
                m_gpu_devices_stt.push_back(
                    QStringLiteral("%1, %2, %3")
                        .arg("OpenCL",
                             QString::fromStdString(device.platform_name),
                             QString::fromStdString(device.name)));
                break;
            case gpu_tools::api_t::cuda: {
                auto item = QStringLiteral("%1, %2, %3")
                                .arg("CUDA", QString::number(device.id),
                                     QString::fromStdString(device.name));
                m_gpu_devices_stt.push_back(item);
                m_gpu_devices_tts.push_back(std::move(item));
                break;
            }
            case gpu_tools::api_t::rocm: {
                auto item = QStringLiteral("%1, %2, %3")
                                .arg("ROCm", QString::number(device.id),
                                     QString::fromStdString(device.name));
                m_gpu_devices_stt.push_back(item);
                m_gpu_devices_tts.push_back(std::move(item));
                m_rocm_gpu_versions.push_back(
                    QString::fromStdString(device.platform_name));
                break;
            }
        }
    });

    if (!auto_gpu_device_stt().isEmpty())
        m_gpu_devices_stt.front().append(" (" + auto_gpu_device_stt() + ")");
    if (!auto_gpu_device_tts().isEmpty())
        m_gpu_devices_tts.front().append(" (" + auto_gpu_device_tts() + ")");

    emit gpu_devices_changed();
#endif
}

QString settings::auto_gpu_device_stt() const {
    return m_gpu_devices_stt.size() <= 1 ? QString{} : m_gpu_devices_stt.at(1);
}

QString settings::auto_gpu_device_tts() const {
    return m_gpu_devices_tts.size() <= 1 ? QString{} : m_gpu_devices_tts.at(1);
}

QString settings::gpu_device_stt() const {
    auto device_str =
        value(QStringLiteral("service/gpu_device_stt")).toString();
    if (std::find(std::next(m_gpu_devices_stt.cbegin()),
                  m_gpu_devices_stt.cend(),
                  device_str) == m_gpu_devices_stt.cend()) {
        return {};
    }

    return device_str;
}

void settings::set_gpu_device_stt(QString value) {
    if (std::find(std::next(m_gpu_devices_stt.cbegin()),
                  m_gpu_devices_stt.cend(), value) == m_gpu_devices_stt.cend())
        value.clear();

    if (value != gpu_device_stt()) {
        setValue(QStringLiteral("service/gpu_device_stt"), value);
        emit gpu_device_stt_changed();
        set_restart_required(true);
    }
}

int settings::gpu_device_idx_stt() const {
    auto current_device = gpu_device_stt();

    if (current_device.isEmpty()) return 0;

    auto it = std::find(m_gpu_devices_stt.cbegin(), m_gpu_devices_stt.cend(),
                        current_device);
    if (it == m_gpu_devices_stt.cend()) return 0;

    return std::distance(m_gpu_devices_stt.cbegin(), it);
}

void settings::set_gpu_device_idx_stt(int value) {
    if (value < 0 || value >= m_gpu_devices_stt.size()) return;
    set_gpu_device_stt(value == 0 ? "" : m_gpu_devices_stt.at(value));
}

QString settings::gpu_device_tts() const {
    auto device_str =
        value(QStringLiteral("service/gpu_device_tts")).toString();
    if (std::find(std::next(m_gpu_devices_tts.cbegin()),
                  m_gpu_devices_tts.cend(),
                  device_str) == m_gpu_devices_tts.cend()) {
        return {};
    }

    return device_str;
}

void settings::set_gpu_device_tts(QString value) {
    if (std::find(std::next(m_gpu_devices_tts.cbegin()),
                  m_gpu_devices_tts.cend(), value) == m_gpu_devices_tts.cend())
        value.clear();

    if (value != gpu_device_tts()) {
        setValue(QStringLiteral("service/gpu_device_tts"), value);
        emit gpu_device_tts_changed();
        set_restart_required(true);
    }
}

int settings::gpu_device_idx_tts() const {
    auto current_device = gpu_device_tts();

    if (current_device.isEmpty()) return 0;

    auto it = std::find(m_gpu_devices_tts.cbegin(), m_gpu_devices_tts.cend(),
                        current_device);
    if (it == m_gpu_devices_tts.cend()) return 0;

    return std::distance(m_gpu_devices_tts.cbegin(), it);
}

void settings::set_gpu_device_idx_tts(int value) {
    if (value < 0 || value >= m_gpu_devices_tts.size()) return;
    set_gpu_device_tts(value == 0 ? "" : m_gpu_devices_tts.at(value));
}

QStringList settings::audio_inputs() const { return m_audio_inputs; }

bool settings::has_audio_input() const { return m_audio_inputs.size() > 1; }

void settings::update_audio_inputs() {
    auto inputs = mic_source::audio_inputs();

    m_audio_inputs.clear();
    m_audio_inputs.push_back(tr("Auto"));

    std::transform(
        inputs.cbegin(), inputs.cend(), std::back_inserter(m_audio_inputs),
        [](const auto& input) { return QStringLiteral("%1").arg(input); });

    emit audio_inputs_changed();
}

QString settings::audio_input() const {
    return value(QStringLiteral("service/audio_input")).toString();
}

void settings::set_audio_input(QString value) {
    if (value == tr("Auto")) value.clear();

    if (value != audio_input()) {
        setValue(QStringLiteral("service/audio_input"), value);
        emit audio_input_changed();
    }
}

int settings::audio_input_idx() const {
    auto current_input = audio_input();

    if (current_input.isEmpty()) return 0;

    auto it = std::find(m_audio_inputs.cbegin(), m_audio_inputs.cend(),
                        current_input);
    if (it == m_audio_inputs.cend()) return 0;

    return std::distance(m_audio_inputs.cbegin(), it);
}

void settings::set_audio_input_idx(int value) {
    if (value < 0 || value >= m_audio_inputs.size()) return;
    set_audio_input(value == 0 ? "" : m_audio_inputs.at(value));
}

bool settings::hotkeys_enabled() const {
    if (!is_xcb()) return false;  // hotkeys are x11 feature
    return value(QStringLiteral("hotkeys_enabled"), false).toBool();
}

void settings::set_hotkeys_enabled(bool value) {
    if (value != hotkeys_enabled()) {
        setValue(QStringLiteral("hotkeys_enabled"), value);
        emit hotkeys_enabled_changed();
    }
}

bool settings::actions_api_enabled() const {
    return value(QStringLiteral("actions_api_enabled"), false).toBool();
}

void settings::set_actions_api_enabled(bool value) {
    if (value != actions_api_enabled()) {
        setValue(QStringLiteral("actions_api_enabled"), value);
        emit actions_api_enabled_changed();
    }
}

bool settings::diacritizer_enabled() const {
    return value(QStringLiteral("diacritizer_enabled"), true).toBool();
}

void settings::set_diacritizer_enabled(bool value) {
    if (value != diacritizer_enabled()) {
        setValue(QStringLiteral("diacritizer_enabled"), value);
        emit diacritizer_enabled_changed();
    }
}

int settings::num_threads() const {
    auto num_threads = value(QStringLiteral("service/num_threads"), 0).toInt();
    return num_threads < 0 ? 0 : num_threads;
}

void settings::set_num_threads(int value) {
    if (value < 1) value = 0;

    if (num_threads() != value) {
        setValue(QStringLiteral("service/num_threads"), value);
        emit num_threads_changed();
        set_restart_required(true);
    }
}

QString settings::hotkey_start_listening() const {
    return value(QStringLiteral("hotkey_start_listening"),
                 QStringLiteral("Ctrl+Alt+Shift+L"))
        .toString();
}

void settings::set_hotkey_start_listening(const QString& value) {
    if (value != hotkey_start_listening()) {
        setValue(QStringLiteral("hotkey_start_listening"), value);
        emit hotkeys_changed();
    }
}

QString settings::hotkey_start_listening_active_window() const {
    return value(QStringLiteral("hotkey_start_listening_active_window"),
                 QStringLiteral("Ctrl+Alt+Shift+K"))
        .toString();
}

void settings::set_hotkey_start_listening_active_window(const QString& value) {
    if (value != hotkey_start_listening_active_window()) {
        setValue(QStringLiteral("hotkey_start_listening_active_window"), value);
        emit hotkeys_changed();
    }
}

QString settings::hotkey_start_listening_clipboard() const {
    return value(QStringLiteral("hotkey_start_listening_clipboard"),
                 QStringLiteral("Ctrl+Alt+Shift+J"))
        .toString();
}

void settings::set_hotkey_start_listening_clipboard(const QString& value) {
    if (value != hotkey_start_listening_clipboard()) {
        setValue(QStringLiteral("hotkey_start_listening_clipboard"), value);
        emit hotkeys_changed();
    }
}

QString settings::hotkey_stop_listening() const {
    return value(QStringLiteral("hotkey_stop_listening"),
                 QStringLiteral("Ctrl+Alt+Shift+S"))
        .toString();
}

void settings::set_hotkey_stop_listening(const QString& value) {
    if (value != hotkey_stop_listening()) {
        setValue(QStringLiteral("hotkey_stop_listening"), value);
        emit hotkeys_changed();
    }
}

QString settings::hotkey_start_reading() const {
    return value(QStringLiteral("hotkey_start_reading"),
                 QStringLiteral("Ctrl+Alt+Shift+R"))
        .toString();
}

void settings::set_hotkey_start_reading(const QString& value) {
    if (value != hotkey_start_reading()) {
        setValue(QStringLiteral("hotkey_start_reading"), value);
        emit hotkeys_changed();
    }
}

QString settings::hotkey_start_reading_clipboard() const {
    return value(QStringLiteral("hotkey_start_reading_clipboard"),
                 QStringLiteral("Ctrl+Alt+Shift+E"))
        .toString();
}

void settings::set_hotkey_start_reading_clipboard(const QString& value) {
    if (value != hotkey_start_reading_clipboard()) {
        setValue(QStringLiteral("hotkey_start_reading_clipboard"), value);
        emit hotkeys_changed();
    }
}

QString settings::hotkey_pause_resume_reading() const {
    return value(QStringLiteral("hotkey_pause_resume_reading"),
                 QStringLiteral("Ctrl+Alt+Shift+P"))
        .toString();
}

void settings::set_hotkey_pause_resume_reading(const QString& value) {
    if (value != hotkey_pause_resume_reading()) {
        setValue(QStringLiteral("hotkey_pause_resume_reading"), value);
        emit hotkeys_changed();
    }
}

QString settings::hotkey_cancel() const {
    return value(QStringLiteral("hotkey_cancel"),
                 QStringLiteral("Ctrl+Alt+Shift+C"))
        .toString();
}

void settings::set_hotkey_cancel(const QString& value) {
    if (value != hotkey_cancel()) {
        setValue(QStringLiteral("hotkey_cancel"), value);
        emit hotkeys_changed();
    }
}

settings::desktop_notification_policy_t settings::desktop_notification_policy()
    const {
    return static_cast<desktop_notification_policy_t>(
        value(QStringLiteral("desktop_notification_policy"),
              static_cast<int>(desktop_notification_policy_t::
                                   DesktopNotificationWhenInacvtive))
            .toInt());
}

void settings::set_desktop_notification_policy(
    desktop_notification_policy_t value) {
    if (value != desktop_notification_policy()) {
        setValue(QStringLiteral("desktop_notification_policy"),
                 static_cast<int>(value));
        emit desktop_notification_policy_changed();
    }
}

bool settings::desktop_notification_details() const {
    return value(QStringLiteral("desktop_notification_details"), false)
        .toBool();
}

void settings::set_desktop_notification_details(bool value) {
    if (value != desktop_notification_details()) {
        setValue(QStringLiteral("desktop_notification_details"), value);
        emit desktop_notification_details_changed();
    }
}

bool settings::gpu_scan_cuda() const {
    return value(QStringLiteral("gpu_scan_cuda"), true).toBool();
}

void settings::set_gpu_scan_cuda(bool value) {
    if (value != gpu_scan_cuda()) {
        setValue(QStringLiteral("gpu_scan_cuda"), value);
        emit gpu_scan_cuda_changed();

        set_restart_required(true);
    }
}

bool settings::mnt_clean_text() const {
    return value(QStringLiteral("mnt_clean_text"), true).toBool();
}

void settings::set_mnt_clean_text(bool value) {
    if (value != mnt_clean_text()) {
        setValue(QStringLiteral("mnt_clean_text"), value);
        emit mnt_clean_text_changed();
    }
}

bool settings::whisper_translate() const {
    return value(QStringLiteral("whisper_translate"), false).toBool();
}

void settings::set_whisper_translate(bool value) {
    if (value != whisper_translate()) {
        setValue(QStringLiteral("whisper_translate"), value);
        emit whisper_translate_changed();
    }
}

bool settings::use_tray() const {
    return value(QStringLiteral("use_tray"), false).toBool();
}

void settings::set_use_tray(bool value) {
    if (value != use_tray()) {
        setValue(QStringLiteral("use_tray"), value);
        emit use_tray_changed();
    }
}

bool settings::clean_ref_voice() const {
    return value(QStringLiteral("clean_ref_voice"), true).toBool();
}

void settings::set_clean_ref_voice(bool value) {
    if (value != clean_ref_voice()) {
        setValue(QStringLiteral("clean_ref_voice"), value);
        emit clean_ref_voice_changed();
    }
}

unsigned int settings::sub_min_segment_dur() const {
    return value(QStringLiteral("sub_min_segment_dur"), 4).toUInt();
}

void settings::set_sub_min_segment_dur(unsigned int value) {
    if (value != sub_min_segment_dur()) {
        setValue(QStringLiteral("sub_min_segment_dur"), value);
        emit sub_config_changed();
    }
}

unsigned int settings::sub_min_line_length() const {
    return value(QStringLiteral("sub_min_line_length"), 30).toUInt();
}

void settings::set_sub_min_line_length(unsigned int value) {
    if (value != sub_min_line_length()) {
        setValue(QStringLiteral("sub_min_line_length"), value);
        emit sub_config_changed();
    }
}

unsigned int settings::sub_max_line_length() const {
    return value(QStringLiteral("sub_max_line_length"), 60).toUInt();
}

void settings::set_sub_max_line_length(unsigned int value) {
    if (value != sub_max_line_length()) {
        setValue(QStringLiteral("sub_max_line_length"), value);
        emit sub_config_changed();
    }
}

bool settings::sub_break_lines() const {
    return value(QStringLiteral("sub_break_lines"), false).toBool();
}

void settings::set_sub_break_lines(bool value) {
    if (value != sub_break_lines()) {
        setValue(QStringLiteral("sub_break_lines"), value);
        emit sub_config_changed();
    }
}

bool settings::start_in_tray() const {
    return value(QStringLiteral("start_in_tray"), false).toBool();
}

void settings::set_start_in_tray(bool value) {
    if (value != start_in_tray()) {
        setValue(QStringLiteral("start_in_tray"), value);
        emit start_in_tray_changed();
    }
}

bool settings::gpu_scan_hip() const {
    return value(QStringLiteral("gpu_scan_hip"), true).toBool();
}

void settings::set_gpu_scan_hip(bool value) {
    if (value != gpu_scan_hip()) {
        setValue(QStringLiteral("gpu_scan_hip"), value);
        emit gpu_scan_hip_changed();

        set_restart_required(true);
    }
}

bool settings::gpu_scan_opencl() const {
    return value(QStringLiteral("gpu_scan_opencl"), true).toBool();
}

void settings::set_gpu_scan_opencl(bool value) {
    if (value != gpu_scan_opencl()) {
        setValue(QStringLiteral("gpu_scan_opencl"), value);
        emit gpu_scan_opencl_changed();

        set_restart_required(true);
    }
}

bool settings::gpu_scan_opencl_always() const {
    return value(QStringLiteral("gpu_scan_opencl_always"), false).toBool();
}

void settings::set_gpu_scan_opencl_always(bool value) {
    if (value != gpu_scan_opencl_always()) {
        setValue(QStringLiteral("gpu_scan_opencl_always"), value);
        emit gpu_scan_opencl_always_changed();

        set_restart_required(true);
    }
}

bool settings::py_feature_scan() const {
    return value(QStringLiteral("service/py_feature_scan"), true).toBool();
}

void settings::set_py_feature_scan(bool value) {
    if (value != py_feature_scan()) {
        setValue(QStringLiteral("service/py_feature_scan"), value);
        emit py_feature_scan_changed();

        set_restart_required(true);
    }
}

void settings::set_cache_audio_format(cache_audio_format_t value) {
    if (cache_audio_format() != value) {
        setValue(QStringLiteral("cache_audio_format"), static_cast<int>(value));
        emit cache_audio_format_changed();
        set_restart_required(true);
    }
}

settings::cache_audio_format_t settings::cache_audio_format() const {
    return static_cast<cache_audio_format_t>(
        value(QStringLiteral("cache_audio_format"),
              static_cast<int>(cache_audio_format_t::CacheAudioFormatOggOpus))
            .toInt());
}

void settings::set_cache_policy(cache_policy_t value) {
    if (cache_policy() != value) {
        setValue(QStringLiteral("cache_policy"), static_cast<int>(value));
        emit cache_policy_changed();
        set_restart_required(true);
    }
}

settings::cache_policy_t settings::cache_policy() const {
    return static_cast<cache_policy_t>(
        value(QStringLiteral("cache_policy"),
              static_cast<int>(cache_policy_t::CacheRemove))
            .toInt());
}

bool settings::gpu_override_version() const {
#ifdef ARCH_X86_64
    return value(QStringLiteral("service/gpu_override_version"), false)
        .toBool();
#else
    return false;
#endif
}

void settings::set_gpu_override_version([[maybe_unused]] bool value) {
#ifdef ARCH_X86_64
    if (gpu_override_version() != value) {
        setValue(QStringLiteral("service/gpu_override_version"), value);
        emit gpu_override_version_changed();
        set_restart_required(true);
    }
#endif
}

QString settings::gpu_overrided_version() {
#ifdef ARCH_X86_64
    auto val =
        value(QStringLiteral("service/gpu_overrided_version"), {}).toString();

    if (val.isEmpty() && !m_rocm_gpu_versions.empty()) {
        val = QString::fromStdString(gpu_tools::rocm_overrided_gfx_version(
            m_rocm_gpu_versions.front().toStdString()));
        setValue(QStringLiteral("service/gpu_overrided_version"), val);
    }

    return val;
#else
    return {};
#endif
}

void settings::set_gpu_overrided_version([[maybe_unused]] QString new_value) {
#ifdef ARCH_X86_64
    auto old_value =
        value(QStringLiteral("service/gpu_overrided_version"), {}).toString();
    if (new_value.isEmpty() && !m_rocm_gpu_versions.empty()) {
        new_value =
            QString::fromStdString(gpu_tools::rocm_overrided_gfx_version(
                m_rocm_gpu_versions.front().toStdString()));
    }

    if (old_value != new_value) {
        setValue(QStringLiteral("service/gpu_overrided_version"), new_value);
        emit gpu_overrided_version_changed();
        set_restart_required(true);
    }
#endif
}

void settings::disable_gpu_scan() {
    set_gpu_scan_cuda(false);
    set_gpu_scan_hip(false);
    set_gpu_scan_opencl(false);
    set_restart_required(false);
}

void settings::disable_py_scan() {
    set_py_feature_scan(false);
    set_restart_required(false);
}

bool settings::is_wayland() const {
    return QGuiApplication::platformName() == "wayland";
}

bool settings::is_xcb() const {
    return QGuiApplication::platformName() == "xcb";
}

bool settings::is_flatpak() const {
#ifdef USE_FLATPAK
    return true;
#endif
    return false;
}

bool settings::is_debug() const {
#ifdef DEBUG
    return true;
#else
    return false;
#endif
}

unsigned int settings::addon_flags() const { return m_addon_flags; }

void settings::update_addon_flags() {
    unsigned int new_flags = addon_flags_t::AddonNone;

#ifdef USE_FLATPAK
    if (QFileInfo::exists(QStringLiteral("/app/extensions/nvidia"))) {
        new_flags |= addon_flags_t::AddonNvidia;
        qDebug() << "nvidia addon exists";
    }
    if (QFileInfo::exists(QStringLiteral("/app/extensions/amd"))) {
        new_flags |= addon_flags_t::AddonAmd;
        qDebug() << "amd addon exists";
    }
#endif

    if (new_flags != m_addon_flags) {
        m_addon_flags = new_flags;
        emit addon_flags_changed();
    }
}

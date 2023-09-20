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
#include <QStandardPaths>
#include <QVariant>
#include <QVariantList>
#include <algorithm>
#include <fstream>
#include <iostream>

#include "config.h"
#include "mic_source.h"
#ifdef ARCH_X86_64
#include "gpu_tools.hpp"
#endif

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

static QString audio_format_to_str(settings::audio_format_t format) {
    switch (format) {
        case settings::audio_format_t::AudioFormatWav:
            return QStringLiteral("wav");
        case settings::audio_format_t::AudioFormatMp3:
            return QStringLiteral("mp3");
        case settings::audio_format_t::AudioFormatOgg:
            return QStringLiteral("ogg");
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

    update_qt_style();
    update_gpu_devices();
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

bool settings::whisper_use_gpu() const {
    return value(QStringLiteral("service/whisper_use_gpu"), false).toBool();
}

void settings::set_whisper_use_gpu(bool value) {
    if (whisper_use_gpu() != value) {
        setValue(QStringLiteral("service/whisper_use_gpu"), value);
        emit whisper_use_gpu_changed();
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

    auto ext = audio_format_to_str(audio_format());

    auto filename = QStringLiteral("speech-note-%1.") + ext;

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

settings::speech_speed_t settings::speech_speed() const {
    return static_cast<speech_speed_t>(
        value(QStringLiteral("speech_speed"),
              static_cast<int>(speech_speed_t::SpeechSpeedNormal))
            .toInt());
}

void settings::set_speech_speed(speech_speed_t value) {
    if (speech_speed() != value) {
        setValue(QStringLiteral("speech_speed"), static_cast<int>(value));
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

bool settings::hint_translator() const {
    return value(QStringLiteral("hint_translator"), true).toBool();
}

void settings::set_hint_translator(bool value) {
    if (hint_translator() != value) {
        setValue(QStringLiteral("hint_translator"), value);
        emit hint_translator_changed();
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

int settings::qt_style_idx() const {
#ifdef USE_DESKTOP
    auto idx = value(QStringLiteral("qt_style_idx2"), 1000).toInt();

    if (idx < 0)
        idx = QQuickStyle::availableStyles().size();
    else if (idx >= 1000)
        idx = QQuickStyle::availableStyles().lastIndexOf(default_qt_style);

    return idx;
#else
    return 0;
#endif
}

void settings::set_qt_style_idx([[maybe_unused]] int value) {
#ifdef USE_DESKTOP
    if (value >= QQuickStyle::availableStyles().size()) value = -1;
    if (qt_style_idx() != value) {
        setValue(QStringLiteral("qt_style_idx2"), value);
        emit qt_style_idx_changed();
        set_restart_required();
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

void settings::set_restart_required() {
    if (!restart_required()) {
        m_restart_required = true;
        emit restart_required_changed();
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
    if (filename.endsWith(QLatin1String(".ogg"), Qt::CaseInsensitive))
        return audio_format_t::AudioFormatOgg;
    return audio_format_t::AudioFormatAuto;
}

bool settings::file_exists(const QString& file_path) const {
    return QFileInfo::exists(file_path.trimmed());
}

QString settings::add_ext_to_audio_filename(const QString& filename) const {
    QString new_filename = filename.trimmed();

    auto audio_format_str =
        settings::audio_format_str_from_filename(filename.trimmed());

    auto sf = new_filename.split('.');

    if (sf.last().toLower() != audio_format_str) {
        if (sf.size() > 1) {
            if (sf.last().isEmpty())
                sf.last() = audio_format_str;
            else
                sf.last() = std::move(audio_format_str);
        } else {
            sf.push_back(std::move(audio_format_str));
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
    styles.append(tr("Don't force"));
    return styles;
#else
    return {};
#endif
}

void settings::update_qt_style() const {
#ifdef USE_DESKTOP
    auto styles = QQuickStyle::availableStyles();

    qDebug() << "available styles:" << styles;
    qDebug() << "style paths:" << QQuickStyle::stylePathList();

    if (qt_style_idx() < 0) return;

    QString style;

    if (qt_style_idx() >= styles.size()) {
        qWarning() << "forcing default style";
        style = default_qt_style;
    } else {
        style = styles.at(qt_style_idx());
    }

    qDebug() << "switching to style:" << style;

    QQuickStyle::setStyle(style);
#endif
}

QStringList settings::gpu_devices() const { return m_gpu_devices; }

bool settings::has_gpu_device() const { return m_gpu_devices.size() > 1; }

void settings::update_gpu_devices() {
#ifdef ARCH_X86_64
    m_gpu_devices.clear();
    m_gpu_devices.push_back(tr("Auto"));

    auto devices = gpu_tools::available_devices();
    std::transform(
        devices.cbegin(), devices.cend(), std::back_inserter(m_gpu_devices),
        [](const auto& device) {
            return [&] {
                switch (device.api) {
                    case gpu_tools::api_t::opencl:
                        return QStringLiteral("%1, %2, %3")
                            .arg("OpenCL",
                                 QString::fromStdString(device.platform_name),
                                 QString::fromStdString(device.name));
                    case gpu_tools::api_t::cuda:
                        return QStringLiteral("%1, %2, %3")
                            .arg("CUDA", QString::number(device.id),
                                 QString::fromStdString(device.name));
                    case gpu_tools::api_t::rocm:
                        return QStringLiteral("%1, %2, %3")
                            .arg("ROCm", QString::number(device.id),
                                 QString::fromStdString(device.name));
                }
                throw std::runtime_error("invalid gpu api");
            }();
        });

    if (!auto_gpu_device().isEmpty())
        m_gpu_devices.front().append(" (" + auto_gpu_device() + ")");

    emit gpu_devices_changed();
#endif
}

QString settings::auto_gpu_device() const {
    return m_gpu_devices.size() <= 1 ? QString{} : m_gpu_devices.at(1);
}

QString settings::gpu_device() const {
    auto device_str = value(QStringLiteral("service/gpu_device")).toString();
    if (std::find(std::next(m_gpu_devices.cbegin()), m_gpu_devices.cend(),
                  device_str) == m_gpu_devices.cend()) {
        return {};
    }

    return device_str;
}

void settings::set_gpu_device(QString value) {
    if (std::find(std::next(m_gpu_devices.cbegin()), m_gpu_devices.cend(),
                  value) == m_gpu_devices.cend())
        value.clear();

    if (value != gpu_device()) {
        setValue(QStringLiteral("service/gpu_device"), value);
        emit gpu_device_changed();
    }
}

int settings::gpu_device_idx() const {
    auto current_device = gpu_device();

    if (current_device.isEmpty()) return 0;

    auto it =
        std::find(m_gpu_devices.cbegin(), m_gpu_devices.cend(), current_device);
    if (it == m_gpu_devices.cend()) return 0;

    return std::distance(m_gpu_devices.cbegin(), it);
}

void settings::set_gpu_device_idx(int value) {
    if (value < 0 || value >= m_gpu_devices.size()) return;
    set_gpu_device(value == 0 ? "" : m_gpu_devices.at(value));
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

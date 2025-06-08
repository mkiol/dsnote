/* Copyright (C) 2021-2025 Michal Kosciesza <michal@mkiol.net>
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
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QRegExp>
#include <QStandardPaths>
#include <QVariant>
#include <QVariantList>
#include <algorithm>
#include <cstdlib>
#include <thread>

#include "config.h"
#include "cpu_tools.hpp"
#include "gpu_tools.hpp"
#include "logger.hpp"
#include "module_tools.hpp"
#include "qtlogger.hpp"

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

static QString text_file_format_to_ext(settings::text_file_format_t format) {
    switch (format) {
        case settings::text_file_format_t::TextFileFormatRaw:
            return QStringLiteral("txt");
        case settings::text_file_format_t::TextFileFormatSrt:
            return QStringLiteral("srt");
        case settings::text_file_format_t::TextFileFormatAss:
            return QStringLiteral("ass");
        case settings::text_file_format_t::TextFileFormatVtt:
            return QStringLiteral("vtt");
        case settings::text_file_format_t::TextFileFormatAuto:
            break;
    }

    return QStringLiteral("txt");
}

static QString video_file_format_to_ext(settings::video_file_format_t format) {
    switch (format) {
        case settings::video_file_format_t::VideoFileFormatMp4:
            return QStringLiteral("mp4");
        case settings::video_file_format_t::VideoFileFormatMkv:
            return QStringLiteral("mkv");
        case settings::video_file_format_t::VideoFileFormatWebm:
            return QStringLiteral("webm");
        case settings::video_file_format_t::VideoFileFormatAuto:
            break;
    }

    return QStringLiteral("mkv");
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

static QStringList string_list_from_list(const QVariantList& list) {
    QStringList slist;

    slist.reserve(list.size());
    for (const auto& item : list) {
        slist.push_back(item.toString());
    }

    return slist;
}

QDebug operator<<(QDebug d, settings::hw_feature_flags_t hw_features) {
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_stt_whispercpp_cuda)
        d << "stt-whispercpp-cuda,";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_stt_whispercpp_hip)
        d << "stt-whispercpp-hip,";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_stt_whispercpp_openvino)
        d << "stt-whispercpp-openvino,";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_stt_whispercpp_opencl)
        d << "stt-whispercpp-opencl,";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_stt_whispercpp_vulkan)
        d << "stt-whispercpp-vulkan,";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_stt_fasterwhisper_cuda)
        d << "stt-fasterwhisper-cuda,";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_stt_fasterwhisper_hip)
        d << "stt-fasterwhisper-hip,";
    if (hw_features & settings::hw_feature_flags_t::hw_feature_tts_coqui_cuda)
        d << "tts-coqui-cuda,";
    if (hw_features & settings::hw_feature_flags_t::hw_feature_tts_coqui_hip)
        d << "tts-coqui-hip";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_tts_whisperspeech_cuda)
        d << "tts-whisperspeech-cuda,";
    if (hw_features &
        settings::hw_feature_flags_t::hw_feature_tts_whisperspeech_hip)
        d << "tts-whisperspeech-hip";
    if (hw_features & settings::hw_feature_flags_t::hw_feature_tts_parler_cuda)
        d << "tts-parler-cuda,";
    if (hw_features & settings::hw_feature_flags_t::hw_feature_tts_parler_hip)
        d << "tts-parler-hip";
    if (hw_features & settings::hw_feature_flags_t::hw_feature_tts_f5_cuda)
        d << "tts-f5-cuda,";
    if (hw_features & settings::hw_feature_flags_t::hw_feature_tts_f5_hip)
        d << "tts-f5-hip";

    return d;
}

QDebug operator<<(QDebug d, settings::trans_rule_type_t type) {
    switch (type) {
        case settings::trans_rule_type_t::TransRuleTypeNone:
            d << "none";
            break;
        case settings::trans_rule_type_t::TransRuleTypeReplaceSimple:
            d << "replace-simple";
            break;
        case settings::trans_rule_type_t::TransRuleTypeReplaceRe:
            d << "replace-re";
            break;
        case settings::trans_rule_type_t::TransRuleTypeMatchSimple:
            d << "match-simple";
            break;
        case settings::trans_rule_type_t::TransRuleTypeMatchRe:
            d << "match-re";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, settings::trans_rule_flags_t flags) {
    if (flags == settings::trans_rule_flags_t::TransRuleNone) {
        d << "none";
    } else {
        if (flags & settings::trans_rule_flags_t::TransRuleMatched)
            d << "matched,";
        if (flags & settings::trans_rule_flags_t::TransRuleTargetStt)
            d << "target-stt,";
        if (flags & settings::trans_rule_flags_t::TransRuleTargetTts)
            d << "target-tts,";
        if (flags & settings::trans_rule_flags_t::TransRuleActionStop)
            d << "action-stop,";
        if (flags & settings::trans_rule_flags_t::TransRuleActionStopListening)
            d << "action-stop-listening,";
        if (flags &
            settings::trans_rule_flags_t::TransRuleActionDeleteLastSentence)
            d << "action-delete-last-sentence,";
        if (flags &
            settings::trans_rule_flags_t::TransRuleActionReadLastSentence)
            d << "action-read-last-sentence,";
    }

    return d;
}

settings::launch_mode_t settings::launch_mode = launch_mode_t::app_stanalone;

settings::settings() : QSettings{settings_filepath(), QSettings::NativeFormat} {
    LOGD("app: " << APP_ORG << " " << APP_ID);
    LOGD("config location: "
         << QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    LOGD("data location: " << QStandardPaths::writableLocation(
             QStandardPaths::AppDataLocation));
    LOGD("cache location: "
         << QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    LOGD("settings file: " << fileName());
    LOGD("platform: " << QGuiApplication::platformName());

    if (launch_mode != launch_mode_t::app) {
        // in app mode, flags are updated in fa
        update_addon_flags();
        update_system_flags();

        enforce_num_threads();
    }

    // remove qml cache
    QDir{QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
         "/qmlcache"}
        .removeRecursively();
}

#define X(name, type, dvalue)                                             \
    type settings::name() const {                                         \
        return qvariant_cast<type>(value(QStringLiteral(#name), dvalue)); \
    }                                                                     \
    void settings::set_##name(type value) {                               \
        if (name() != value) {                                            \
            setValue(QStringLiteral(#name), value);                       \
            emit name##_changed();                                        \
        }                                                                 \
    }
SETTINGS_PROPERTY_TABLE
#undef X

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

static QString file_save_filename(const QDir& dir, QString filename,
                                  const QString& ext) {
    const int max_i = 99999;
    int i = 1;

    if (filename.isEmpty()) {
        filename = QStringLiteral("speech-note-%1.") + ext;
    } else {
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

QString settings::audio_file_save_dir() const {
    auto dir = value(QStringLiteral("audio_file_save_dir")).toString();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    return dir;
}

QUrl settings::audio_file_save_dir_url() const {
    return QUrl::fromLocalFile(audio_file_save_dir());
}

void settings::set_audio_file_save_dir(const QString& value) {
    if (audio_file_save_dir() != value) {
        setValue(QStringLiteral("audio_file_save_dir"), value);
        emit audio_file_save_dir_changed();
    }
}

void settings::update_audio_file_save_path(const QString& path) {
    QFileInfo fi{path};

    if (!fi.baseName().isEmpty()) {
        setValue(QStringLiteral("audio_file_save_last_filename"),
                 fi.fileName());
    }

    set_audio_file_save_dir(fi.absoluteDir().absolutePath());
}

void settings::set_audio_file_save_dir_url(const QUrl& value) {
    set_audio_file_save_dir(value.toLocalFile());
}

QString settings::audio_file_save_dir_name() const {
    return audio_file_save_dir_url().fileName();
}

QString settings::audio_file_save_filename() const {
    auto filename =
        value(QStringLiteral("audio_file_save_last_filename")).toString();

    return file_save_filename(
        QDir{audio_file_save_dir()}, filename,
        filename.isEmpty() ? audio_format_to_ext(audio_format())
                           : audio_ext_from_filename(audio_format(), filename));
}

QString settings::video_file_save_dir() const {
    auto dir = value(QStringLiteral("video_file_save_dir")).toString();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    return dir;
}

QUrl settings::video_file_save_dir_url() const {
    return QUrl::fromLocalFile(video_file_save_dir());
}

void settings::set_video_file_save_dir(const QString& value) {
    if (video_file_save_dir() != value) {
        setValue(QStringLiteral("video_file_save_dir"), value);
        emit video_file_save_dir_changed();
    }
}

void settings::update_video_file_save_path(const QString& path) {
    QFileInfo fi{path};

    if (!fi.baseName().isEmpty()) {
        setValue(QStringLiteral("video_file_save_last_filename"),
                 fi.fileName());
    }

    set_video_file_save_dir(fi.absoluteDir().absolutePath());
}

void settings::set_video_file_save_dir_url(const QUrl& value) {
    set_video_file_save_dir(value.toLocalFile());
}

QString settings::video_file_save_dir_name() const {
    return video_file_save_dir_url().fileName();
}

QString settings::video_file_save_filename() const {
    auto filename =
        value(QStringLiteral("video_file_save_last_filename")).toString();

    return file_save_filename(
        QDir{video_file_save_dir()}, filename,
        filename.isEmpty() ? video_file_format_to_ext(video_file_format())
                           : video_file_ext_from_filename(filename));
}

QString settings::text_file_save_dir() const {
    auto dir = value(QStringLiteral("text_file_save_dir")).toString();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }

    return dir;
}

QUrl settings::text_file_save_dir_url() const {
    return QUrl::fromLocalFile(text_file_save_dir());
}

void settings::set_text_file_save_dir(const QString& value) {
    if (text_file_save_dir() != value) {
        setValue(QStringLiteral("text_file_save_dir"), value);
        emit text_file_save_dir_changed();
    }
}

void settings::update_text_file_save_path(const QString& path) {
    QFileInfo fi{path};

    if (!fi.baseName().isEmpty()) {
        setValue(QStringLiteral("text_file_save_last_filename"), fi.fileName());
    }

    set_text_file_save_dir(fi.absoluteDir().absolutePath());
}

void settings::set_text_file_save_dir_url(const QUrl& value) {
    set_text_file_save_dir(value.toLocalFile());
}

QString settings::text_file_save_dir_name() const {
    return text_file_save_dir_url().fileName();
}

QString settings::text_file_save_filename() const {
    auto filename =
        value(QStringLiteral("text_file_save_last_filename")).toString();

    return file_save_filename(QDir{text_file_save_dir()}, filename,
                              filename.isEmpty()
                                  ? text_file_format_to_ext(text_file_format())
                                  : text_file_ext_from_filename(filename));
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
    return std::clamp(value(QStringLiteral("speech_speed2"), 10U).toUInt(), 1U,
                      20U);
}

void settings::set_speech_speed(unsigned int value) {
    value = std::clamp(value, 1U, 20U);

    if (speech_speed() != value) {
        setValue(QStringLiteral("speech_speed2"), static_cast<int>(value));
        emit speech_speed_changed();
    }
}

QString settings::note() const {
    if (keep_last_note())
        return value(QStringLiteral("note"), {}).toString();
    else
        return m_note;
}

void settings::set_note(const QString& value) {
    if (note() != value) {
        if (keep_last_note())
            setValue(QStringLiteral("note"), value);
        else
            m_note = value;
        emit note_changed();
    }
}

QFont settings::notepad_font() const {
    QFont font;
    font.fromString(value(QStringLiteral("notepad_font"), 0).toString());
    return font;
}

void settings::set_notepad_font(const QFont& value) {
    if (notepad_font() != value) {
        setValue(QStringLiteral("notepad_font"), value.toString());
        emit notepad_font_changed();
    }
}

void settings::set_text_file_format(text_file_format_t value) {
    if (text_file_format() != value) {
        setValue(QStringLiteral("text_file_format"), static_cast<int>(value));
        emit text_file_format_changed();
    }
}

settings::text_file_format_t settings::text_file_format() const {
    if (!subtitles_support()) return text_file_format_t::TextFileFormatRaw;

    return static_cast<text_file_format_t>(
        value(QStringLiteral("text_file_format"),
              static_cast<int>(text_file_format_t::TextFileFormatAuto))
            .toInt());
}

void settings::set_video_file_format(video_file_format_t value) {
    if (video_file_format() != value) {
        setValue(QStringLiteral("video_file_format"), static_cast<int>(value));
        emit video_file_format_changed();
    }
}

settings::video_file_format_t settings::video_file_format() const {
    return static_cast<video_file_format_t>(
        value(QStringLiteral("video_file_format"),
              static_cast<int>(video_file_format_t::VideoFileFormatAuto))
            .toInt());
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
        emit audio_file_save_dir_changed();
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

settings::text_format_t settings::mnt_text_format() const {
    if (subtitles_support()) {
        return static_cast<text_format_t>(
            value(QStringLiteral("mnt_text_format"),
                  static_cast<int>(text_format_t::TextFormatRaw))
                .toInt());
    }

    return text_format_t::TextFormatRaw;
}

void settings::set_mnt_text_format(text_format_t value) {
    if (mnt_text_format() != value) {
        setValue(QStringLiteral("mnt_text_format"), static_cast<int>(value));
        emit mnt_text_format_changed();
    }
}

settings::text_format_t settings::stt_tts_text_format() const {
    if (subtitles_support()) {
        return static_cast<text_format_t>(
            value(QStringLiteral("stt_tts_text_format"),
                  static_cast<int>(text_format_t::TextFormatRaw))
                .toInt());
    }

    return text_format_t::TextFormatRaw;
}

void settings::set_stt_tts_text_format(text_format_t value) {
    if (stt_tts_text_format() != value) {
        setValue(QStringLiteral("stt_tts_text_format"),
                 static_cast<int>(value));
        emit stt_tts_text_format_changed();
    }
}

unsigned int settings::hint_done_flags() const {
    return value(QStringLiteral("hint_done_flags"), 0).toUInt();
}

void settings::set_hint_done(settings::hint_done_flags_t value) {
    auto flags = hint_done_flags() | value;

    if (hint_done_flags() != flags) {
        setValue(QStringLiteral("hint_done_flags"), flags);
        emit hint_done_flags_changed();
    }
}

settings::insert_mode_t settings::insert_mode() const {
    auto val = static_cast<insert_mode_t>(
        value(QStringLiteral("insert_mode"),
              static_cast<int>(insert_mode_t::InsertNewLine))
            .toInt());
#ifdef USE_SFOS
    // InsertAtCursor is not supported on SFOS
    if (val == insert_mode_t::InsertAtCursor)
        val = insert_mode_t::InsertNewLine;
#endif
    return val;
}

void settings::set_insert_mode(insert_mode_t value) {
    if (insert_mode() != value) {
        setValue(QStringLiteral("insert_mode"), static_cast<int>(value));
        emit insert_mode_changed();
    }
}

QString settings::x11_compose_file() const {
    return value(QStringLiteral("x11_compose_file"), {}).toString();
}

void settings::set_x11_compose_file(const QString& value) {
    if (x11_compose_file() != value) {
        setValue(QStringLiteral("x11_compose_file"), value);
        emit x11_compose_file_changed();
    }
}

QString settings::fake_keyboard_layout() const {
    return value(QStringLiteral("fake_keyboard_layout"), {}).toString();
}

void settings::set_fake_keyboard_layout(const QString& value) {
    if (fake_keyboard_layout() != value) {
        setValue(QStringLiteral("fake_keyboard_layout"), value);
        emit fake_keyboard_layout_changed();
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

bool settings::hw_accel_supported() const {
#ifndef ARCH_ARM_32
    return true;
#else
    return false;
#endif
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
    if (instance()->audio_format() == audio_format_t::AudioFormatAuto) {
        return filename_to_audio_format_static(filename);
    }

    return instance()->audio_format();
}

settings::audio_format_t settings::audio_format_from_filename(
    audio_format_t audio_format, const QString& filename) {
    if (audio_format == audio_format_t::AudioFormatAuto)
        audio_format = instance()->audio_format();

    if (audio_format == audio_format_t::AudioFormatAuto) {
        auto real_format = filename_to_audio_format_static(filename);
        return real_format == audio_format_t::AudioFormatAuto
                   ? audio_format_t::AudioFormatMp3
                   : real_format;
    }

    return audio_format;
}

QString settings::audio_format_str_from_filename(audio_format_t audio_format,
                                                 const QString& filename) {
    if (audio_format == audio_format_t::AudioFormatAuto)
        audio_format = instance()->audio_format();

    if (audio_format == audio_format_t::AudioFormatAuto) {
        return audio_format_to_str(filename_to_audio_format_static(filename));
    }

    return audio_format_to_str(audio_format);
}

QString settings::audio_ext_from_filename(audio_format_t audio_format,
                                          const QString& filename) {
    if (audio_format == audio_format_t::AudioFormatAuto)
        audio_format = instance()->audio_format();

    if (instance()->audio_format() == audio_format_t::AudioFormatAuto) {
        return audio_format_to_ext(filename_to_audio_format_static(filename));
    }

    return audio_format_to_ext(audio_format);
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

QString settings::text_file_ext_from_filename(const QString& filename) {
    if (!settings::instance()->subtitles_support())
        return text_file_format_to_ext(
            settings::text_file_format_t::TextFileFormatRaw);

    if (settings::instance()->text_file_format() ==
        settings::text_file_format_t::TextFileFormatAuto) {
        return text_file_format_to_ext(
            filename_to_text_file_format_static(filename));
    } else {
        return text_file_format_to_ext(
            settings::instance()->text_file_format());
    }
}

settings::text_file_format_t settings::filename_to_text_file_format(
    const QString& filename) const {
    return filename_to_text_file_format_static(QFileInfo{filename}.fileName());
}

settings::text_file_format_t settings::filename_to_text_file_format_static(
    const QString& filename) {
    if (!settings::instance()->subtitles_support())
        return text_file_format_t::TextFileFormatRaw;

    if (filename.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive))
        return text_file_format_t::TextFileFormatRaw;
    if (filename.endsWith(QLatin1String(".srt"), Qt::CaseInsensitive))
        return text_file_format_t::TextFileFormatSrt;
    if (filename.endsWith(QLatin1String(".ass"), Qt::CaseInsensitive))
        return text_file_format_t::TextFileFormatAss;
    if (filename.endsWith(QLatin1String(".vtt"), Qt::CaseInsensitive))
        return text_file_format_t::TextFileFormatVtt;

    return text_file_format_t::TextFileFormatAuto;
}

QString settings::video_file_ext_from_filename(const QString& filename) {
    if (settings::instance()->video_file_format() ==
        settings::video_file_format_t::VideoFileFormatAuto) {
        return video_file_format_to_ext(
            filename_to_video_file_format_static(filename));
    } else {
        return video_file_format_to_ext(
            settings::instance()->video_file_format());
    }
}

settings::video_file_format_t settings::filename_to_video_file_format(
    const QString& filename) const {
    return filename_to_video_file_format_static(QFileInfo{filename}.fileName());
}

settings::video_file_format_t settings::filename_to_video_file_format_static(
    const QString& filename) {
    if (filename.endsWith(QLatin1String(".mp4"), Qt::CaseInsensitive))
        return video_file_format_t::VideoFileFormatMp4;
    if (filename.endsWith(QLatin1String(".mkv"), Qt::CaseInsensitive))
        return video_file_format_t::VideoFileFormatMkv;
    if (filename.endsWith(QLatin1String(".webm"), Qt::CaseInsensitive))
        return video_file_format_t::VideoFileFormatWebm;
    return video_file_format_t::VideoFileFormatAuto;
}

bool settings::file_exists(const QString& file_path) const {
    return QFileInfo::exists(file_path.trimmed());
}

static QString add_ext_to_filename(QString filename, QString ext) {
    auto sf = filename.split('.');

    if (sf.last().toLower() != ext) {
        if (sf.size() > 1) {
            if (sf.last().isEmpty())
                sf.last() = ext;
            else
                sf.last() = std::move(ext);
        } else {
            sf.push_back(std::move(ext));
        }
    }

    filename = sf.join('.');

    return filename;
}

QString settings::add_ext_to_audio_filename(const QString& filename) const {
    return add_ext_to_filename(
        filename.trimmed(),
        audio_ext_from_filename(settings::instance()->audio_format(),
                                filename.trimmed()));
}

QString settings::add_ext_to_audio_file_path(const QString& file_path) const {
    QFileInfo fi{file_path.trimmed()};
    return fi.absoluteDir().absoluteFilePath(
        add_ext_to_audio_filename(fi.fileName()));
}

QString settings::add_ext_to_text_file_filename(const QString& filename) const {
    return add_ext_to_filename(filename.trimmed(),
                               text_file_ext_from_filename(filename.trimmed()));
}

QString settings::add_ext_to_text_file_path(const QString& file_path) const {
    QFileInfo fi{file_path.trimmed()};
    return fi.absoluteDir().absoluteFilePath(
        add_ext_to_text_file_filename(fi.fileName()));
}

QString settings::add_ext_to_video_file_filename(
    const QString& filename) const {
    return add_ext_to_filename(
        filename.trimmed(), video_file_ext_from_filename(filename.trimmed()));
}

QString settings::add_ext_to_video_file_path(const QString& file_path) const {
    QFileInfo fi{file_path.trimmed()};
    return fi.absoluteDir().absoluteFilePath(
        add_ext_to_video_file_filename(fi.fileName()));
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

bool settings::is_native_style() const { return m_native_style; }

#ifdef USE_DESKTOP
bool settings::use_default_qt_style() {
    const auto* desk_name_str = getenv("XDG_CURRENT_DESKTOP");
    if (!desk_name_str) {
        LOGD("no XDG_CURRENT_DESKTOP");
        return false;
    }

    LOGD("XDG_CURRENT_DESKTOP: " << desk_name_str);

    QString desk_name{desk_name_str};

    m_kde = desk_name.contains("KDE");

    return m_kde || desk_name.contains("XFCE");
}

void settings::update_qt_style(QQmlApplicationEngine* engine) {
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

    LOGD("available styles: " << styles);
    LOGD("style paths: " << QQuickStyle::stylePathList());
    LOGD("import paths: " << engine->importPathList());
    LOGD("library paths: " << QCoreApplication::libraryPaths());

    QString style;

    if (qt_style_auto()) {
        LOGD("using auto qt style");

        if (styles.contains(default_qt_style_fallback)) {
            style = use_default_qt_style() && styles.contains(default_qt_style)
                        ? default_qt_style
                        : default_qt_style_fallback;
        } else if (styles.contains(default_qt_style)) {
            style = default_qt_style;
        } else {
            LOGW("default qt style not found");
        }
    } else {
        auto idx = qt_style_idx();

        if (idx >= 0 && idx < styles.size()) style = styles.at(idx);

        if (!styles.contains(style)) {
            LOGW("qt style not found: " << style);
            style.clear();
        }
    }

    if (style.isEmpty()) {
        LOGD("don't forcing any qt style");
        m_native_style = true;
    } else {
        LOGD("switching to style: " << style);

        QQuickStyle::setStyle(style);

        m_native_style = style == default_qt_style;
    }
}
#endif

void settings::enforce_num_threads() const {
    unsigned int conf_num_threads = num_threads();

    unsigned int num_threads =
        conf_num_threads > 0
            ? std::min(conf_num_threads,
                       std::max(std::thread::hardware_concurrency(), 2U) - 1)
            : 0;

    LOGD("enforcing num threads: " << num_threads);

    if (num_threads > 0) {
        setenv("OPENBLAS_NUM_THREADS", std::to_string(num_threads).c_str(), 1);
        setenv("OMP_NUM_THREADS", std::to_string(num_threads).c_str(), 1);
    }
}
void settings::update_hw_devices_from_fa(
    const QVariantMap& features_availability) {
#define X(name, _)                                                       \
    if (features_availability.contains(#name "-gpu-devices")) {          \
        m_##name##_gpu_devices = string_list_from_list(                  \
            features_availability.value(#name "-gpu-devices").toList()); \
        LOGD(#name "-gpu-devices from fa: " << m_##name##_gpu_devices);  \
    } else {                                                             \
        LOGD("no " #name "-gpu-devices from fa");                        \
    }
    GPU_ENGINE_TABLE
#undef X

    emit gpu_devices_changed();
}

void settings::scan_hw_devices(unsigned int hw_feature_flags) {
#define X(name, _)                  \
    m_##name##_gpu_devices.clear(); \
    m_##name##_gpu_devices.push_back(tr("Auto"));
    GPU_ENGINE_TABLE
#undef X

    m_rocm_gpu_versions.clear();

#define X(name, dvalue) LOGD("scan " #name ":" << hw_scan_##name());
    GPU_SCAN_TABLE
#undef X
    LOGD("hw feature flags: "
         << static_cast<hw_feature_flags_t>(hw_feature_flags));

    bool disable_fasterwhisper_cuda =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_stt_fasterwhisper_cuda) == 0;
    bool disable_fasterwhisper_hip =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_stt_fasterwhisper_hip) == 0;
    bool disable_whispercpp_cuda =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_stt_whispercpp_cuda) == 0;
    bool disable_whispercpp_openvino =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_stt_whispercpp_openvino) == 0;
    bool disable_whispercpp_hip =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_stt_whispercpp_hip) == 0;
    bool disable_whispercpp_opencl =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_stt_whispercpp_opencl) == 0;
    bool disable_whispercpp_vulkan =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_stt_whispercpp_vulkan) == 0;
    bool disable_coqui_cuda =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_coqui_cuda) == 0;
    bool disable_coqui_hip =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_coqui_hip) == 0;
    bool disable_whisperspeech_cuda =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_tts_whisperspeech_cuda) == 0;
    bool disable_whisperspeech_hip =
        (hw_feature_flags &
         hw_feature_flags_t::hw_feature_tts_whisperspeech_hip) == 0;
    bool disable_parler_cuda =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_parler_cuda) ==
        0;
    bool disable_parler_hip =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_parler_hip) == 0;
    bool disable_f5_cuda =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_f5_cuda) == 0;
    bool disable_f5_hip =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_f5_hip) == 0;
    bool disable_kokoro_cuda =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_kokoro_cuda) ==
        0;
    bool disable_kokoro_hip =
        (hw_feature_flags & hw_feature_flags_t::hw_feature_tts_kokoro_hip) == 0;

    auto result = gpu_tools::available_devices(
        /*cuda=*/hw_scan_cuda(),
        /*hip=*/hw_scan_hip(),
        /*vulkan=*/hw_scan_vulkan(),
        /*vulkan_igpu=*/hw_scan_vulkan_igpu(),
        /*vulkan_cpu=*/hw_scan_vulkan_cpu(),
        /*openvino=*/hw_scan_openvino(),
        /*openvino_gpu=*/hw_scan_openvino_gpu(),
        /*opencl=*/hw_scan_opencl(),
        /*opencl_clover=*/hw_scan_opencl_legacy());

    std::for_each(
        result.devices.cbegin(), result.devices.cend(),
        [&](const auto& device) {
            switch (device.api) {
                case gpu_tools::api_t::opencl:
                    if (disable_whispercpp_opencl) return;
                    m_whispercpp_gpu_devices.push_back(
                        QStringLiteral("%1, %2, %3")
                            .arg(
                                "OpenCL",
                                QString::fromStdString(device.platform_name)
                                    .trimmed(),
                                QString::fromStdString(device.name).trimmed()));
                    break;
                case gpu_tools::api_t::cuda: {
                    if (disable_fasterwhisper_cuda && disable_whispercpp_cuda &&
                        disable_coqui_cuda && disable_whisperspeech_cuda &&
                        disable_parler_cuda && disable_f5_cuda &&
                        disable_kokoro_cuda)
                        return;
                    auto item =
                        QStringLiteral("%1, %2, %3")
                            .arg("CUDA", QString::number(device.id),
                                 QString::fromStdString(device.name).trimmed());
                    if (!disable_fasterwhisper_cuda)
                        m_fasterwhisper_gpu_devices.push_back(item);
                    if (!disable_whispercpp_cuda)
                        m_whispercpp_gpu_devices.push_back(item);
                    if (!disable_coqui_cuda)
                        m_coqui_gpu_devices.push_back(item);
                    if (!disable_whisperspeech_cuda)
                        m_whisperspeech_gpu_devices.push_back(item);
                    if (!disable_parler_cuda)
                        m_parler_gpu_devices.push_back(item);
                    if (!disable_f5_cuda) m_f5_gpu_devices.push_back(item);
                    if (!disable_kokoro_cuda)
                        m_kokoro_gpu_devices.push_back(item);
                    break;
                }
                case gpu_tools::api_t::rocm: {
                    if (disable_fasterwhisper_hip && disable_whispercpp_hip &&
                        disable_coqui_hip && disable_whisperspeech_hip &&
                        disable_parler_hip && disable_f5_hip &&
                        disable_kokoro_hip)
                        return;
                    auto item =
                        QStringLiteral("%1, %2, %3")
                            .arg("ROCm", QString::number(device.id),
                                 QString::fromStdString(device.name).trimmed());
                    if (!disable_fasterwhisper_hip)
                        m_fasterwhisper_gpu_devices.push_back(item);
                    if (!disable_whispercpp_hip)
                        m_whispercpp_gpu_devices.push_back(item);
                    if (!disable_coqui_hip) m_coqui_gpu_devices.push_back(item);
                    if (!disable_whisperspeech_hip)
                        m_whisperspeech_gpu_devices.push_back(item);
                    if (!disable_parler_hip)
                        m_parler_gpu_devices.push_back(item);
                    if (!disable_f5_hip) m_f5_gpu_devices.push_back(item);
                    if (!disable_kokoro_hip)
                        m_kokoro_gpu_devices.push_back(item);
                    m_rocm_gpu_versions.push_back(
                        QString::fromStdString(device.platform_name));
                    break;
                }
                case gpu_tools::api_t::openvino:
                    if (disable_whispercpp_openvino) return;
                    m_whispercpp_gpu_devices.push_back(
                        QStringLiteral("%1, %2, %3")
                            .arg("OpenVINO",
                                 QString::fromStdString(device.name).trimmed(),
                                 QString::fromStdString(device.platform_name)
                                     .trimmed()));
                    break;
                case gpu_tools::api_t::vulkan:
                    if (disable_whispercpp_vulkan) return;
                    m_whispercpp_gpu_devices.push_back(
                        QStringLiteral("%1, %2, %3")
                            .arg(
                                "Vulkan", QString::number(device.id),
                                QString::fromStdString(device.name).trimmed()));
                    break;
            }
        });

#define X(name, _)                                   \
    if (!name##_auto_gpu_device().isEmpty())         \
        m_##name##_gpu_devices.front().append(" (" + \
                                              name##_auto_gpu_device() + ")");
    GPU_ENGINE_TABLE
#undef X

    emit gpu_devices_changed();

    if (result.error == gpu_tools::error_t::cuda_uknown_error &&
        (!is_flatpak() || addon_flags() & addon_flags_t::AddonNvidia) > 0) {
        LOGW("*********************************************");
        LOGW(
            "Most likely, NVIDIA kernel module has not been fully "
            "initialized. Try executing 'nvidia-modprobe -c 0 -u' "
            "before running Speech Note");
        LOGW("*********************************************");
        add_error_flags(error_flags_t::ErrorCudaUnknown);
    }

    update_system_flags();
}

#define X(name)                                                             \
    bool settings::name##_autolang_with_sup() const {                       \
        return value(QStringLiteral("service/" #name "_autolang_with_sup"), \
                     true)                                                  \
            .toBool();                                                      \
    }                                                                       \
    void settings::set_##name##_autolang_with_sup(bool value) {             \
        if (name##_autolang_with_sup() != value) {                          \
            setValue(QStringLiteral("service/" #name "_autolang_with_sup"), \
                     value);                                                \
            emit name##_changed();                                          \
            set_restart_required(true);                                     \
        }                                                                   \
    }
X(whispercpp)
#undef X

#define X(name)                                                                \
    bool settings::name##_gpu_flash_attn() const {                             \
        return value(QStringLiteral("service/" #name "_gpu_flash_attn"),       \
                     false)                                                    \
            .toBool();                                                         \
    }                                                                          \
    void settings::set_##name##_gpu_flash_attn(bool value) {                   \
        if (name##_gpu_flash_attn() != value) {                                \
            setValue(QStringLiteral("service/" #name "_gpu_flash_attn"),       \
                     value);                                                   \
            emit name##_changed();                                             \
            set_restart_required(true);                                        \
        }                                                                      \
    }                                                                          \
    void settings::reset_##name##_gpu_flash_attn() {                           \
        set_##name##_gpu_flash_attn(false);                                    \
    }                                                                          \
    int settings::name##_cpu_threads() const {                                 \
        return std::clamp<int>(                                                \
            value(QStringLiteral("service/" #name "_cpu_threads"), 4).toInt(), \
            1, std::thread::hardware_concurrency());                           \
    }                                                                          \
    void settings::set_##name##_cpu_threads(int value) {                       \
        if (name##_cpu_threads() != value) {                                   \
            setValue(QStringLiteral("service/" #name "_cpu_threads"), value);  \
            emit name##_changed();                                             \
            set_restart_required(true);                                        \
        }                                                                      \
    }                                                                          \
    void settings::reset_##name##_cpu_threads() {                              \
        set_##name##_cpu_threads(4);                                           \
    }                                                                          \
    int settings::name##_beam_search() const {                                 \
        return std::clamp<int>(                                                \
            value(QStringLiteral("service/" #name "_beam_search"), 1).toInt(), \
            1, 100);                                                           \
    }                                                                          \
    void settings::set_##name##_beam_search(int value) {                       \
        if (name##_beam_search() != value) {                                   \
            setValue(QStringLiteral("service/" #name "_beam_search"), value);  \
            emit name##_changed();                                             \
            set_restart_required(true);                                        \
        }                                                                      \
    }                                                                          \
    void settings::reset_##name##_beam_search() {                              \
        set_##name##_beam_search(1);                                           \
    }                                                                          \
    settings::option_t settings::name##_audioctx_size() const {                \
        return static_cast<option_t>(                                          \
            value(QStringLiteral("service/" #name "_audioctx_size"),           \
                  static_cast<int>(option_t::OptionAuto))                      \
                .toInt());                                                     \
    }                                                                          \
    void settings::set_##name##_audioctx_size(option_t value) {                \
        if (name##_audioctx_size() != value) {                                 \
            setValue(QStringLiteral("service/" #name "_audioctx_size"),        \
                     static_cast<int>(value));                                 \
            emit name##_changed();                                             \
            set_restart_required(true);                                        \
        }                                                                      \
    }                                                                          \
    void settings::reset_##name##_audioctx_size() {                            \
        set_##name##_audioctx_size(option_t::OptionAuto);                      \
    }                                                                          \
    int settings::name##_audioctx_size_value() const {                         \
        return value(QStringLiteral("service/" #name "_audioctx_size_value"),  \
                     1500)                                                     \
            .toInt();                                                          \
    }                                                                          \
    void settings::set_##name##_audioctx_size_value(int value) {               \
        if (name##_audioctx_size_value() != value) {                           \
            setValue(QStringLiteral("service/" #name "_audioctx_size_value"),  \
                     value);                                                   \
            emit name##_changed();                                             \
            set_restart_required(true);                                        \
        }                                                                      \
    }                                                                          \
    settings::engine_profile_t settings::name##_profile() const {              \
        return static_cast<engine_profile_t>(                                  \
            value(                                                             \
                QStringLiteral("service/" #name "_profile"),                   \
                static_cast<int>(engine_profile_t::EngineProfilePerformance))  \
                .toInt());                                                     \
    }                                                                          \
    void settings::set_##name##_profile(engine_profile_t value) {              \
        if (name##_profile() != value) {                                       \
            setValue(QStringLiteral("service/" #name "_profile"),              \
                     static_cast<int>(value));                                 \
            emit name##_changed();                                             \
            set_restart_required(true);                                        \
        }                                                                      \
    }                                                                          \
    void settings::reset_##name##_audioctx_size_value() {                      \
        set_##name##_audioctx_size_value(1500);                                \
    }                                                                          \
    void settings::reset_##name##_options() {                                  \
        reset_##name##_gpu_flash_attn();                                       \
        reset_##name##_cpu_threads();                                          \
        reset_##name##_beam_search();                                          \
        set_##name##_use_gpu(false);                                           \
        reset_##name##_audioctx_size();                                        \
        reset_##name##_audioctx_size_value();                                  \
    }
X(whispercpp)
X(fasterwhisper)
#undef X

#define X(name, enabled)                                                      \
    bool settings::name##_use_gpu() const {                                   \
        return value(QStringLiteral(#name "_use_gpu"), enabled).toBool();     \
    }                                                                         \
    void settings::set_##name##_use_gpu(bool value) {                         \
        if (name##_use_gpu() != value) {                                      \
            setValue(QStringLiteral(#name "_use_gpu"), value);                \
            emit name##_use_gpu_changed();                                    \
        }                                                                     \
    }                                                                         \
    QStringList settings::name##_gpu_devices() const {                        \
        return m_##name##_gpu_devices;                                        \
    }                                                                         \
    bool settings::has_##name##_gpu_device() const {                          \
        return m_##name##_gpu_devices.size() > 1;                             \
    }                                                                         \
    QString settings::name##_auto_gpu_device() const {                        \
        return m_##name##_gpu_devices.size() <= 1                             \
                   ? QString{}                                                \
                   : m_##name##_gpu_devices.at(1);                            \
    }                                                                         \
    QString settings::name##_gpu_device() const {                             \
        auto v =                                                              \
            value(QStringLiteral("service/" #name "_gpu_device")).toString(); \
        auto vl = v.split(',');                                               \
        if (vl.size() < 2) return {};                                         \
        auto it = std::find_if(                                               \
            m_##name##_gpu_devices.cbegin(), m_##name##_gpu_devices.cend(),   \
            [&vl](const auto& vv) {                                           \
                auto vvl = vv.split(',');                                     \
                return vvl.size() > 2 &&                                      \
                       vvl.at(0).trimmed() == vl.at(0).trimmed() &&           \
                       vvl.at(2).trimmed() == vl.at(1).trimmed();             \
            });                                                               \
        if (it == m_##name##_gpu_devices.cend()) return {};                   \
        return *it;                                                           \
    }                                                                         \
    void settings::set_##name##_gpu_device(const QString& value) {            \
        auto vl = value.split(',');                                           \
        auto n = vl.size() > 2 ? QStringLiteral("%1,%2").arg(                 \
                                     vl.at(0).trimmed(), vl.at(2).trimmed())  \
                               : "";                                          \
        auto v =                                                              \
            settings::value(QStringLiteral("service/" #name "_gpu_device"))   \
                .toString();                                                  \
        if (v != n) {                                                         \
            setValue(QStringLiteral("service/" #name "_gpu_device"), n);      \
            emit name##_gpu_device_changed();                                 \
            set_restart_required(true);                                       \
        }                                                                     \
    }                                                                         \
    int settings::name##_gpu_device_idx() const {                             \
        auto vl = name##_gpu_device().split(',');                             \
        if (vl.size() < 3) return 0;                                          \
        auto it = std::find_if(                                               \
            m_##name##_gpu_devices.cbegin(), m_##name##_gpu_devices.cend(),   \
            [&vl](const auto& vv) {                                           \
                auto vvl = vv.split(',');                                     \
                return vvl.size() > 2 &&                                      \
                       vvl.at(0).trimmed() == vl.at(0).trimmed() &&           \
                       vvl.at(2).trimmed() == vl.at(2).trimmed();             \
            });                                                               \
        if (it == m_##name##_gpu_devices.cend()) return 0;                    \
        return std::distance(m_##name##_gpu_devices.cbegin(), it);            \
    }                                                                         \
    void settings::set_##name##_gpu_device_idx(int value) {                   \
        if (value < 0 || value >= m_##name##_gpu_devices.size()) return;      \
        set_##name##_gpu_device(                                              \
            value == 0 ? "" : m_##name##_gpu_devices.at(value));              \
    }
GPU_ENGINE_TABLE
#undef X

QString settings::audio_input_device() const {
    return value(QStringLiteral("audio_input_device")).toString();
}

void settings::set_audio_input_device(const QString& value) {
    if (value != audio_input_device()) {
        setValue(QStringLiteral("audio_input_device"), value);
        emit audio_input_device_changed();
    }
}

settings::hotkeys_type_t settings::hotkeys_type() const {
#ifdef USE_X11_FEATURES
    if (is_wayland()) {
        return hotkeys_type_t::HotkeysTypePortal;
    }

    return static_cast<hotkeys_type_t>(
        value(QStringLiteral("hotkeys_type"),
              static_cast<int>(hotkeys_type_t::HotkeysTypeX11))
            .toInt());
#else
    return hotkeys_type_t::HotkeysTypePortal;
#endif
}

void settings::set_hotkeys_type(hotkeys_type_t value) {
    if (value != hotkeys_type()) {
        setValue(QStringLiteral("hotkeys_type"), static_cast<int>(value));
        emit hotkeys_type_changed();
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

int settings::mix_volume_change() const {
    return std::clamp(value(QStringLiteral("mix_volume_change"), 0).toInt(),
                      -30, 30);
}

void settings::set_mix_volume_change(int value) {
    value = std::clamp(value, -30, 30);

    if (mix_volume_change() != value) {
        setValue(QStringLiteral("mix_volume_change"), value);
        emit mix_volume_change_changed();
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

QVariantList settings::hotkeys_table() const {
    QVariantList table;
#define X(name, id, desc, key) \
    table.push_back(QStringList{id, desc, hotkey_##name()});
    HOTKEY_TABLE
#undef X
    return table;
}

void settings::reset_hotkey(const QString& requested_id) {
#define X(name, id, desc, key) \
    if (requested_id == id) {  \
        reset_hotkey_##name(); \
        return;                \
    }
    HOTKEY_TABLE
#undef X
}

void settings::set_hotkey(const QString& requested_id, const QString& value) {
#define X(name, id, desc, key)    \
    if (requested_id == id) {     \
        set_hotkey_##name(value); \
        return;                   \
    }
    HOTKEY_TABLE
#undef X
}

#define X(name, id, desc, key)                                             \
    QString settings::hotkey_##name() const {                              \
        return value(QStringLiteral("hotkey_" #name), QStringLiteral(key)) \
            .toString();                                                   \
    }                                                                      \
    void settings::set_hotkey_##name(const QString& value) {               \
        if (value != hotkey_##name()) {                                    \
            setValue(QStringLiteral("hotkey_" #name), value);              \
            emit hotkeys_changed();                                        \
        }                                                                  \
    }                                                                      \
    void settings::reset_hotkey_##name() {                                 \
        set_hotkey_##name(QStringLiteral(key));                            \
        emit hotkeys_table_changed();                                      \
    }
HOTKEY_TABLE
#undef X

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

settings::file_import_action_t settings::file_import_action() const {
    return static_cast<file_import_action_t>(
        value(QStringLiteral("file_import_action"),
              static_cast<int>(file_import_action_t::FileImportActionAsk))
            .toInt());
}

void settings::set_file_import_action(file_import_action_t value) {
    if (value != file_import_action()) {
        setValue(QStringLiteral("file_import_action"), static_cast<int>(value));
        emit file_import_action_changed();
    }
}

settings::default_export_tab_t settings::default_export_tab() const {
    return static_cast<default_export_tab_t>(
        value(QStringLiteral("default_export_tab"),
              static_cast<int>(default_export_tab_t::DefaultExportTabText))
            .toInt());
}

void settings::set_default_export_tab(default_export_tab_t value) {
    if (value != default_export_tab()) {
        setValue(QStringLiteral("default_export_tab"), static_cast<int>(value));
        emit default_export_tab_changed();
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

bool settings::keep_last_note() const {
    return value(QStringLiteral("keep_last_note"), true).toBool();
}

void settings::set_keep_last_note(bool new_value) {
    if (new_value != keep_last_note()) {
        setValue(QStringLiteral("keep_last_note"), new_value);
        emit keep_last_note_changed();

        if (new_value) {
            setValue(QStringLiteral("note"), m_note);
            m_note.clear();
        } else {
            m_note = value(QStringLiteral("note"), {}).toString();
            setValue(QStringLiteral("note"), {});
        }
    }
}

#define X(name, dvalue)                                                  \
    bool settings::hw_scan_##name() const {                              \
        return value(QStringLiteral("hw_scan_" #name), dvalue).toBool(); \
    }                                                                    \
    void settings::set_hw_scan_##name(bool value) {                      \
        if (value != hw_scan_##name()) {                                 \
            setValue(QStringLiteral("hw_scan_" #name), value);           \
            emit hw_scan_##name##_changed();                             \
            set_restart_required(true);                                  \
        }                                                                \
    }
GPU_SCAN_TABLE
#undef X

settings::tts_subtitles_sync_mode_t settings::tts_subtitles_sync() const {
    return static_cast<settings::tts_subtitles_sync_mode_t>(
        value(QStringLiteral("tts_subtitles_sync"),
              static_cast<int>(settings::tts_subtitles_sync_mode_t::
                                   TtsSubtitleSyncOnFitOnlyIfLonger))
            .toInt());
}

void settings::set_tts_subtitles_sync(tts_subtitles_sync_mode_t value) {
    if (value != tts_subtitles_sync()) {
        setValue(QStringLiteral("tts_subtitles_sync"), static_cast<int>(value));
        emit tts_subtitles_sync_changed();
    }
}

settings::tts_tag_mode_t settings::tts_tag_mode() const {
    return static_cast<settings::tts_tag_mode_t>(
        value(QStringLiteral("tts_tag_mode"),
              static_cast<int>(settings::tts_tag_mode_t::TtsTagModeSupport))
            .toInt());
}

void settings::set_tts_tag_mode(tts_tag_mode_t value) {
    if (value != tts_tag_mode()) {
        setValue(QStringLiteral("tts_tag_mode"), static_cast<int>(value));
        emit tts_tag_mode_changed();
    }
}

settings::py_scan_mode_t settings::py_scan_mode() const {
    return static_cast<py_scan_mode_t>(
        value(QStringLiteral("service/py_scan_mode"),
              static_cast<int>(is_flatpak() && cpu_tools::arch() ==
                                                   cpu_tools::arch_t::x86_64
                                   ? py_scan_mode_t::PyScanOffAllEnabled
                                   : py_scan_mode_t::PyScanOn))
            .toInt());
}

void settings::set_py_scan_mode(py_scan_mode_t value) {
    if (value != py_scan_mode()) {
        setValue(QStringLiteral("service/py_scan_mode"),
                 static_cast<int>(value));
        emit py_scan_mode_changed();

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

int settings::settings_stt_engine_idx() const {
    return value(QStringLiteral("settings_stt_engine_idx"), 0).toInt();
}

void settings::set_settings_stt_engine_idx(int value) {
    if (settings_stt_engine_idx() != value) {
        setValue(QStringLiteral("settings_stt_engine_idx"), value);
        emit settings_stt_engine_idx_changed();
    }
}

int settings::settings_tts_engine_idx() const {
    return value(QStringLiteral("settings_tts_engine_idx"), 0).toInt();
}

void settings::set_settings_tts_engine_idx(int value) {
    if (settings_tts_engine_idx() != value) {
        setValue(QStringLiteral("settings_tts_engine_idx"), value);
        emit settings_tts_engine_idx_changed();
    }
}

bool settings::gpu_override_version() const {
    return value(QStringLiteral("service/gpu_override_version"), false)
        .toBool();
}

void settings::set_gpu_override_version([[maybe_unused]] bool value) {
    if (gpu_override_version() != value) {
        setValue(QStringLiteral("service/gpu_override_version"), value);
        emit gpu_override_version_changed();
        set_restart_required(true);
    }
}

QString settings::gpu_overrided_version() {
    auto val =
        value(QStringLiteral("service/gpu_overrided_version"), {}).toString();

    if (val.isEmpty() && !m_rocm_gpu_versions.empty()) {
        val = QString::fromStdString(gpu_tools::rocm_overrided_gfx_version(
            m_rocm_gpu_versions.front().toStdString()));
        setValue(QStringLiteral("service/gpu_overrided_version"), val);
    }

    return val;
}

void settings::set_gpu_overrided_version([[maybe_unused]] QString new_value) {
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
}

void settings::disable_hw_scan() {
#define X(name, dvalue) set_hw_scan_##name(false);
    GPU_SCAN_TABLE
#undef X
}

void settings::disable_py_scan() {
    set_py_scan_mode(py_scan_mode_t::PyScanOffAllDisabled);
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
    auto nvidia_metainfo_file = QStringLiteral(
        "/app/extensions/nvidia/share/metainfo/"
        "net.mkiol.SpeechNote.Addon.nvidia.metainfo.xml");
    auto amd_metainfo_file = QStringLiteral(
        "/app/extensions/amd/share/metainfo/"
        "net.mkiol.SpeechNote.Addon.amd.metainfo.xml");

    bool has_nvidia_addon = QFileInfo::exists(nvidia_metainfo_file);
    bool has_amd_addon = QFileInfo::exists(amd_metainfo_file);

    auto get_addon_ver = [](const QString& metainfo_file) -> QString {
        QDomDocument doc{};
        QFile file{metainfo_file};
        if (!file.open(QIODevice::ReadOnly)) {
            LOGW("cannot open addon metainfo file: " << metainfo_file);
            return {};
        }

        if (!doc.setContent(&file)) {
            LOGW("cannot parse addon metainfo file: " << metainfo_file);
            file.close();
            return {};
        }

        auto release_elements =
            doc.elementsByTagName(QStringLiteral("release"));

        QDate latest_date;
        QString latest_ver;
        for (int i = 0; i < release_elements.size(); ++i) {
            auto ele = release_elements.at(i).toElement();
            if (!ele.hasAttribute("version") || !ele.hasAttribute("date"))
                continue;
            auto date = QDate::fromString(ele.attribute("date"), Qt::ISODate);
            if (date.isValid() &&
                (latest_date.isNull() || date > latest_date)) {
                latest_date = date;
                latest_ver = ele.attribute("version");
            }
        }

        return latest_ver;
    };

    if (has_nvidia_addon) {
        new_flags |= addon_flags_t::AddonNvidia;
        auto ver = get_addon_ver(nvidia_metainfo_file);
        LOGD("flatpak addon detected: nvidia " << ver);
        if (!ver.startsWith(APP_ADDON_VERSION, Qt::CaseInsensitive)) {
            LOGW("*********************************************");
            LOGW(
                "NVIDIA GPU acceleration add-on version is incompatible. "
                "Required version is " APP_ADDON_VERSION ".");
            LOGW("*********************************************");
            add_error_flags(error_flags_t::ErrorIncompatibleNvidiaGpuAddon);
        }
    }

    if (has_amd_addon) {
        new_flags |= addon_flags_t::AddonAmd;
        auto ver = get_addon_ver(amd_metainfo_file);
        LOGD("flatpak addon detected: amd " << ver);
        if (!ver.startsWith(APP_ADDON_VERSION, Qt::CaseInsensitive)) {
            LOGW("*********************************************");
            LOGW(
                "AMD GPU acceleration add-on version is incompatible. "
                "Required version is " APP_ADDON_VERSION ".");
            LOGW("*********************************************");
            add_error_flags(error_flags_t::ErrorIncompatibleAmdGpuAddon);
        }
    }
#endif

    if (new_flags != m_addon_flags) {
        m_addon_flags = new_flags;
        LOGD("addon-flags: " << m_addon_flags);
        emit addon_flags_changed();

        if (m_addon_flags & addon_flags_t::AddonNvidia &&
            m_addon_flags & addon_flags_t::AddonAmd) {
            LOGW("*********************************************");
            LOGW(
                "Both NVIDIA and AMD GPU acceleration add-ons are "
                "installed, which is not optimal. "
                "Uninstall one of them.");
            LOGW("*********************************************");
            add_error_flags(error_flags_t::ErrorMoreThanOneGpuAddons);
        }
    }
}

void settings::update_addon_flags_from_fa(
    const QVariantMap& features_availability) {
    if (features_availability.contains("addon-flags")) {
        auto vl = features_availability.value("addon-flags").toList();
        if (!vl.isEmpty()) m_addon_flags = vl.front().toUInt();
        LOGD("addon-flags from fa: " << m_addon_flags);
        emit addon_flags_changed();
    } else {
        LOGD("no addon-flags from fa");
    }
}

unsigned int settings::system_flags() const { return m_system_flags; }

void settings::update_system_flags() {
    unsigned int new_flags = system_flags_t::SystemNone;

    if (gpu_tools::has_nvidia_gpu()) {
        new_flags |= system_flags_t::SystemNvidiaGpu;
        LOGD("nvidia gpu detected");
    }
    if (gpu_tools::has_amd_gpu()) {
        new_flags |= system_flags_t::SystemAmdGpu;
        LOGD("amd gpu detected");
    }

    if (m_whispercpp_gpu_devices.size() > 1 ||
        m_fasterwhisper_gpu_devices.size() > 1 ||
        m_coqui_gpu_devices.size() > 1 ||
        m_whisperspeech_gpu_devices.size() > 1) {
        new_flags |= system_flags_t::SystemHwAccel;
        LOGD("hw accel detected");
    }

    if (new_flags != m_system_flags) {
        m_system_flags = new_flags;
        LOGD("system-flags: " << m_system_flags);
        emit system_flags_changed();
    }
}

void settings::update_system_flags_from_fa(
    const QVariantMap& features_availability) {
    if (features_availability.contains("system-flags")) {
        auto vl = features_availability.value("system-flags").toList();
        if (!vl.isEmpty()) m_system_flags = vl.front().toUInt();
        LOGD("system-flags from fa: " << m_system_flags);
        emit system_flags_changed();
    } else {
        LOGD("no system-flags from fa");
    }

    if (features_availability.contains("error-flags")) {
        auto vl = features_availability.value("error-flags").toList();
        if (!vl.isEmpty()) m_error_flags = vl.front().toUInt();
        LOGD("error-flags from fa: " << m_error_flags);
        emit error_flags_changed();
    } else {
        LOGD("no error-flags from fa");
    }
}

unsigned int settings::error_flags() const { return m_error_flags; }

void settings::add_error_flags(error_flags_t new_flag) {
    unsigned int new_flags =
        m_error_flags | static_cast<unsigned int>(new_flag);

    if (new_flags != m_error_flags) {
        m_error_flags = new_flags;
        emit error_flags_changed();
    }
}

bool settings::subtitles_support() const {
#ifdef USE_SFOS
    return value(QStringLiteral("subtitles_support"), false).toBool();
#else
    return true;
#endif
}

void settings::set_subtitles_support(bool value) {
    if (subtitles_support() != value) {
        setValue(QStringLiteral("subtitles_support"), value);
        emit subtitles_support_changed();
        emit stt_tts_text_format_changed();
        emit mnt_text_format_changed();
    }
}

QVariantList settings::make_default_trans_rules() {
    std::underlying_type_t<trans_rule_flags_t> flags =
        trans_rule_flags_t::TransRuleNone;

    QVariantList rules;
    rules.push_back(QVariantList{
        /*[0] flags=*/flags | trans_rule_flags_t::TransRuleTargetStt,
        /*[1] type=*/
        static_cast<std::underlying_type_t<trans_rule_type_t>>(
            trans_rule_type_t::TransRuleTypeReplaceRe),
        /*[2] name=*/
        tr("Example: Replace \"%1\" with \"%2\" and start the next word with a "
           "capital letter")
            .arg("period", "."),
        /*[3] pattern=*/
        QString{"[\\s\\,\\?\\!\\.]*period[\\s\\,\\?\\!\\.]*(\\w)?"},
        /*[4] replace=*/QString{". \\U\\1"},
        /*[5] langs=*/QString{"en"},
        /*[6] test_test=*/QString{}});
    rules.push_back(QVariantList{
        /*[0] flags=*/flags | trans_rule_flags_t::TransRuleTargetStt,
        /*[1] type=*/
        static_cast<std::underlying_type_t<trans_rule_type_t>>(
            trans_rule_type_t::TransRuleTypeReplaceRe),
        /*[2] name=*/
        tr("Example: Replace \"%1\" with \"%2\" and start the next word with a "
           "lowercase letter")
            .arg("comma", ","),
        /*[3] pattern=*/
        QString{"[\\s\\,\\?\\!\\.]*comma[\\s\\,\\?\\!\\.]*(\\w)?"},
        /*[4] replace=*/QString{", \\u\\1"},
        /*[5] langs=*/QString{"en"},
        /*[6] test_test=*/QString{}});
    rules.push_back(QVariantList{
        /*[0] flags=*/flags | trans_rule_flags_t::TransRuleTargetStt,
        /*[1] type=*/
        static_cast<std::underlying_type_t<trans_rule_type_t>>(
            trans_rule_type_t::TransRuleTypeReplaceRe),
        /*[2] name=*/
        tr("Example: Insert newline instead of the word \"%1\"")
            .arg("new line"),
        /*[3] pattern=*/
        QString{"[\\s*\\,]?new[\\s\\-]*line[\\s\\,\\?\\!\\.]*"},
        /*[4] replace=*/QString{".\\n"},
        /*[5] langs=*/QString{"en"},
        /*[6] test_test=*/QString{}});
    // rules.push_back(QVariantList{
    //     /*[0] flags=*/flags | trans_rule_flags_t::TransRuleTargetTts,
    //     /*[1] type=*/
    //     static_cast<std::underlying_type_t<trans_rule_type_t>>(
    //         trans_rule_type_t::TransRuleTypeReplaceSimple),
    //     /*[2] name=*/
    //     tr("Example: Add silence after \"%1\"").arg("."),
    //     /*[3] pattern=*/
    //     QString{"."},
    //     /*[4] replace=*/QString{". {silence:1s}"},
    //     /*[5] langs=*/QString{},
    //     /*[6] test_test=*/QString{}});
    rules.push_back(QVariantList{
        /*[0] flags=*/flags | trans_rule_flags_t::TransRuleTargetTts,
        /*[1] type=*/
        static_cast<std::underlying_type_t<trans_rule_type_t>>(
            trans_rule_type_t::TransRuleTypeReplaceRe),
        /*[2] name=*/
        tr("Example: Correct pronunciation of the Polish name \"%1\"")
            .arg("Michał"),
        /*[3] pattern=*/
        QString{"Micha[lł]"},
        /*[4] replace=*/QString{"Me how"},
        /*[5] langs=*/QString{"en"},
        /*[6] test_test=*/QString{}});
    return rules;
}

QVariantList settings::trans_rules() const {
    if (contains(QStringLiteral("trans_rules"))) {
        return value(QStringLiteral("trans_rules"), {}).toList();
    }

    return make_default_trans_rules();
}

void settings::set_trans_rules(const QVariantList& value) {
    if (trans_rules() != value) {
        setValue(QStringLiteral("trans_rules"), value);
        emit trans_rules_changed();
    }
}

void settings::trans_rule_set_flag(int index, unsigned int mask, bool enabled) {
    if (index < 0) return;
    auto rules = trans_rules();
    if (rules.size() > index) {
        auto l = rules[index].toList();
        if (l.isEmpty() || (l[0].toUInt() & mask) == enabled) return;
        if (enabled) {
            l[0] = l[0].toUInt() | mask;
        } else {
            l[0] = l[0].toUInt() & ~mask;
        }
        rules[index] = l;
        set_trans_rules(rules);
    }
}

void settings::trans_rule_set_target_stt(int index, bool enabled) {
    trans_rule_set_flag(index, trans_rule_flags_t::TransRuleTargetStt, enabled);
}

void settings::trans_rule_set_target_tts(int index, bool enabled) {
    trans_rule_set_flag(index, trans_rule_flags_t::TransRuleTargetTts, enabled);
}

void settings::trans_rule_delete(int index) {
    if (index < 0) return;
    auto rules = trans_rules();
    if (rules.size() > index) {
        rules.erase(std::next(rules.begin(), index));
        set_trans_rules(rules);
    }
}

void settings::trans_rule_move_up(int index) {
    if (index <= 0) return;
    auto rules = trans_rules();
    if (rules.size() <= index) return;
    std::swap(rules[index - 1], rules[index]);
    set_trans_rules(rules);
}

void settings::trans_rule_move_down(int index) {
    if (index < 0) return;
    auto rules = trans_rules();
    if (rules.size() <= index + 1) return;
    std::swap(rules[index + 1], rules[index]);
    set_trans_rules(rules);
}

void settings::trans_rule_clone(int index) {
    if (index < 0) return;
    auto rules = trans_rules();
    if (index >= rules.size()) return;
    auto rule = rules.at(index).toList();
    auto name = rule.at(2).toString();
    if (name.isEmpty()) {
        name = tr("Clone of \"%1\"").arg(index + 1);
    } else {
        name = tr("Clone of \"%1\"").arg(name);
    }
    rule[2] = name;
    rules.insert(index + 1, rule);
    set_trans_rules(rules);
}

QVariantList settings::make_default_tts_voice_prompts() {
    QVariantList rules;
    rules.push_back(
        QVariantList{/*[0] name=*/"Jon",
                     /*[1] prompt=*/"Joh's talking. Very clear audio.",
                     /*[2] flags=*/0});
    rules.push_back(
        QVariantList{/*[0] name=*/"Lea",
                     /*[1] prompt=*/"Lea's talking. Very clear audio.",
                     /*[2] flags=*/0});
    rules.push_back(
        QVariantList{/*[0] name=*/"Happy Jerry",
                     /*[1] prompt=*/"Jerry's talking. Happy. Very clear audio.",
                     /*[2] flags=*/0});
    rules.push_back(QVariantList{
        /*[0] name=*/"Sad Elisabeth",
        /*[1] prompt=*/"Elisabeth's talking. Sad. Very clear audio.",
        /*[2] flags=*/0});
    return rules;
}

QVariantList settings::tts_voice_prompts() const {
    if (contains(QStringLiteral("tts_voice_prompts"))) {
        return value(QStringLiteral("tts_voice_prompts"), {}).toList();
    }

    return make_default_tts_voice_prompts();
}

QStringList settings::tts_voice_prompt_names() const {
    QStringList names;

    auto prompts = tts_voice_prompts();
    std::transform(prompts.cbegin(), prompts.cend(), std::back_inserter(names),
                   [](const auto& prompt) {
                       auto l = prompt.toList();
                       return l.isEmpty() ? QString{} : l.at(0).toString();
                   });

    return names;
}

void settings::set_tts_voice_prompts(const QVariantList& value) {
    if (tts_voice_prompts() != value) {
        setValue(QStringLiteral("tts_voice_prompts"), value);
        emit tts_voice_prompts_changed();
    }
}

QString settings::tts_name_of_voice_prompt(const QString& name) const {
    auto prompts = tts_voice_prompts();
    auto it =
        std::find_if(prompts.cbegin(), prompts.cend(), [&](const auto& prompt) {
            auto l = prompt.toList();
            return l.size() > 1 && l.at(0) == name;
        });

    if (it == prompts.cend()) {
        return prompts.isEmpty() ? QString{}
                                 : prompts.front().toList().front().toString();
    }

    return it->toList().front().toString();
}

int settings::tts_index_of_voice_prompt(const QString& name) const {
    auto prompts = tts_voice_prompts();
    auto it =
        std::find_if(prompts.cbegin(), prompts.cend(), [&](const auto& prompt) {
            auto l = prompt.toList();
            return l.size() > 1 && l.at(0) == name;
        });

    if (it == prompts.cend()) {
        return prompts.isEmpty() ? -1 : 0;
    }

    return std::distance(prompts.cbegin(), it);
}

QString settings::tts_desc_of_voice_prompt(const QString& name) const {
    auto prompts = tts_voice_prompts();
    auto it =
        std::find_if(prompts.cbegin(), prompts.cend(), [&](const auto& prompt) {
            auto l = prompt.toList();
            return l.size() > 1 && l.at(0) == name;
        });

    if (it == prompts.cend()) {
        return {};
    }

    return it->toList().at(1).toString();
}

std::optional<settings::voice_profile_prompt_t> settings::tts_voice_prompt(
    const QString& name) const {
    auto prompts = tts_voice_prompts();
    auto it = std::find_if(prompts.cbegin(), prompts.cend(),
                           [&name](const auto& prompt) {
                               auto l = prompt.toList();
                               return l.size() > 1 && l.at(0) == name;
                           });

    if (it == prompts.cend()) {
        return std::nullopt;
    }

    auto l = it->toList();

    return voice_profile_prompt_t{/*name=*/l.at(0).toString(),
                          /*desc=*/l.at(1).toString()};
}

void settings::tts_update_voice_prompt(const QString& name,
                                       const voice_profile_prompt_t& voice_prompt) {
    auto prompts = tts_voice_prompts();

    bool push_new = true;
    if (!name.isEmpty()) {
        auto it = std::find_if(prompts.begin(), prompts.end(),
                               [&](const auto& prompt) {
                                   auto l = prompt.toList();
                                   return l.size() > 1 && l.at(0) == name;
                               });

        if (it != prompts.end()) {
            push_new = false;
            *it = QStringList{voice_prompt.name, voice_prompt.desc};
        }
    }

    if (push_new) {
        prompts.push_back(QStringList{voice_prompt.name, voice_prompt.desc});
    }

    set_tts_voice_prompts(prompts);
}

void settings::tts_delete_voice_prompt(const QString& name) {
    auto prompts = tts_voice_prompts();

    auto it =
        std::find_if(prompts.begin(), prompts.end(), [&](const auto& prompt) {
            auto l = prompt.toList();
            return l.size() > 1 && l.at(0) == name;
        });

    if (it != prompts.end()) {
        prompts.erase(it);
        set_tts_voice_prompts(prompts);
    }
}

void settings::tts_clone_voice_prompt(const QString& name) {
    auto voice_prompt = tts_voice_prompt(name);
    if (!voice_prompt) return;

    auto prompts = settings::tts_voice_prompts();

    prompts.push_back(QStringList{tr("Clone of \"%1\"").arg(voice_prompt->name),
                                  voice_prompt->desc});

    settings::set_tts_voice_prompts(prompts);
}

QString settings::tts_active_voice_prompt() const {
    return tts_name_of_voice_prompt(
        value(QStringLiteral("tts_active_voice_prompt"), {}).toString());
}

void settings::set_tts_active_voice_prompt(const QString& value) {
    if (tts_active_voice_prompt() != value) {
        setValue(QStringLiteral("tts_active_voice_prompt"), value);
        emit tts_active_voice_prompt_changed();
    }
}

int settings::tts_active_voice_prompt_idx() const {
    return tts_index_of_voice_prompt(
        value(QStringLiteral("tts_active_voice_prompt"), {}).toString());
}

void settings::set_tts_active_voice_prompt_idx(int idx) {
    if (idx < 0) return;
    auto prompts = settings::tts_voice_prompts();
    if (prompts.size() > idx)
        set_tts_active_voice_prompt(prompts.at(idx).toList().at(0).toString());
}

QString settings::tts_active_voice_prompt_for_in_mnt() const {
    return tts_name_of_voice_prompt(
        value(QStringLiteral("tts_active_voice_prompt_for_in_mnt"), {})
            .toString());
}

void settings::set_tts_active_voice_prompt_for_in_mnt(const QString& value) {
    if (tts_active_voice_prompt_for_in_mnt() != value) {
        setValue(QStringLiteral("tts_active_voice_prompt_for_in_mnt"), value);
        emit tts_active_voice_prompt_for_in_mnt_changed();
    }
}

int settings::tts_active_voice_prompt_for_in_mnt_idx() const {
    return tts_index_of_voice_prompt(
        value(QStringLiteral("tts_active_voice_prompt_for_in_mnt"), {})
            .toString());
}

void settings::set_tts_active_voice_prompt_for_in_mnt_idx(int idx) {
    if (idx < 0) return;
    auto prompts = settings::tts_voice_prompts();
    if (prompts.size() > idx)
        set_tts_active_voice_prompt_for_in_mnt(
            prompts.at(idx).toList().at(0).toString());
}

QString settings::tts_active_voice_prompt_for_out_mnt() const {
    return tts_name_of_voice_prompt(
        value(QStringLiteral("tts_active_voice_prompt_for_out_mnt"), {})
            .toString());
}

void settings::set_tts_active_voice_prompt_for_out_mnt(const QString& value) {
    if (tts_active_voice_prompt_for_out_mnt() != value) {
        setValue(QStringLiteral("tts_active_voice_prompt_for_out_mnt"), value);
        emit tts_active_voice_prompt_for_out_mnt_changed();
    }
}

int settings::tts_active_voice_prompt_for_out_mnt_idx() const {
    return tts_index_of_voice_prompt(
        value(QStringLiteral("tts_active_voice_prompt_for_out_mnt"), {})
            .toString());
}

void settings::set_tts_active_voice_prompt_for_out_mnt_idx(int idx) {
    if (idx < 0) return;
    auto prompts = settings::tts_voice_prompts();
    if (prompts.size() > idx)
        set_tts_active_voice_prompt_for_out_mnt(
            prompts.at(idx).toList().at(0).toString());
}

settings::voice_profile_type_t settings::active_voice_profile_type() const {
    return static_cast<voice_profile_type_t>(
        value(QStringLiteral("active_voice_profile_type"),
              static_cast<int>(voice_profile_type_t::VoiceProfileAudioSample))
            .toInt());
}

void settings::set_active_voice_profile_type(voice_profile_type_t value) {
    if (active_voice_profile_type() != value) {
        setValue(QStringLiteral("active_voice_profile_type"),
                 static_cast<int>(value));
        emit active_voice_profile_type_changed();
    }
}

settings::fake_keyboard_type_t settings::fake_keyboard_type() const {
#ifdef USE_X11_FEATURES
    if (is_wayland()) {
        return fake_keyboard_type_t::FakeKeyboardTypeYdo;
    }

    return static_cast<fake_keyboard_type_t>(
        value(QStringLiteral("fake_keyboard_type"),
              static_cast<int>(fake_keyboard_type_t::FakeKeyboardTypeXdo))
            .toInt());
#else
    return fake_keyboard_type_t::FakeKeyboardTypeYdo;
#endif
}

void settings::set_fake_keyboard_type(fake_keyboard_type_t value) {
    if (fake_keyboard_type() != value) {
        setValue(QStringLiteral("fake_keyboard_type"), static_cast<int>(value));
        emit fake_keyboard_type_changed();
    }
}

int settings::fake_keyboard_delay() const {
    auto v = value(QStringLiteral("fake_keyboard_delay"), 10).toInt();
    return std::clamp(v, 1, 1000);
}

void settings::set_fake_keyboard_delay(int value) {
    if (fake_keyboard_delay() != value) {
        setValue(QStringLiteral("fake_keyboard_delay"), value);
        emit fake_keyboard_delay_changed();
    }
}

/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "qdebug.h"
#include "singleton.h"

class settings : public QSettings, public singleton<settings> {
    Q_OBJECT

    // app
    Q_PROPERTY(QString note READ note WRITE set_note NOTIFY note_changed)
    Q_PROPERTY(speech_mode_t speech_mode READ speech_mode WRITE set_speech_mode
                   NOTIFY speech_mode_changed)
    Q_PROPERTY(insert_mode_t insert_mode READ insert_mode WRITE set_insert_mode
                   NOTIFY insert_mode_changed)
    Q_PROPERTY(mode_t mode READ mode WRITE set_mode NOTIFY mode_changed)
    Q_PROPERTY(QString file_save_dir READ file_save_dir WRITE set_file_save_dir
                   NOTIFY file_save_dir_changed)
    Q_PROPERTY(QUrl file_save_dir_url READ file_save_dir_url WRITE
                   set_file_save_dir_url NOTIFY file_save_dir_changed)
    Q_PROPERTY(QString file_save_dir_name READ file_save_dir_name NOTIFY
                   file_save_dir_changed)
    Q_PROPERTY(QString file_save_filename READ file_save_filename NOTIFY
                   file_save_dir_changed)
    Q_PROPERTY(QString file_open_dir READ file_open_dir WRITE set_file_open_dir
                   NOTIFY file_open_dir_changed)
    Q_PROPERTY(QUrl file_open_dir_url READ file_open_dir_url WRITE
                   set_file_open_dir_url NOTIFY file_open_dir_changed)
    Q_PROPERTY(QString file_open_dir_name READ file_open_dir_name NOTIFY
                   file_open_dir_changed)
    Q_PROPERTY(QString prev_app_ver READ prev_app_ver WRITE set_prev_app_ver
                   NOTIFY prev_app_ver_changed)
    Q_PROPERTY(bool translator_mode READ translator_mode WRITE
                   set_translator_mode NOTIFY translator_mode_changed)
    Q_PROPERTY(
        bool translate_when_typing READ translate_when_typing WRITE
            set_translate_when_typing NOTIFY translate_when_typing_changed)
    Q_PROPERTY(bool hint_translator READ hint_translator WRITE
                   set_hint_translator NOTIFY hint_translator_changed)
    Q_PROPERTY(int qt_style_idx READ qt_style_idx WRITE set_qt_style_idx NOTIFY
                   qt_style_idx_changed)
    Q_PROPERTY(bool restart_required READ restart_required NOTIFY
                   restart_required_changed)
    Q_PROPERTY(speech_speed_t speech_speed READ speech_speed WRITE
                   set_speech_speed NOTIFY speech_speed_changed)
    Q_PROPERTY(int font_size READ font_size WRITE set_font_size NOTIFY
                   font_size_changed)
    Q_PROPERTY(audio_format_t audio_format READ audio_format WRITE
                   set_audio_format NOTIFY audio_format_changed)
    Q_PROPERTY(audio_quality_t audio_quality READ audio_quality WRITE
                   set_audio_quality NOTIFY audio_quality_changed)

    // service
    Q_PROPERTY(QString models_dir READ models_dir WRITE set_models_dir NOTIFY
                   models_dir_changed)
    Q_PROPERTY(QUrl models_dir_url READ models_dir_url WRITE set_models_dir_url
                   NOTIFY models_dir_changed)
    Q_PROPERTY(
        QString models_dir_name READ models_dir_name NOTIFY models_dir_changed)
    Q_PROPERTY(QString cache_dir READ cache_dir WRITE set_cache_dir NOTIFY
                   cache_dir_changed)
    Q_PROPERTY(QUrl cache_dir_url READ cache_dir_url WRITE set_cache_dir_url
                   NOTIFY cache_dir_changed)
    Q_PROPERTY(bool restore_punctuation READ restore_punctuation WRITE
                   set_restore_punctuation NOTIFY restore_punctuation_changed)
    Q_PROPERTY(QString default_stt_model READ default_stt_model WRITE
                   set_default_stt_model NOTIFY default_stt_model_changed)
    Q_PROPERTY(QString default_tts_model READ default_tts_model WRITE
                   set_default_tts_model NOTIFY default_tts_model_changed)
    Q_PROPERTY(QString default_mnt_lang READ default_mnt_lang WRITE
                   set_default_mnt_lang NOTIFY default_mnt_lang_changed)
    Q_PROPERTY(QString default_mnt_out_lang READ default_mnt_out_lang WRITE
                   set_default_mnt_out_lang NOTIFY default_mnt_out_lang_changed)
    Q_PROPERTY(bool whisper_use_gpu READ whisper_use_gpu WRITE
                   set_whisper_use_gpu NOTIFY whisper_use_gpu_changed)
    Q_PROPERTY(
        QStringList gpu_devices READ gpu_devices NOTIFY gpu_devices_changed)
    Q_PROPERTY(int gpu_device_idx READ gpu_device_idx WRITE set_gpu_device_idx
                   NOTIFY gpu_device_changed)
    Q_PROPERTY(QString gpu_device READ gpu_device WRITE set_gpu_device NOTIFY
                   gpu_device_changed)
    Q_PROPERTY(
        QStringList audio_inputs READ audio_inputs NOTIFY audio_inputs_changed)
    Q_PROPERTY(int audio_input_idx READ audio_input_idx WRITE
                   set_audio_input_idx NOTIFY audio_input_changed)
    Q_PROPERTY(QString audio_input READ audio_input WRITE set_audio_input NOTIFY
                   audio_input_changed)

   public:
    enum class mode_t { Stt = 0, Tts = 1 };
    Q_ENUM(mode_t)
    friend QDebug operator<<(QDebug d, mode_t mode);

    enum class launch_mode_t { app_stanalone, app, service };
    friend QDebug operator<<(QDebug d, launch_mode_t launch_mode);

    enum class speech_mode_t {
        SpeechAutomatic = 0,
        SpeechManual = 1,
        SpeechSingleSentence = 2
    };
    Q_ENUM(speech_mode_t)
    friend QDebug operator<<(QDebug d, speech_mode_t speech_mode);

    enum class insert_mode_t {
        InsertInLine = 1,
        InsertNewLine = 0,
    };
    Q_ENUM(insert_mode_t)

    enum class speech_speed_t {
        SpeechSpeedVerySlow = -2,
        SpeechSpeedSlow = -1,
        SpeechSpeedNormal = 0,
        SpeechSpeedFast = 1,
        SpeechSpeedVeryFast = 2
    };
    Q_ENUM(speech_speed_t)

    enum class audio_format_t {
        AudioFormatAuto = 0,
        AudioFormatWav = 1,
        AudioFormatMp3 = 2,
        AudioFormatOgg = 3
    };
    Q_ENUM(audio_format_t)

    enum class audio_quality_t {
        AudioQualityVbrHigh = 10,
        AudioQualityVbrMedium = 11,
        AudioQualityVbrLow = 12,
    };
    Q_ENUM(audio_quality_t)

    settings();

    launch_mode_t launch_mode() const;
    void set_launch_mode(launch_mode_t launch_mode);
    QString module_checksum(const QString &name) const;
    void set_module_checksum(const QString &name, const QString &value);

    // app
    QString note() const;
    void set_note(const QString &value);
    speech_mode_t speech_mode() const;
    void set_speech_mode(speech_mode_t value);
    speech_speed_t speech_speed() const;
    void set_speech_speed(speech_speed_t value);
    insert_mode_t insert_mode() const;
    void set_insert_mode(insert_mode_t value);
    mode_t mode() const;
    void set_mode(mode_t value);
    QString file_save_dir() const;
    void set_file_save_dir(const QString &value);
    QUrl file_save_dir_url() const;
    void set_file_save_dir_url(const QUrl &value);
    QString file_save_dir_name() const;
    QString file_save_filename() const;
    QString file_open_dir() const;
    void set_file_open_dir(const QString &value);
    QUrl file_open_dir_url() const;
    void set_file_open_dir_url(const QUrl &value);
    QString file_open_dir_name() const;
    QString prev_app_ver() const;
    void set_prev_app_ver(const QString &value);
    bool translator_mode() const;
    void set_translator_mode(bool value);
    bool translate_when_typing() const;
    void set_translate_when_typing(bool value);
    QString default_tts_model_for_mnt_lang(const QString &lang);
    void set_default_tts_model_for_mnt_lang(const QString &lang,
                                            const QString &value);
    bool hint_translator() const;
    void set_hint_translator(bool value);
    int qt_style_idx() const;
    void set_qt_style_idx(int value);
    bool restart_required() const;
    void set_restart_required();
    void set_font_size(int value);
    int font_size() const;
    void set_audio_format(audio_format_t value);
    audio_format_t audio_format() const;
    void set_audio_quality(audio_quality_t value);
    audio_quality_t audio_quality() const;

    Q_INVOKABLE QUrl app_icon() const;
    Q_INVOKABLE bool py_supported() const;
    Q_INVOKABLE bool gpu_supported() const;
    Q_INVOKABLE bool has_gpu_device() const;
    Q_INVOKABLE bool has_audio_input() const;
    Q_INVOKABLE QStringList qt_styles() const;
    Q_INVOKABLE bool file_exists(const QString &file_path) const;
    Q_INVOKABLE QString
    add_ext_to_audio_filename(const QString &filename) const;
    static QString audio_format_str_from_filename(const QString &filename);

    // service
    QString models_dir() const;
    void set_models_dir(const QString &value);
    QUrl models_dir_url() const;
    void set_models_dir_url(const QUrl &value);
    QString models_dir_name() const;
    QString cache_dir() const;
    void set_cache_dir(const QString &value);
    QUrl cache_dir_url() const;
    void set_cache_dir_url(const QUrl &value);
    bool restore_punctuation() const;
    void set_restore_punctuation(bool value);
    QStringList enabled_models();
    void set_enabled_models(const QStringList &value);
    bool whisper_use_gpu() const;
    void set_whisper_use_gpu(bool value);
    QStringList gpu_devices() const;
    QString gpu_device() const;
    void set_gpu_device(QString value);
    int gpu_device_idx() const;
    void set_gpu_device_idx(int value);
    QStringList audio_inputs() const;
    QString audio_input() const;
    void set_audio_input(QString value);
    int audio_input_idx() const;
    void set_audio_input_idx(int value);

    // stt
    QString default_stt_model() const;
    void set_default_stt_model(const QString &value);
    QString default_stt_model_for_lang(const QString &lang);
    void set_default_stt_model_for_lang(const QString &lang,
                                        const QString &value);

    // tts
    QString default_tts_model() const;
    void set_default_tts_model(const QString &value);
    QString default_tts_model_for_lang(const QString &lang);
    void set_default_tts_model_for_lang(const QString &lang,
                                        const QString &value);

    // mnt
    QString default_mnt_lang() const;
    void set_default_mnt_lang(const QString &value);
    QString default_mnt_out_lang() const;
    void set_default_mnt_out_lang(const QString &value);

   signals:
    // app
    void speech_mode_changed();
    void note_changed();
    void insert_mode_changed();
    void mode_changed();
    void file_save_dir_changed();
    void file_open_dir_changed();
    void prev_app_ver_changed();
    void translator_mode_changed();
    void translate_when_typing_changed();
    void default_tts_models_for_mnt_changed(const QString &lang);
    void hint_translator_changed();
    void qt_style_idx_changed();
    void restart_required_changed();
    void speech_speed_changed();
    void font_size_changed();
    void audio_format_changed();
    void audio_quality_changed();

    // service
    void models_dir_changed();
    void cache_dir_changed();
    void restore_punctuation_changed();
    void default_stt_model_changed();
    void default_stt_models_changed(const QString &lang);
    void default_tts_model_changed();
    void default_tts_models_changed(const QString &lang);
    void default_mnt_lang_changed();
    void default_mnt_out_lang_changed();
    void whisper_use_gpu_changed();
    void gpu_devices_changed();
    void gpu_device_changed();
    void audio_inputs_changed();
    void audio_input_changed();

   private:
    inline static const QString settings_filename =
        QStringLiteral("settings.conf");
    inline static const QString default_qt_style =
        QStringLiteral("org.kde.desktop");
    bool m_restart_required = false;
    QStringList m_gpu_devices;
    QStringList m_audio_inputs;

    static QString settings_filepath();
    void update_qt_style() const;
    void update_gpu_devices();
    void update_audio_inputs();

    launch_mode_t m_launch_mode = launch_mode_t::app_stanalone;
};

#endif  // SETTINGS_H

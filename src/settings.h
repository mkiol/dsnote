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
#ifdef USE_DESKTOP
#include <QQmlApplicationEngine>
#endif

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
    Q_PROPERTY(QString file_audio_open_dir READ file_audio_open_dir WRITE
                   set_file_audio_open_dir NOTIFY file_audio_open_dir_changed)
    Q_PROPERTY(
        QUrl file_audio_open_dir_url READ file_audio_open_dir_url WRITE
            set_file_audio_open_dir_url NOTIFY file_audio_open_dir_changed)
    Q_PROPERTY(QString file_audio_open_dir_name READ file_audio_open_dir_name
                   NOTIFY file_audio_open_dir_changed)
    Q_PROPERTY(QString prev_app_ver READ prev_app_ver WRITE set_prev_app_ver
                   NOTIFY prev_app_ver_changed)
    Q_PROPERTY(bool translator_mode READ translator_mode WRITE
                   set_translator_mode NOTIFY translator_mode_changed)
    Q_PROPERTY(
        bool translate_when_typing READ translate_when_typing WRITE
            set_translate_when_typing NOTIFY translate_when_typing_changed)
    Q_PROPERTY(text_format_t mnt_text_format READ mnt_text_format WRITE
                   set_mnt_text_format NOTIFY mnt_text_format_changed)
    Q_PROPERTY(bool hint_translator READ hint_translator WRITE
                   set_hint_translator NOTIFY hint_translator_changed)
    Q_PROPERTY(int qt_style_idx READ qt_style_idx WRITE set_qt_style_idx NOTIFY
                   qt_style_changed)
    Q_PROPERTY(QString qt_style_name READ qt_style_name WRITE set_qt_style_name
                   NOTIFY qt_style_changed)
    Q_PROPERTY(bool qt_style_auto READ qt_style_auto WRITE set_qt_style_auto
                   NOTIFY qt_style_changed)
    Q_PROPERTY(bool restart_required READ restart_required NOTIFY
                   restart_required_changed)
    Q_PROPERTY(unsigned int speech_speed READ speech_speed WRITE
                   set_speech_speed NOTIFY speech_speed_changed)
    Q_PROPERTY(int font_size READ font_size WRITE set_font_size NOTIFY
                   font_size_changed)
    Q_PROPERTY(audio_format_t audio_format READ audio_format WRITE
                   set_audio_format NOTIFY audio_format_changed)
    Q_PROPERTY(QString audio_format_str READ audio_format_str NOTIFY
                   audio_format_changed)
    Q_PROPERTY(audio_quality_t audio_quality READ audio_quality WRITE
                   set_audio_quality NOTIFY audio_quality_changed)
    Q_PROPERTY(QString mtag_album_name READ mtag_album_name WRITE
                   set_mtag_album_name NOTIFY mtag_album_name_changed)
    Q_PROPERTY(QString mtag_artist_name READ mtag_artist_name WRITE
                   set_mtag_artist_name NOTIFY mtag_artist_name_changed)
    Q_PROPERTY(bool mtag READ mtag WRITE set_mtag NOTIFY mtag_changed)
    Q_PROPERTY(bool hotkeys_enabled READ hotkeys_enabled WRITE
                   set_hotkeys_enabled NOTIFY hotkeys_enabled_changed)
    Q_PROPERTY(QString hotkey_start_listening READ hotkey_start_listening WRITE
                   set_hotkey_start_listening NOTIFY hotkeys_changed)
    Q_PROPERTY(
        QString hotkey_start_listening_active_window READ
            hotkey_start_listening_active_window WRITE
                set_hotkey_start_listening_active_window NOTIFY hotkeys_changed)
    Q_PROPERTY(
        QString hotkey_start_listening_clipboard READ
            hotkey_start_listening_clipboard WRITE
                set_hotkey_start_listening_clipboard NOTIFY hotkeys_changed)
    Q_PROPERTY(QString hotkey_stop_listening READ hotkey_stop_listening WRITE
                   set_hotkey_stop_listening NOTIFY hotkeys_changed)
    Q_PROPERTY(QString hotkey_start_reading READ hotkey_start_reading WRITE
                   set_hotkey_start_reading NOTIFY hotkeys_changed)
    Q_PROPERTY(
        QString hotkey_start_reading_clipboard READ
            hotkey_start_reading_clipboard WRITE
                set_hotkey_start_reading_clipboard NOTIFY hotkeys_changed)
    Q_PROPERTY(
        QString hotkey_pause_resume_reading READ hotkey_pause_resume_reading
            WRITE set_hotkey_pause_resume_reading NOTIFY hotkeys_changed)
    Q_PROPERTY(QString hotkey_cancel READ hotkey_cancel WRITE set_hotkey_cancel
                   NOTIFY hotkeys_changed)
    Q_PROPERTY(
        desktop_notification_policy_t desktop_notification_policy READ
            desktop_notification_policy WRITE set_desktop_notification_policy
                NOTIFY desktop_notification_policy_changed)
    Q_PROPERTY(bool actions_api_enabled READ actions_api_enabled WRITE
                   set_actions_api_enabled NOTIFY actions_api_enabled_changed)
    Q_PROPERTY(bool diacritizer_enabled READ diacritizer_enabled WRITE
                   set_diacritizer_enabled NOTIFY diacritizer_enabled_changed)
    Q_PROPERTY(bool gpu_scan_cuda READ gpu_scan_cuda WRITE set_gpu_scan_cuda
                   NOTIFY gpu_scan_cuda_changed)
    Q_PROPERTY(bool gpu_scan_hip READ gpu_scan_hip WRITE set_gpu_scan_hip NOTIFY
                   gpu_scan_hip_changed)
    Q_PROPERTY(bool gpu_scan_opencl READ gpu_scan_opencl WRITE
                   set_gpu_scan_opencl NOTIFY gpu_scan_opencl_changed)
    Q_PROPERTY(
        bool gpu_scan_opencl_always READ gpu_scan_opencl_always WRITE
            set_gpu_scan_opencl_always NOTIFY gpu_scan_opencl_always_changed)
    Q_PROPERTY(bool stt_use_gpu READ stt_use_gpu WRITE set_stt_use_gpu NOTIFY
                   stt_use_gpu_changed)
    Q_PROPERTY(bool tts_use_gpu READ tts_use_gpu WRITE set_tts_use_gpu NOTIFY
                   tts_use_gpu_changed)
    Q_PROPERTY(QString active_tts_ref_voice READ active_tts_ref_voice WRITE
                   set_active_tts_ref_voice NOTIFY active_tts_ref_voice_changed)
    Q_PROPERTY(QString active_tts_for_in_mnt_ref_voice READ
                   active_tts_for_in_mnt_ref_voice WRITE
                       set_active_tts_for_in_mnt_ref_voice NOTIFY
                           active_tts_for_in_mnt_ref_voice_changed)
    Q_PROPERTY(QString active_tts_for_out_mnt_ref_voice READ
                   active_tts_for_out_mnt_ref_voice WRITE
                       set_active_tts_for_out_mnt_ref_voice NOTIFY
                           active_tts_for_out_mnt_ref_voice_changed)
    Q_PROPERTY(bool mnt_clean_text READ mnt_clean_text WRITE set_mnt_clean_text
                   NOTIFY mnt_clean_text_changed)
    Q_PROPERTY(bool whisper_translate READ whisper_translate WRITE
                   set_whisper_translate NOTIFY whisper_translate_changed)
    Q_PROPERTY(
        bool use_tray READ use_tray WRITE set_use_tray NOTIFY use_tray_changed)
    Q_PROPERTY(bool start_in_tray READ start_in_tray WRITE set_start_in_tray
                   NOTIFY start_in_tray_changed)
    Q_PROPERTY(bool clean_ref_voice READ clean_ref_voice WRITE
                   set_clean_ref_voice NOTIFY clean_ref_voice_changed)
    Q_PROPERTY(
        unsigned int addon_flags READ addon_flags NOTIFY addon_flags_changed)

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

    Q_PROPERTY(QStringList gpu_devices_stt READ gpu_devices_stt NOTIFY
                   gpu_devices_changed)
    Q_PROPERTY(int gpu_device_idx_stt READ gpu_device_idx_stt WRITE
                   set_gpu_device_idx_stt NOTIFY gpu_device_stt_changed)
    Q_PROPERTY(QString gpu_device_stt READ gpu_device_stt WRITE
                   set_gpu_device_stt NOTIFY gpu_device_stt_changed)
    Q_PROPERTY(QString auto_gpu_device_stt READ auto_gpu_device_stt NOTIFY
                   gpu_device_stt_changed)
    Q_PROPERTY(QStringList gpu_devices_tts READ gpu_devices_tts NOTIFY
                   gpu_devices_changed)
    Q_PROPERTY(int gpu_device_idx_tts READ gpu_device_idx_tts WRITE
                   set_gpu_device_idx_tts NOTIFY gpu_device_tts_changed)
    Q_PROPERTY(QString gpu_device_tts READ gpu_device_tts WRITE
                   set_gpu_device_tts NOTIFY gpu_device_tts_changed)
    Q_PROPERTY(QString auto_gpu_device_tts READ auto_gpu_device_tts NOTIFY
                   gpu_device_tts_changed)
    Q_PROPERTY(
        QStringList audio_inputs READ audio_inputs NOTIFY audio_inputs_changed)
    Q_PROPERTY(int audio_input_idx READ audio_input_idx WRITE
                   set_audio_input_idx NOTIFY audio_input_changed)
    Q_PROPERTY(QString audio_input READ audio_input WRITE set_audio_input NOTIFY
                   audio_input_changed)
    Q_PROPERTY(bool py_feature_scan READ py_feature_scan WRITE
                   set_py_feature_scan NOTIFY py_feature_scan_changed)
    Q_PROPERTY(
        cache_audio_format_t cache_audio_format READ cache_audio_format WRITE
            set_cache_audio_format NOTIFY cache_audio_format_changed)
    Q_PROPERTY(cache_policy_t cache_policy READ cache_policy WRITE
                   set_cache_policy NOTIFY cache_policy_changed)
    Q_PROPERTY(int num_threads READ num_threads WRITE set_num_threads NOTIFY
                   num_threads_changed)
    Q_PROPERTY(
        QString py_path READ py_path WRITE set_py_path NOTIFY py_path_changed)
    Q_PROPERTY(bool gpu_override_version READ gpu_override_version WRITE
                   set_gpu_override_version NOTIFY gpu_override_version_changed)
    Q_PROPERTY(
        QString gpu_overrided_version READ gpu_overrided_version WRITE
            set_gpu_overrided_version NOTIFY gpu_overrided_version_changed)

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

    enum class audio_format_t {
        AudioFormatAuto = 0,
        AudioFormatWav = 1,
        AudioFormatMp3 = 2,
        AudioFormatOggVorbis = 3,
        AudioFormatOggOpus = 4
    };
    Q_ENUM(audio_format_t)

    enum class cache_audio_format_t {
        CacheAudioFormatWav = 0,
        CacheAudioFormatMp3 = 1,
        CacheAudioFormatOggVorbis = 2,
        CacheAudioFormatOggOpus = 3,
        CacheAudioFormatFlac = 4
    };
    Q_ENUM(cache_audio_format_t)

    enum class cache_policy_t { CacheRemove = 0, CacheNoRemove = 1 };
    Q_ENUM(cache_policy_t)

    enum class audio_quality_t {
        AudioQualityVbrHigh = 10,
        AudioQualityVbrMedium = 11,
        AudioQualityVbrLow = 12,
    };
    Q_ENUM(audio_quality_t)

    enum class desktop_notification_policy_t {
        DesktopNotificationNever = 0,
        DesktopNotificationWhenInacvtive = 1,
        DesktopNotificationAlways = 2
    };
    Q_ENUM(desktop_notification_policy_t)

    enum class text_format_t {
        TextFormatRaw = 0,
        TextFormatHtml = 1,
        TextFormatMarkdown = 2,
        TextFormatSubRip = 3
    };
    Q_ENUM(text_format_t)

    enum addon_flags_t : unsigned int {
        AddonNone = 0,
        AddonNvidia = 1 << 0,
        AddonAmd = 1 << 1
    };
    Q_ENUM(addon_flags_t)

    settings();

    launch_mode_t launch_mode() const;
    void set_launch_mode(launch_mode_t launch_mode);
    QString module_checksum(const QString &name) const;
    void set_module_checksum(const QString &name, const QString &value);
    void scan_gpu_devices();
    void disable_gpu_scan();
    void disable_py_scan();
#ifdef USE_DESKTOP
    void update_qt_style(QQmlApplicationEngine *engine) const;
#endif
    // app
    QString note() const;
    void set_note(const QString &value);
    speech_mode_t speech_mode() const;
    void set_speech_mode(speech_mode_t value);
    unsigned int speech_speed() const;
    void set_speech_speed(unsigned int value);
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
    Q_INVOKABLE void update_file_save_path(const QString &path);
    QString file_open_dir() const;
    void set_file_open_dir(const QString &value);
    QUrl file_open_dir_url() const;
    void set_file_open_dir_url(const QUrl &value);
    QString file_open_dir_name() const;
    QString file_audio_open_dir() const;
    void set_file_audio_open_dir(const QString &value);
    QUrl file_audio_open_dir_url() const;
    void set_file_audio_open_dir_url(const QUrl &value);
    QString file_audio_open_dir_name() const;
    QString prev_app_ver() const;
    void set_prev_app_ver(const QString &value);
    bool translator_mode() const;
    void set_translator_mode(bool value);
    bool translate_when_typing() const;
    void set_translate_when_typing(bool value);
    void set_mnt_text_format(text_format_t value);
    text_format_t mnt_text_format() const;
    QString default_tts_model_for_mnt_lang(const QString &lang);
    void set_default_tts_model_for_mnt_lang(const QString &lang,
                                            const QString &value);
    bool hint_translator() const;
    void set_hint_translator(bool value);
    int qt_style_idx() const;
    void set_qt_style_idx(int value);
    QString qt_style_name() const;
    void set_qt_style_name(QString value);
    void set_qt_style_auto(bool value);
    bool qt_style_auto() const;
    bool restart_required() const;
    void set_font_size(int value);
    int font_size() const;
    void set_audio_format(audio_format_t value);
    audio_format_t audio_format() const;
    void set_audio_quality(audio_quality_t value);
    audio_quality_t audio_quality() const;
    QString mtag_album_name() const;
    void set_mtag_album_name(const QString &value);
    QString mtag_artist_name() const;
    void set_mtag_artist_name(const QString &value);
    void set_mtag(bool value);
    bool mtag() const;
    QString audio_format_str() const;
    bool hotkeys_enabled() const;
    void set_hotkeys_enabled(bool value);
    QString hotkey_start_listening() const;
    void set_hotkey_start_listening(const QString &value);
    QString hotkey_start_listening_active_window() const;
    void set_hotkey_start_listening_active_window(const QString &value);
    QString hotkey_start_listening_clipboard() const;
    void set_hotkey_start_listening_clipboard(const QString &value);
    QString hotkey_stop_listening() const;
    void set_hotkey_stop_listening(const QString &value);
    QString hotkey_start_reading() const;
    void set_hotkey_start_reading(const QString &value);
    QString hotkey_start_reading_clipboard() const;
    void set_hotkey_start_reading_clipboard(const QString &value);
    QString hotkey_pause_resume_reading() const;
    void set_hotkey_pause_resume_reading(const QString &value);
    QString hotkey_cancel() const;
    void set_hotkey_cancel(const QString &value);
    desktop_notification_policy_t desktop_notification_policy() const;
    void set_desktop_notification_policy(desktop_notification_policy_t value);
    bool actions_api_enabled() const;
    void set_actions_api_enabled(bool value);
    bool diacritizer_enabled() const;
    void set_diacritizer_enabled(bool value);
    bool gpu_scan_cuda() const;
    void set_gpu_scan_cuda(bool value);
    bool gpu_scan_hip() const;
    void set_gpu_scan_hip(bool value);
    bool gpu_scan_opencl() const;
    void set_gpu_scan_opencl(bool value);
    bool gpu_scan_opencl_always() const;
    void set_gpu_scan_opencl_always(bool value);
    bool whisper_use_gpu() const;
    void set_whisper_use_gpu(bool value);
    bool stt_use_gpu() const;
    void set_stt_use_gpu(bool value);
    bool tts_use_gpu() const;
    void set_tts_use_gpu(bool value);
    QString active_tts_ref_voice() const;
    void set_active_tts_ref_voice(const QString &value);
    QString active_tts_for_in_mnt_ref_voice() const;
    void set_active_tts_for_in_mnt_ref_voice(const QString &value);
    QString active_tts_for_out_mnt_ref_voice() const;
    void set_active_tts_for_out_mnt_ref_voice(const QString &value);
    bool mnt_clean_text() const;
    void set_mnt_clean_text(bool value);
    bool whisper_translate() const;
    void set_whisper_translate(bool value);
    bool use_tray() const;
    void set_use_tray(bool value);
    bool start_in_tray() const;
    void set_start_in_tray(bool value);
    bool clean_ref_voice() const;
    void set_clean_ref_voice(bool value);
    unsigned int addon_flags() const;

    Q_INVOKABLE QUrl app_icon() const;
    Q_INVOKABLE bool py_supported() const;
    Q_INVOKABLE bool gpu_supported() const;
    Q_INVOKABLE bool has_gpu_device_stt() const;
    Q_INVOKABLE bool has_gpu_device_tts() const;
    Q_INVOKABLE bool has_audio_input() const;
    Q_INVOKABLE bool is_wayland() const;
    Q_INVOKABLE bool is_xcb() const;
    Q_INVOKABLE bool is_flatpak() const;
    Q_INVOKABLE QStringList qt_styles() const;
    Q_INVOKABLE bool file_exists(const QString &file_path) const;
    Q_INVOKABLE QString
    add_ext_to_audio_filename(const QString &filename) const;
    Q_INVOKABLE QString
    add_ext_to_audio_file_path(const QString &file_path) const;
    Q_INVOKABLE QString
    base_name_from_file_path(const QString &file_path) const;
    Q_INVOKABLE QString file_path_from_url(const QUrl &file_url) const;
    Q_INVOKABLE QString dir_of_file(const QString &file_path) const;
    Q_INVOKABLE audio_format_t
    filename_to_audio_format(const QString &filename) const;
    static QString audio_format_str_from_filename(const QString &filename);
    static QString audio_ext_from_filename(const QString &filename);
    static audio_format_t audio_format_from_filename(const QString &filename);
    static audio_format_t filename_to_audio_format_static(
        const QString &filename);
    Q_INVOKABLE bool is_debug() const;

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
    QStringList audio_inputs() const;
    QString audio_input() const;
    void set_audio_input(QString value);
    int audio_input_idx() const;
    void set_audio_input_idx(int value);
    bool py_feature_scan() const;
    void set_py_feature_scan(bool value);
    int num_threads() const;
    void set_num_threads(int value);
    QString py_path() const;
    void set_py_path(const QString &value);

    QStringList gpu_devices_stt() const;
    QString gpu_device_stt() const;
    QString auto_gpu_device_stt() const;
    void set_gpu_device_stt(QString value);
    int gpu_device_idx_stt() const;
    void set_gpu_device_idx_stt(int value);

    QStringList gpu_devices_tts() const;
    QString gpu_device_tts() const;
    QString auto_gpu_device_tts() const;
    void set_gpu_device_tts(QString value);
    int gpu_device_idx_tts() const;
    void set_gpu_device_idx_tts(int value);

    void set_cache_audio_format(cache_audio_format_t value);
    cache_audio_format_t cache_audio_format() const;
    void set_cache_policy(cache_policy_t value);
    cache_policy_t cache_policy() const;

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

    bool gpu_override_version() const;
    void set_gpu_override_version(bool value);
    QString gpu_overrided_version();
    void set_gpu_overrided_version(QString new_value);

   signals:
    // app
    void speech_mode_changed();
    void note_changed();
    void insert_mode_changed();
    void mode_changed();
    void file_save_dir_changed();
    void file_open_dir_changed();
    void file_audio_open_dir_changed();
    void prev_app_ver_changed();
    void translator_mode_changed();
    void translate_when_typing_changed();
    void default_tts_models_for_mnt_changed(const QString &lang);
    void hint_translator_changed();
    void qt_style_changed();
    void restart_required_changed();
    void speech_speed_changed();
    void font_size_changed();
    void audio_format_changed();
    void audio_quality_changed();
    void mtag_album_name_changed();
    void mtag_artist_name_changed();
    void mtag_changed();
    void hotkeys_enabled_changed();
    void hotkeys_changed();
    void desktop_notification_policy_changed();
    void actions_api_enabled_changed();
    void diacritizer_enabled_changed();
    void gpu_scan_cuda_changed();
    void gpu_scan_hip_changed();
    void gpu_scan_opencl_changed();
    void gpu_scan_opencl_always_changed();
    void whisper_use_gpu_changed();
    void stt_use_gpu_changed();
    void tts_use_gpu_changed();
    void active_tts_ref_voice_changed();
    void active_tts_for_in_mnt_ref_voice_changed();
    void active_tts_for_out_mnt_ref_voice_changed();
    void mnt_clean_text_changed();
    void whisper_translate_changed();
    void use_tray_changed();
    void start_in_tray_changed();
    void mnt_text_format_changed();
    void clean_ref_voice_changed();
    void addon_flags_changed();

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
    void gpu_devices_changed();
    void gpu_device_stt_changed();
    void gpu_device_tts_changed();
    void audio_inputs_changed();
    void audio_input_changed();
    void py_feature_scan_changed();
    void cache_audio_format_changed();
    void cache_policy_changed();
    void num_threads_changed();
    void py_path_changed();
    void gpu_override_version_changed();
    void gpu_overrided_version_changed();

   private:
    inline static const QString settings_filename =
        QStringLiteral("settings.conf");
    inline static const QString default_qt_style =
        QStringLiteral("org.kde.desktop");
    inline static const QString default_qt_style_fallback =
        QStringLiteral("org.kde.breeze");
    bool m_restart_required = false;
    QStringList m_gpu_devices_stt;
    QStringList m_gpu_devices_tts;
    std::vector<QString> m_rocm_gpu_versions;
    QStringList m_audio_inputs;
    unsigned int m_addon_flags = addon_flags_t::AddonNone;

    static QString settings_filepath();
    void update_audio_inputs();
    void set_restart_required(bool value);
    void enforce_num_threads() const;
    void update_addon_flags();

    launch_mode_t m_launch_mode = launch_mode_t::app_stanalone;
};

#endif  // SETTINGS_H

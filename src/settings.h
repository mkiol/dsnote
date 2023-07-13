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
    Q_PROPERTY(bool translate READ translate WRITE set_translate NOTIFY
                   translate_changed)
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
    bool translate() const;
    void set_translate(bool value);
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

    Q_INVOKABLE QUrl app_icon() const;
    Q_INVOKABLE bool py_supported() const;

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
    void translate_changed();
    void insert_mode_changed();
    void mode_changed();
    void file_save_dir_changed();
    void file_open_dir_changed();
    void prev_app_ver_changed();
    void translator_mode_changed();
    void translate_when_typing_changed();
    void default_tts_models_for_mnt_changed(const QString &lang);
    void hint_translator_changed();

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

   private:
    inline static const QString settings_filename =
        QStringLiteral("settings.conf");
    static QString settings_filepath();

    launch_mode_t m_launch_mode = launch_mode_t::app_stanalone;
};

#endif  // SETTINGS_H

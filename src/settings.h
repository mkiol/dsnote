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

    // service
    Q_PROPERTY(QString models_dir READ models_dir WRITE set_models_dir NOTIFY
                   models_dir_changed)
    Q_PROPERTY(QUrl models_dir_url READ models_dir_url WRITE set_models_dir_url
                   NOTIFY models_dir_changed)
    Q_PROPERTY(
        QString models_dir_name READ models_dir_name NOTIFY models_dir_changed)
    Q_PROPERTY(QString default_model READ default_model WRITE set_default_model
                   NOTIFY default_model_changed)

   public:
    enum class launch_mode_t { app_stanalone, app, stt_service };
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

    // app
    QString note() const;
    void set_note(const QString &value);
    speech_mode_t speech_mode() const;
    void set_speech_mode(speech_mode_t value);
    bool translate() const;
    void set_translate(bool value);
    insert_mode_t insert_mode() const;
    void set_insert_mode(insert_mode_t value);
    Q_INVOKABLE QUrl app_icon() const;

    // service
    QString models_dir() const;
    void set_models_dir(const QString &value);
    QUrl models_dir_url() const;
    void set_models_dir_url(const QUrl &value);
    QString models_dir_name() const;
    QString default_model() const;
    void set_default_model(const QString &value);
    QStringList enabled_models();
    void set_enabled_models(const QStringList &value);
    QString default_model_for_lang(const QString &lang);
    void set_default_model_for_lang(const QString &lang, const QString &value);

   signals:
    // app
    void speech_mode_changed();
    void note_changed();
    void translate_changed();
    void insert_mode_changed();

    // service
    void models_dir_changed();
    void default_model_changed();
    void default_models_changed(const QString &lang);

   private:
    inline static const QString settings_filename =
        QStringLiteral("settings.conf");
    static QString settings_filepath();

    launch_mode_t m_launch_mode = launch_mode_t::app_stanalone;
};

#endif  // SETTINGS_H

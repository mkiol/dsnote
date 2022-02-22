/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
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
#include <QUrl>

class settings : public QSettings {
    Q_OBJECT

    // app
    Q_PROPERTY(QString note READ note WRITE set_note NOTIFY note_changed)
    Q_PROPERTY(speech_mode_type speech_mode READ speech_mode WRITE
                   set_speech_mode NOTIFY speech_mode_changed)

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
    enum class speech_mode_type {
        SpeechAutomatic = 0,
        SpeechManual = 1,
        SpeechSingleSentence = 2
    };
    Q_ENUM(speech_mode_type)

    static settings *instance();

    // app
    QString note() const;
    void set_note(const QString &value);
    speech_mode_type speech_mode() const;
    void set_speech_mode(speech_mode_type value);
    Q_INVOKABLE QUrl app_icon() const;

    // service
    QString models_dir() const;
    void set_models_dir(const QString &value);
    QUrl models_dir_url() const;
    void set_models_dir_url(const QUrl &value);
    QString models_dir_name() const;
    QString default_model() const;
    void set_default_model(const QString &value);

   signals:
    // app
    void speech_mode_changed();
    void note_changed();

    // service
    void models_dir_changed();
    void default_model_changed();

   private:
    static settings *m_instance;
    settings() = default;
};

#endif  // SETTINGS_H

/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QObject>
#include <QString>
#include <QUrl>

class settings : public QSettings
{
    Q_OBJECT

    Q_PROPERTY (QString note READ note WRITE set_note NOTIFY note_changed)
    Q_PROPERTY (QString lang_models_dir READ lang_models_dir WRITE set_lang_models_dir NOTIFY lang_models_dir_changed)
    Q_PROPERTY (QUrl lang_models_dir_url READ lang_models_dir_url WRITE set_lang_models_dir_url NOTIFY lang_models_dir_changed)
    Q_PROPERTY (QString lang_models_dir_name READ lang_models_dir_name NOTIFY lang_models_dir_changed)
    Q_PROPERTY (QString lang READ lang WRITE set_lang NOTIFY lang_changed)
    Q_PROPERTY (speech_mode_type speech_mode READ speech_mode WRITE set_speech_mode NOTIFY speech_mode_changed)

public:
    enum class speech_mode_type { SpeechAutomatic = 0, SpeechManual = 1 };
    Q_ENUM(speech_mode_type)

    static settings* instance();

    QString lang_models_dir() const;
    void set_lang_models_dir(const QString& value);
    QUrl lang_models_dir_url() const;
    void set_lang_models_dir_url(const QUrl& value);
    QString lang_models_dir_name() const;
    QString note() const;
    void set_note(const QString& value);

    QString lang() const;
    void set_lang(const QString& value);

    speech_mode_type speech_mode() const;
    void set_speech_mode(speech_mode_type value);

    Q_INVOKABLE QUrl app_icon() const;

signals:
    void lang_models_dir_changed();
    void lang_changed();
    void speech_mode_changed();
    void note_changed();

private:
    static settings* m_instance;

    settings(QObject *parent = nullptr);
};

#endif // SETTINGS_H

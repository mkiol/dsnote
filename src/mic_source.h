/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MIC_SOURCE_H
#define MIC_SOURCE_H

#include <QObject>
#include <QAudioInput>
#include <QIODevice>
#include <QTimer>

#include <memory>

class mic_source : public QObject
{
    Q_OBJECT
public:
    explicit mic_source(QObject *parent = nullptr);
    ~mic_source();
    bool ok() const;
    int64_t read_audio(char* buff, int64_t max_size);

signals:
    void audio_available();

private slots:
    void handle_state_changed(QAudio::State new_state);
    void handle_read_timeout();

private:
    std::unique_ptr<QAudioInput> audio_input;
    QTimer timer;
    QIODevice* audio_device;

    void init_audio();
    void start();
};

#endif // MIC_SOURCE_H

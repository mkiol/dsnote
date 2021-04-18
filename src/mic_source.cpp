/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mic_source.h"

#include <QAudioFormat>
#include <QDebug>

mic_source::mic_source(QObject *parent) : QObject{parent}
{
    init_audio();
    start();
}

mic_source::~mic_source()
{
    timer.stop();
    audio_input->stop();
}

bool mic_source::ok() const
{
    return audio_input ? true : false;
}

void mic_source::init_audio()
{
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    auto info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format)) {
        qWarning() << "mic audio format is not supported";
        return;
    }

    audio_input = std::make_unique<QAudioInput>(format);

    connect(audio_input.get(), &QAudioInput::stateChanged, this, &mic_source::handle_state_changed);
}

void mic_source::start()
{
    audio_device = audio_input->start();

    timer.setInterval(200); // 200 ms
    connect(&timer, &QTimer::timeout, this, &mic_source::handle_read_timeout);
    timer.start();
}

void mic_source::handle_state_changed(QAudio::State new_state)
{
    qDebug() << "audio state:" << new_state;
}

void mic_source::handle_read_timeout()
{
    emit audio_available();
}

int64_t mic_source::read_audio(char* buff, int64_t max_size)
{
    return audio_device->read(buff, max_size);
}

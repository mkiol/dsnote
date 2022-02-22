/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mic_source.h"

#include <QAudioFormat>
#include <QDebug>

mic_source::mic_source(QObject* parent) : audio_source{parent} {
    init_audio();
    start();
}

mic_source::~mic_source() {
    timer.stop();
    audio_input->stop();
}

bool mic_source::ok() const { return audio_input ? true : false; }

void mic_source::init_audio() {
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    auto info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format)) {
        throw std::runtime_error("mic audio format is not supported");
    }

    audio_input = std::make_unique<QAudioInput>(format);

    connect(audio_input.get(), &QAudioInput::stateChanged, this,
            &mic_source::handle_state_changed);
}

void mic_source::start() {
    audio_device = audio_input->start();

    timer.setInterval(200);  // 200 ms
    connect(&timer, &QTimer::timeout, this, &mic_source::handle_read_timeout);
    timer.start();
}

void mic_source::handle_state_changed(QAudio::State new_state) {
    qDebug() << "audio state:" << new_state;
}

void mic_source::handle_read_timeout() {
    if ((audio_input->state() != QAudio::ActiveState &&
         audio_input->state() != QAudio::IdleState) ||
        audio_input->error() != QAudio::NoError) {
        if (audio_input->error() != QAudio::NoError)
            qWarning() << "audio input error:" << audio_input->error();
        timer.stop();
        emit error();
        return;
    }

    emit audio_available();
}

void mic_source::clear() {
    char buff[std::numeric_limits<short>::max()];
    while (audio_device->read(buff, std::numeric_limits<short>::max()))
        continue;
}

int64_t mic_source::read_audio(char* buff, int64_t max_size) {
    if (audio_input->state() != QAudio::ActiveState &&
        audio_input->state() != QAudio::IdleState) {
        qWarning() << "audio input is not active and cannot read audio";
        return 0;
    }

    return audio_device->read(buff, max_size);
}

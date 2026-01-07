/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mic_source.h"

#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QDebug>

mic_source::mic_source(const QString& preferred_audio_input, QObject* parent)
    : audio_source{parent} {
    qDebug() << "mic source created";
    init_audio(preferred_audio_input);
    start();
}

mic_source::~mic_source() {
    qDebug() << "mic source dtor";
    m_audio_input->suspend();

    m_stopped = true;
}

bool mic_source::ok() const { return static_cast<bool>(m_audio_input); }

void mic_source::stop() {
    qDebug() << "mic source stop";
    m_audio_input->suspend();
    m_stopped = true;
}

void mic_source::slowdown() {
}

void mic_source::speedup() {
}

static QAudioFormat audio_format() {
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    return format;
}

static bool has_audio_input(const QString& name) {
    auto ad_list = QMediaDevices::audioInputs();
    return std::find_if(ad_list.cbegin(), ad_list.cend(),
                        [&name](const auto& ad) {
                            return ad.description() == name;
                        }) != ad_list.cend();
}

static QAudioDevice audio_input_info(const QString& name) {
    auto ad_list = QMediaDevices::audioInputs();
    auto it = std::find_if(
        ad_list.cbegin(), ad_list.cend(),
        [&name](const auto& ad) { return ad.description() == name; });
    if (it != ad_list.cend()) {
        return *it;
    }
    return QMediaDevices::defaultAudioInput();
}

QStringList mic_source::audio_inputs() {
    QStringList list;

    auto format = audio_format();

    auto ad_list = QMediaDevices::audioInputs();
    qDebug() << "supported audio input devices:";
    for (const auto& ad : ad_list) {
        if (ad.isFormatSupported(format)) {
            qDebug() << ad.description();
            list.push_back(ad.description());
        }
    }

    if (list.isEmpty()) qWarning() << "no supported audio input device";

    return list;
}

void mic_source::init_audio(const QString& preferred_audio_input) {
    auto format = audio_format();

    auto input_name{preferred_audio_input};
    if (preferred_audio_input.isEmpty() ||
        !has_audio_input(preferred_audio_input)) {
        auto info = QMediaDevices::defaultAudioInput();
        if (info.isNull()) {
            qWarning() << "no audio input";
            throw std::runtime_error("no audio input");
        }

        input_name = info.description();
    }

    auto input_info = audio_input_info(input_name);
    if (!input_info.isFormatSupported(format)) {
        qWarning() << "format not supported for audio input:"
                   << input_info.description();
        throw std::runtime_error("audio format is not supported");
    }

    qDebug() << "using audio input:" << input_info.description()
             << "(preferred was " << preferred_audio_input << ")";
    m_audio_input = std::make_unique<QAudioSource>(input_info, format);

    connect(m_audio_input.get(), &QAudioSource::stateChanged, this,
            &mic_source::handle_state_changed);
}

void mic_source::start() {
    m_audio_device = m_audio_input->start();

    m_timer.setInterval(200);
    connect(&m_timer, &QTimer::timeout, this, &mic_source::handle_read_timeout);
    m_timer.start();
}

void mic_source::handle_state_changed(QAudio::State new_state) {
    qDebug() << "audio state:" << new_state;

    if (new_state == QAudio::State::StoppedState ||
        new_state == QAudio::State::SuspendedState || m_stopped) {
        qDebug() << "audio ended";
        if (m_audio_input->error() == QAudio::NoError) m_eof = true;
    }
}

void mic_source::handle_read_timeout() {
    if (m_audio_input->error() != QAudio::NoError) {
        qWarning() << "audio input error:" << m_audio_input->error();
        m_timer.stop();
        emit error();
        return;
    }

    if (m_stopped && m_audio_input->state() != QAudio::State::SuspendedState)
        stop();

    if (m_ended) {
        emit ended();
        m_timer.stop();
        return;
    }

    emit audio_available();
}

mic_source::audio_data mic_source::read_audio(char* buf, size_t max_size) {
    audio_data data;
    data.data = buf;
    data.sof = m_sof;

    bool bytes_available = !m_eof || m_audio_input->bytesAvailable() > 0;

    if (!bytes_available) {
        data.eof = m_eof;
        if (data.eof) m_ended = true;
        return data;
    }

    data.size = static_cast<size_t>(m_audio_device->read(buf, static_cast<qint64>(max_size)));
    data.eof = m_eof && !bytes_available;

    m_sof = false;

    if (data.eof) m_ended = true;

    return data;
}

void mic_source::clear() { m_audio_device->readAll(); }

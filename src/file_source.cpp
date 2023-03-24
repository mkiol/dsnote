/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "file_source.h"

#include <QAudioFormat>
#include <numeric>

file_source::file_source(const QString &file, QObject *parent)
    : audio_source{parent}, m_file{file} {
    init_audio();
    start();
}

bool file_source::ok() const {
    return m_decoder.error() == QAudioDecoder::NoError;
}

void file_source::init_audio() {
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    m_decoder.setAudioFormat(format);
    m_decoder.setSourceFilename(m_file);

    connect(&m_decoder, &QAudioDecoder::stateChanged, this,
            &file_source::handle_state_changed);
}

void file_source::stop() {
    qDebug() << "file source stop";
    m_decoder.stop();
}

void file_source::start() {
    m_decoder.start();

    m_timer.setInterval(200);  // 200 ms
    connect(&m_timer, &QTimer::timeout, this,
            &file_source::handle_read_timeout);
    m_timer.start();
}

void file_source::handle_state_changed(QAudioDecoder::State new_state) {
    qDebug() << "audio state:" << new_state;

    if (new_state == QAudioDecoder::StoppedState) {
        qDebug() << "decoding ended";
        if (m_decoder.error() == QAudioDecoder::NoError) m_eof = true;
    }
}

void file_source::handle_read_timeout() {
    if (m_decoder.error() != QAudioDecoder::NoError) {
        qWarning() << "audio decoder error:" << m_decoder.errorString();
        m_decoder.stop();
        m_timer.stop();
        emit error();
        return;
    }

    if (m_decoder.bufferAvailable() || !m_buf.empty() || (m_eof && !m_ended)) {
        emit audio_available();

        if (m_eof && m_buf.empty()) {
            emit ended();
            m_ended = true;
        }
    }
}

void file_source::clear() {
    while (m_decoder.bufferAvailable()) m_decoder.read();
}

double file_source::progress() const {
    if (m_decoder.duration() <= 0) return -1;

    if (m_decoder.position() <= 0) return 0;

    return static_cast<double>(m_decoder.position()) / m_decoder.duration();
}

void file_source::decode_available_buffer() {
    if (!m_decoder.bufferAvailable()) return;

    auto audio_buf = m_decoder.read();

    if (!audio_buf.isValid()) {
        qWarning() << "audio buf is invalid";
        return;
    }

    auto size = static_cast<size_t>(audio_buf.byteCount());

    if (size <= 0) {
        qWarning() << "audio buf empty";
        return;
    }

    m_buf.reserve(m_buf.size() + size);

    auto *src = audio_buf.data<char>();

    for (size_t i = 0; i < size; ++i) m_buf.push_back(src[i]);
}

static void shift_left(std::vector<char> &vec, size_t distance) {
    if (distance >= vec.size()) {
        vec.clear();
        return;
    }

    auto beg = vec.cbegin();
    std::advance(beg, distance);

    std::copy(beg, vec.cend(), vec.begin());

    vec.resize(vec.size() - distance);
}

file_source::audio_data file_source::read_audio(char *buf, size_t max_size) {
    decode_available_buffer();

    audio_data data;
    data.data = buf;
    data.sof = m_sof;

    if (m_buf.empty()) {
        data.eof = m_eof;
        m_ended = false;
        return data;
    }

    data.size = std::min(max_size, m_buf.size());

    memcpy(buf, m_buf.data(), data.size);

    shift_left(m_buf, data.size);  // TO-DO: avoid huge mem copy

    data.eof = m_eof && m_buf.empty();

    m_sof = false;

    return data;
}

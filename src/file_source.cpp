/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "file_source.h"

#include <QAudioFormat>

file_source::file_source(const QString &file, QObject *parent) : audio_source{parent}, file{file}
{
    init_audio();
    start();
}

bool file_source::ok() const
{
    return decoder.error() == QAudioDecoder::NoError;
}

void file_source::init_audio()
{
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    decoder.setAudioFormat(format);
    decoder.setSourceFilename(file);

    connect(&decoder, &QAudioDecoder::stateChanged, this, &file_source::handle_state_changed);
}

void file_source::start()
{
    decoder.start();

    timer.setInterval(200); // 200 ms
    connect(&timer, &QTimer::timeout, this, &file_source::handle_read_timeout);
    timer.start();
}

void file_source::handle_state_changed(QAudioDecoder::State new_state)
{
    qDebug() << "audio state:" << new_state;

    if (new_state == QAudioDecoder::StoppedState) {
        qDebug() << "Decoding ended";
        if (decoder.error() == QAudioDecoder::NoError) {
            emit ended();
        }
    }
}

void file_source::handle_read_timeout()
{
    if (decoder.error() != QAudioDecoder::NoError) {
        qWarning() << "audio decoder error:" << decoder.errorString();
        decoder.stop();
        timer.stop();
        emit error();
        return;
    }

    if (decoder.bufferAvailable() || buff_size > 0) {
        emit audio_available();
    }
}

void file_source::clear()
{
    while (decoder.bufferAvailable()) decoder.read();
}

double file_source::progress() const
{
    if (decoder.duration() <= 0)
        return -1;

    if (decoder.position() <= 0) return 0;

    return static_cast<double>(decoder.position()) / decoder.duration();
}

int64_t file_source::read_audio(char* buff, int64_t max_size)
{
    int64_t size = 0;
    if (this->buff_size > 0) {
        memcpy(buff, &this->buff.front(), this->buff_size);
        buff += this->buff_size;
        size = this->buff_size;
        this->buff_size = 0;
    }

    while (decoder.bufferAvailable() && size <= max_size) {
        auto abuff = decoder.read();
        if (!abuff.isValid()) break;

        char *src = abuff.data<char>();

        auto asize = static_cast<int64_t>(abuff.byteCount());
        auto size_to_return = std::min(max_size - size, asize);
        auto size_to_buff = asize - size_to_return;

        if (size_to_return > 0) {
            memcpy(buff, src, size_to_return);
            size += size_to_return;
            buff += size_to_return;
        }

        if (size_to_buff > 0) {
            memcpy(&this->buff.front(), src, size_to_buff);
            this->buff_size = size_to_buff;
            break;
        }
    }

    return size;
}

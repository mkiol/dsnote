/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "file_source.h"

#include <QDebug>

file_source::file_source(const QString &file, int stream_index, QObject *parent)
    : audio_source{parent}, m_file{file}, m_stream_index{stream_index} {
    start();
}

bool file_source::ok() const { return m_error; }

void file_source::stop() {
    qDebug() << "file source stop";
    m_mc.cancel();
}

void file_source::slowdown() {
    if (m_timer.interval() == m_timer_slow) return;

    // qDebug() << "slowdown";
    m_timer.setInterval(m_timer_slow);
    m_timer.start();
}

void file_source::speedup() {
    if (m_timer.interval() == m_timer_quick) return;

    // qDebug() << "speedup";
    m_timer.setInterval(m_timer_quick);
    m_timer.start();
}

void file_source::start() {
    connect(&m_timer, &QTimer::timeout, this,
            &file_source::handle_read_timeout);
    m_timer.setInterval(m_timer_quick);
    m_timer.start();

    auto stream = m_stream_index >= 0
                      ? std::make_optional<media_compressor::stream_t>(
                            media_compressor::stream_t{
                                m_stream_index,
                                media_compressor::media_type_t::audio,
                                {},
                                {}})
                      : std::nullopt;

    media_compressor::options_t opts{
        media_compressor::quality_t::vbr_medium,
        media_compressor::flags_t::flag_force_mono_output |
            media_compressor::flags_t::flag_force_16k_sample_rate_output,
        1.0, stream,
        /*clip_info=*/{}};

    m_mc.decompress_to_data_raw_async({m_file.toStdString()},
                                      /*options=*/
                                      opts,
                                      /*data_ready_callback=*/{},
                                      /*task_finished_callback=*/{});
}

void file_source::handle_read_timeout() {
    if (m_mc.error()) {
        qWarning() << "audio decoder error";
        m_timer.stop();
        emit error();
        return;
    }

    if (!m_eof) {
        emit audio_available();
    } else if (m_eof && !m_ended) {
        emit audio_available();

        m_ended = true;
        emit ended();
        m_timer.stop();
    }
}

void file_source::clear() {
    // do nothing
}

double file_source::progress() const { return m_progress; }

file_source::audio_data file_source::read_audio(char *buf, size_t max_size) {
    audio_data data;
    data.data = buf;
    data.sof = m_sof;

    auto info = m_mc.get_data(data.data, max_size);

    data.size = info.size;
    data.eof = info.eof;

    m_eof = data.eof;
    m_sof = false;
    m_progress =
        std::min(1.0, info.bytes_read / static_cast<double>(info.total));

    return data;
}

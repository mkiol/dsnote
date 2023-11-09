/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "file_source.h"

#include <QDebug>

file_source::file_source(const QString &file, QObject *parent)
    : audio_source{parent}, m_file{file} {
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

    m_mc.decompress_to_raw_async({m_file.toStdString()}, /*mono_16khz=*/true,
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

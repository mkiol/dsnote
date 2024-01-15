/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef FILE_SOURCE_H
#define FILE_SOURCE_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>

#include "audio_source.h"
#include "media_compressor.hpp"

class file_source : public audio_source {
    Q_OBJECT
   public:
    explicit file_source(const QString &file, int stream_index,
                         QObject *parent = nullptr);
    bool ok() const override;
    audio_data read_audio(char *buf, size_t max_size) override;
    double progress() const override;
    void clear() override;
    void stop() override;
    void slowdown() override;
    void speedup() override;
    inline source_type type() const override { return source_type::file; }
    inline QString audio_file() const { return m_file; }

   private:
    static const int m_timer_quick = 5;
    static const int m_timer_slow = 100;

    QTimer m_timer;
    QByteArray m_buf;
    QString m_file;
    bool m_eof = false;
    bool m_sof = true;
    bool m_ended = false;
    bool m_error = false;
    media_compressor m_mc;
    double m_progress = 0.0;
    int m_stream_index = -1;

    void start();
    void handle_read_timeout();
};

#endif  // FILE_SOURCE_H

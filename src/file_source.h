/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef FILE_SOURCE_H
#define FILE_SOURCE_H

#include <QAudioDecoder>
#include <QObject>
#include <QString>
#include <QTimer>
#include <cstdint>
#include <vector>

#include "audio_source.h"

class file_source : public audio_source {
    Q_OBJECT
   public:
    explicit file_source(const QString &file, QObject *parent = nullptr);
    bool ok() const override;
    audio_data read_audio(char *buf, size_t max_size) override;
    double progress() const override;
    void clear() override;
    void stop() override;
    inline source_type type() const override { return source_type::file; }
    inline QString audio_file() const { return m_file; }

   private:
    QAudioDecoder m_decoder;
    QTimer m_timer;
    std::vector<char> m_buf;
    QString m_file;
    bool m_eof = false;
    bool m_sof = true;
    bool m_ended = false;

    void init_audio();
    void start();
    void decode_available_buffer();
    void handle_state_changed(QAudioDecoder::State new_state);
    void handle_read_timeout();
};

#endif  // FILE_SOURCE_H

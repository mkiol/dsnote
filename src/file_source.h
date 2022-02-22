/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
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

#include "audio_source.h"

class file_source : public audio_source {
    Q_OBJECT
   public:
    explicit file_source(const QString &file, QObject *parent = nullptr);
    bool ok() const override;
    int64_t read_audio(char *buff, int64_t max_size) override;
    double progress() const override;
    void clear() override;
    inline source_type type() const override { return source_type::file; }
    inline QString audio_file() const { return file; };

   private slots:
    void handle_state_changed(QAudioDecoder::State new_state);
    void handle_read_timeout();

   private:
    static const size_t buff_max_size = 16000;

    QAudioDecoder decoder;
    QTimer timer;
    std::array<char, buff_max_size> buff;
    size_t buff_size = 0;
    QString file;

    void init_audio();
    void start();
};

#endif  // FILE_SOURCE_H

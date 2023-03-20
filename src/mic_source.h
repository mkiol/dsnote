/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MIC_SOURCE_H
#define MIC_SOURCE_H

#include <QAudioInput>
#include <QIODevice>
#include <QObject>
#include <QTimer>
#include <memory>

#include "audio_source.h"

class mic_source : public audio_source {
    Q_OBJECT
   public:
    explicit mic_source(QObject* parent = nullptr);
    ~mic_source() override;
    bool ok() const override;
    audio_data read_audio(char* buf, size_t max_size) override;
    void clear() override;
    inline source_type type() const override { return source_type::mic; }

   private:
    std::unique_ptr<QAudioInput> audio_input;
    QTimer timer;
    QIODevice* audio_device;
    int m_sof = true;

    void init_audio();
    void start();
    void handle_state_changed(QAudio::State new_state);
    void handle_read_timeout();
};

#endif  // MIC_SOURCE_H

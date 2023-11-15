/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef RECORDER_H
#define RECORDER_H

#include <QAudioInput>
#include <QFile>
#include <QIODevice>
#include <QObject>
#include <QString>
#include <cstdint>
#include <memory>
#include <thread>

class recorder : public QObject {
    Q_OBJECT
   public:
    explicit recorder(QString wav_file_path, QObject* parent = nullptr);
    ~recorder() final;
    void start();
    void stop();
    bool recording() const;
    bool processing() const;
    long long duration() const;

   signals:
    void recording_changed();
    void duration_changed();
    void processing_changed();

   private:
    struct wav_header {
        uint8_t RIFF[4] = {'R', 'I', 'F', 'F'};
        uint32_t chunk_size = 0;
        uint8_t WAVE[4] = {'W', 'A', 'V', 'E'};
        uint8_t fmt[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_size = 16;
        uint16_t audio_format = 1;
        uint16_t num_channels = 0;
        uint32_t sample_rate = 0;
        uint32_t bytes_per_sec = 0;
        uint16_t block_align = 2;
        uint16_t bits_per_sample = 16;
        uint8_t data[4] = {'d', 'a', 't', 'a'};
        uint32_t data_size = 0;
    };

    std::unique_ptr<QAudioInput> m_audio_input;
    QString m_wav_file_path;
    QFile m_audio_device;
    long long m_duration = 0;
    std::thread m_processing_thread;
    bool m_processing = false;

    void process();
    void export_to_file();
};

#endif  // RECORDER_H

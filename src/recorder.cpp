/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "recorder.hpp"

#include <QAudioFormat>
#include <QDebug>

#include "denoiser.hpp"
#include "settings.h"

static const int sample_rate = 44100;

static QAudioFormat audio_format() {
    QAudioFormat format;
    format.setSampleRate(sample_rate);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    return format;
}

static bool has_audio_input(const QString& name) {
    auto ad_list = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    return std::find_if(ad_list.cbegin(), ad_list.cend(),
                        [&name](const auto& ad) {
                            return ad.deviceName() == name;
                        }) != ad_list.cend();
}

static QAudioDeviceInfo audio_input_info(const QString& name) {
    auto ad_list = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    return *std::find_if(
        ad_list.cbegin(), ad_list.cend(),
        [&name](const auto& ad) { return ad.deviceName() == name; });
}

recorder::recorder(QString wav_file_path, QObject* parent)
    : QObject{parent}, m_wav_file_path{std::move(wav_file_path)} {
    auto format = audio_format();

    auto input_name = settings::instance()->audio_input();
    if (input_name.isEmpty() || !has_audio_input(input_name)) {
        auto info = QAudioDeviceInfo::defaultInputDevice();
        if (info.isNull()) {
            qWarning() << "no audio input";
            throw std::runtime_error("no audio input");
        }

        input_name = info.deviceName();
    }

    auto input_info = audio_input_info(input_name);
    if (!input_info.isFormatSupported(format)) {
        qWarning() << "format not supported for audio input:"
                   << input_info.deviceName();
        throw std::runtime_error("audio format is not supported");
    }

    qDebug() << "using audio input:" << input_info.deviceName();
    m_audio_input = std::make_unique<QAudioInput>(input_info, format);

    connect(m_audio_input.get(), &QAudioInput::stateChanged, this,
            [this](QAudio::State new_state) {
                qDebug() << "recorder state:" << new_state;

                emit recording_changed();
            });
    connect(m_audio_input.get(), &QAudioInput::notify, this, [this]() {
        m_duration = m_audio_input->elapsedUSecs() / 1000000;
        emit duration_changed();
    });
}

recorder::~recorder() {
    if (m_processing_thread.joinable()) m_processing_thread.join();
}

long long recorder::duration() const { return m_duration; }

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

static void write_wav_header(int sample_rate, int sample_width, int channels,
                             uint32_t num_samples, QFile& wav_file) {
    wav_header header;
    header.data_size = num_samples * sample_width * channels;
    header.chunk_size = header.data_size + sizeof(wav_header) - 8;
    header.sample_rate = sample_rate;
    header.num_channels = channels;
    header.bytes_per_sec = sample_rate * sample_width * channels;
    header.block_align = sample_width * channels;

    wav_file.write(reinterpret_cast<const char*>(&header), sizeof(wav_header));
}

void recorder::start() {
    qDebug() << "recorder start";

    m_audio_device.close();

    m_duration = 0;
    emit duration_changed();

    m_audio_device.setFileName(m_wav_file_path);
    m_audio_device.open(QIODevice::OpenModeFlag::ReadWrite);
    m_audio_device.seek(sizeof(wav_header));
    m_audio_input->start(&m_audio_device);
}

static void denoise(QFile& file) {
    file.seek(sizeof(wav_header));

    auto data = file.readAll();

    denoiser{sample_rate}.process_char(data.data(), data.size());

    file.seek(sizeof(wav_header));

    file.write(data);
}

void recorder::stop() {
    qDebug() << "recorder stop";

    m_audio_input->stop();

    unsigned long pos = m_audio_device.pos();
    if (pos <= sizeof(wav_header)) {
        qWarning() << "invalid size in recorded file";
        m_audio_device.close();
        return;
    }

    process();
}

void recorder::export_to_file() {
    unsigned long pos = m_audio_device.pos();

    m_audio_device.seek(0);

    write_wav_header(sample_rate, 2, 1, (pos - sizeof(wav_header)) / 2,
                     m_audio_device);

    m_audio_device.close();
    m_audio_input->reset();

    m_duration = 0;
    emit duration_changed();
}

void recorder::process() {
    if (m_processing_thread.joinable()) m_processing_thread.join();

    m_processing_thread = std::thread{[this] {
        qDebug() << "recorder processing started";
        denoise(m_audio_device);
        export_to_file();
        qDebug() << "recorder processing ended";

        m_processing = false;
        emit processing_changed();
    }};

    m_processing = true;
    emit processing_changed();
}

bool recorder::recording() const {
    return m_audio_input->state() == QAudio::State::ActiveState;
}

bool recorder::processing() const { return m_processing; }

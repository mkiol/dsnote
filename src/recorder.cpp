/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "recorder.hpp"

#include <QAudioFormat>
#include <QDebug>
#include <QFileInfo>
#include <chrono>

#include "denoiser.hpp"
#include "media_compressor.hpp"
#include "settings.h"

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
    init();
}

recorder::recorder(QString input_file_path, QString wav_file_path,
                   QObject* parent)
    : QObject{parent},
      m_input_file_path{std::move(input_file_path)},
      m_wav_file_path{std::move(wav_file_path)} {
    init();
}

recorder::~recorder() {
    cancel();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    if (m_audio_input) m_audio_input->stop();
}

void recorder::cancel() {
    qDebug() << "recorder cancel requested";
    m_cancel_requested = true;
}

void recorder::init() {
    if (!m_input_file_path.isEmpty()) {
        if (!QFileInfo::exists(m_input_file_path)) {
            qCritical() << "cannot open file:" << m_input_file_path;
            throw std::runtime_error{"cannot open file: " +
                                     m_input_file_path.toStdString()};
        }

        qDebug() << "using existing file for recording";
    } else {
        qDebug() << "using mic for recording";

        auto format = make_audio_format();

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
}

QAudioFormat recorder::make_audio_format() {
    QAudioFormat format;
    format.setSampleRate(m_sample_rate);
    format.setChannelCount(m_num_channels);
    format.setSampleSize(16);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    return format;
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
    if (!m_audio_input) return;

    qDebug() << "recorder start";

    m_audio_device.close();

    m_duration = 0;
    emit duration_changed();

    m_audio_device.setFileName(m_wav_file_path);
    if (m_audio_device.exists()) m_audio_device.remove();
    m_audio_device.open(QIODevice::OpenModeFlag::ReadWrite);
    m_audio_device.seek(sizeof(wav_header));
    m_audio_input->start(&m_audio_device);
}

void recorder::denoise(int sample_rate) {
    auto clean_ref_voice = settings::instance()->clean_ref_voice();

    int flags = denoiser::task_flags::task_probs;
    if (clean_ref_voice) {
        flags |= (denoiser::task_flags::task_normalize_two_pass |
                  denoiser::task_flags::task_denoise);
    }

    auto start = std::chrono::steady_clock::now();

    denoiser dn{
        sample_rate, flags,
        static_cast<uint64_t>(m_audio_device.size() - sizeof(wav_header))};

    std::array<char, denoiser::frame_size * 100> buff;

    m_audio_device.seek(sizeof(wav_header));

    while (!m_cancel_requested) {
        auto size = m_audio_device.read(buff.data(), buff.size());
        if (size <= 0) break;

        dn.process_char(buff.data(), size);

        if (clean_ref_voice) {
            m_audio_device.seek(m_audio_device.pos() - size);
            m_audio_device.write(buff.data(), size);
        }

        if (static_cast<size_t>(size) < buff.size()) break;
    }

    if (clean_ref_voice) {
        m_audio_device.seek(sizeof(wav_header));

        while (!m_cancel_requested) {
            auto size = m_audio_device.read(buff.data(), buff.size());
            if (size <= 0) break;

            dn.normalize_second_pass_char(buff.data(), size);

            m_audio_device.seek(m_audio_device.pos() - size);
            m_audio_device.write(buff.data(), size);

            if (static_cast<size_t>(size) < buff.size()) break;
        }
    }

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start)
                   .count();

    qDebug() << "processing dur:" << dur;

    m_probs = dn.speech_probs();

    m_audio_device.seek(sizeof(wav_header));
}

void recorder::stop() {
    if (!m_audio_input) return;

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

void recorder::process_from_mic() {
    denoise(m_sample_rate);

    m_audio_device.seek(0);

    write_wav_header(m_sample_rate, 2, m_num_channels,
                     (m_audio_device.size() - sizeof(wav_header)) / 2,
                     m_audio_device);
}

void recorder::process_from_input_file() {
    try {
        media_compressor::stream_t stream;
        stream.media_type = media_compressor::media_type_t::audio;

        media_compressor::options_t opts{
            media_compressor::quality_t::vbr_medium,
            media_compressor::flag_force_mono_output,
            1.0,
            std::move(stream),
            {}};

        media_compressor{}.decompress_to_file({m_input_file_path.toStdString()},
                                              m_wav_file_path.toStdString(),
                                              opts);
    } catch (const std::runtime_error& error) {
        qCritical() << "cannot decompress file:" << error.what();
        return;
    }

    m_audio_device.setFileName(m_wav_file_path);

    if (!m_audio_device.open(QIODevice::OpenModeFlag::ReadWrite)) {
        qCritical() << "cannot open file:" << m_wav_file_path;
        return;
    }

    wav_header header{};

    if (static_cast<unsigned long>(m_audio_device.read(
            reinterpret_cast<char*>(&header), sizeof(wav_header))) <
        sizeof(wav_header)) {
        qCritical() << "invalid wav header size";
        return;
    }

    denoise(header.sample_rate);
}

void recorder::process() {
    if (m_processing_thread.joinable()) m_processing_thread.join();

    if (m_audio_input) m_audio_input->reset();

    m_cancel_requested = false;

    m_processing_thread = std::thread{[this] {
        qDebug() << "recorder processing started";

        if (m_input_file_path.isEmpty())
            process_from_mic();
        else
            process_from_input_file();

        m_audio_device.close();

        m_duration = 0;
        emit duration_changed();

        m_processing = false;
        emit processing_changed();

        qDebug() << "recorder processing ended";
    }};

    m_processing = true;
    emit processing_changed();
}

bool recorder::recording() const {
    return m_audio_input &&
           m_audio_input->state() == QAudio::State::ActiveState;
}

bool recorder::processing() const { return m_processing; }

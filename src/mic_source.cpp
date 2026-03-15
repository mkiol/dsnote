/* Copyright (C) 2021-2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mic_source.h"

#include <QAudioFormat>
#include <QDebug>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QMediaDevices>
#else
#include <QAudioDeviceInfo>
#endif

mic_source::mic_source(const QString& preferred_audio_input, QObject* parent)
    : audio_source{parent} {
    qDebug() << "mic source created";
    init_audio(preferred_audio_input);
    start();
}

mic_source::~mic_source() {
    qDebug() << "mic source dtor";

    m_timer.stop();
    disconnect(&m_timer, &QTimer::timeout, this,
               &mic_source::handle_read_timeout);

    if (m_audio_source) {
        disconnect(m_audio_source.get(), &AudioSourceType::stateChanged, this,
                   &mic_source::handle_state_changed);
        m_audio_source->stop();
    }

    m_stopped = true;
    m_audio_device = nullptr;
}

bool mic_source::ok() const { return static_cast<bool>(m_audio_source); }

void mic_source::stop() {
    qDebug() << "mic source stop";

    if (m_stopped) return;  // Already stopped, prevent double-stop

    m_stopped = true;
    m_eof = true;  // Mark as EOF so next read will signal end

    // Disconnect the state changed signal before stopping to prevent callbacks
    // during stop
    if (m_audio_source) {
        disconnect(m_audio_source.get(), &AudioSourceType::stateChanged, this,
                   &mic_source::handle_state_changed);
        m_audio_source->stop();
        m_audio_device = nullptr;  // Invalidate the device pointer as it's
                                   // owned by QAudioSource
    }

    // Don't stop the timer here - let it continue to trigger one more read with
    // EOF The timer will be stopped in handle_read_timeout() when m_ended is
    // set
}

void mic_source::slowdown() {
    // do notning
}

void mic_source::speedup() {
    // do notning
}

static QAudioFormat audio_format() {
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    format.setSampleSize(16);
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
#else
    format.setSampleFormat(QAudioFormat::Int16);
#endif

    return format;
}

static bool has_audio_input(const QString& name) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    return std::find_if(devices.cbegin(), devices.cend(),
                        [&name](const auto& device) {
                            return device.deviceName() == name;
                        }) != devices.cend();
#else
    auto devices = QMediaDevices::audioInputs();
    return std::find_if(devices.cbegin(), devices.cend(),
                        [&name](const auto& device) {
                            return device.description() == name;
                        }) != devices.cend();
#endif
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static QAudioDeviceInfo audio_input_device(const QString& name) {
    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    auto it = std::find_if(
        devices.cbegin(), devices.cend(),
        [&name](const auto& device) { return device.deviceName() == name; });
    return it != devices.cend() ? *it : QAudioDeviceInfo{};
}
#else
static QAudioDevice audio_input_device(const QString& name) {
    auto devices = QMediaDevices::audioInputs();
    auto it = std::find_if(
        devices.cbegin(), devices.cend(),
        [&name](const auto& device) { return device.description() == name; });
    return it != devices.cend() ? *it : QAudioDevice{};
}
#endif

QStringList mic_source::audio_inputs() {
    QStringList list;

    auto format = audio_format();

    qDebug() << "supported audio input devices:";
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (const auto& device : devices) {
        if (device.isFormatSupported(format)) {
            qDebug() << device.deviceName();
            list.push_back(device.deviceName());
        }
    }
#else
    auto devices = QMediaDevices::audioInputs();
    for (const auto& device : devices) {
        if (device.isFormatSupported(format)) {
            qDebug() << device.description();
            list.push_back(device.description());
        }
    }
#endif

    if (list.isEmpty()) qWarning() << "no supported audio input device";

    return list;
}

void mic_source::init_audio(const QString& preferred_audio_input) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using DeviceInfoType = QAudioDeviceInfo;
    DeviceInfoType defaultDevice = QAudioDeviceInfo::defaultInputDevice();
    auto get_device_name = [](DeviceInfoType device) {
        return device.deviceName();
    };
#else
    using DeviceInfoType = QAudioDevice;
    DeviceInfoType defaultDevice = QMediaDevices::defaultAudioInput();
    auto get_device_name = [](DeviceInfoType device) {
        return device.description();
    };
#endif

    auto format = audio_format();

    DeviceInfoType device;
    auto input_name{preferred_audio_input};

    if (preferred_audio_input.isEmpty() ||
        !has_audio_input(preferred_audio_input)) {
        device = defaultDevice;
        if (device.isNull()) {
            qWarning() << "no audio input";
            throw std::runtime_error("no audio input");
        }

        input_name = get_device_name(device);
    } else {
        device = audio_input_device(input_name);
    }

    if (!device.isFormatSupported(format)) {
        qWarning() << "format not supported for audio input:"
                   << get_device_name(device);
        throw std::runtime_error("audio format is not supported");
    }

    qDebug() << "using audio input:" << get_device_name(device)
             << "(preferred was " << preferred_audio_input << ")";
    m_audio_source = std::make_unique<AudioSourceType>(device, format);

    connect(m_audio_source.get(), &AudioSourceType::stateChanged, this,
            &mic_source::handle_state_changed);
}

void mic_source::start() {
    m_audio_device = m_audio_source->start();

    m_timer.setInterval(200);  // 200 ms
    connect(&m_timer, &QTimer::timeout, this, &mic_source::handle_read_timeout);
    m_timer.start();
}

void mic_source::handle_state_changed(QAudio::State new_state) {
    qDebug() << "audio state:" << new_state;

    if (new_state == QAudio::State::StoppedState ||
        new_state == QAudio::State::SuspendedState || m_stopped) {
        qDebug() << "audio ended";
        if (m_audio_source && m_audio_source->error() == QAudio::NoError)
            m_eof = true;
    }
}

void mic_source::handle_read_timeout() {
    if (!m_audio_source) {
        m_timer.stop();
        return;
    }

    if (m_audio_source->error() != QAudio::NoError) {
        qWarning() << "audio input error:" << m_audio_source->error();
        m_timer.stop();
        emit error();
        return;
    }

    /*bool bytes_available = !m_eof || m_audio_source->bufferSize() > 0;
    qDebug() << "mic read timeout: b_avai=" << bytes_available
             << "eof=" << m_eof << "ended=" << m_ended << "sof=" << m_sof
             << "b_ready=" << m_audio_source->bufferSize();*/

    if (m_ended) {
        emit ended();
        m_timer.stop();
        return;
    }

    emit audio_available();
}

void mic_source::clear() {
    qDebug() << "mic clear";

    if (!m_audio_device) return;

    char buff[std::numeric_limits<short>::max()];
    while (m_audio_device->read(buff, std::numeric_limits<short>::max()))
        continue;
}

audio_source::audio_data mic_source::read_audio(char* buf, size_t max_size) {
    audio_data data;
    data.data = buf;
    data.sof = m_sof;

    if (!m_audio_device) {
        data.eof = true;
        m_ended = true;
        return data;
    }

    bool bytes_available = !m_eof || m_audio_device->bytesAvailable() > 0;

    /*qDebug() << "read_audio: b_avai=" << bytes_available << "eof=" << m_eof
             << "ended=" << m_ended << "sof=" << m_sof
             << "b_ready=" << m_audio_device->bytesAvailable();*/

    if (!bytes_available) {
        data.eof = m_eof;
        if (data.eof) m_ended = true;
        return data;
    }

    data.size = m_audio_device->read(buf, max_size);
    data.eof = m_eof && !bytes_available;

    m_sof = false;

    if (data.eof) m_ended = true;

    return data;
}

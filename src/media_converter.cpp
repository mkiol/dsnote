/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "media_converter.hpp"

#include <QDebug>

QDebug operator<<(QDebug d, media_converter::state_t state) {
    switch (state) {
        case media_converter::state_t::idle:
            d << "idle";
            break;
        case media_converter::state_t::cancelling:
            d << "cancelling";
            break;
        case media_converter::state_t::error:
            d << "error";
            break;
        case media_converter::state_t::importing_subtitles:
            d << "importing-subtitles";
            break;
        case media_converter::state_t::exporting_to_subtitles:
            d << "exporting-to-subtitles";
            break;
        case media_converter::state_t::exporting_to_audio:
            d << "exporting-to-audio";
            break;
        case media_converter::state_t::exporting_to_audio_mix:
            d << "exporting-to-audio-mix";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, media_converter::task_t task) {
    switch (task) {
        case media_converter::task_t::none:
            d << "none";
            break;
        case media_converter::task_t::import_subtitles_async:
            d << "import-subtitles-async";
            break;
        case media_converter::task_t::export_to_subtitles_async:
            d << "export-to-subtitles-async";
            break;
        case media_converter::task_t::export_to_audio_async:
            d << "export-to-audio-async";
            break;
        case media_converter::task_t::export_to_audio_mix_async:
            d << "export-to-audio-mix-async";
            break;
    }

    return d;
}

void media_converter::clear() {
    m_data.clear();
    m_state = state_t::idle;
    m_task = task_t::none;
    m_cancelled = false;

    if (m_progress > 0.0) {
        m_progress = 0.0;
        emit progress_changed();
    }
}

void media_converter::cancel() {
    if (!m_mc || m_state == state_t::idle || m_state == state_t::cancelling)
        return;

    set_state(state_t::cancelling);
    m_mc->cancel();
}

bool media_converter::export_to_subtitles_async(
    const QString& input_file_path, const QString& output_file_path,
    media_compressor::format_t format) {
    qDebug() << "media converter task:" << task_t::export_to_subtitles_async;
    clear();
    m_task = task_t::export_to_subtitles_async;

    try {
        m_mc = std::make_unique<media_compressor>();

        media_compressor::options_t opts{
            media_compressor::quality_t::vbr_medium,
            media_compressor::flag_none,
            media_compressor::stream_t{
                -1, media_compressor::media_type_t::subtitles, {}, {}, 0},
            {}};

        m_mc->compress_to_file_async(
            {input_file_path.toStdString()}, output_file_path.toStdString(),
            format, opts, {}, [&]() { emit processing_done(); });
    } catch (const std::runtime_error& err) {
        qCritical() << "media converter error:" << err.what();
        m_mc.reset();
        m_task = task_t::none;
        return false;
    }

    set_state(state_t::exporting_to_subtitles);

    return true;
}

bool media_converter::import_subtitles_async(const QString& file_path,
                                              int stream_index) {
    qDebug() << "media converter task:" << task_t::import_subtitles_async;
    clear();
    m_task = task_t::import_subtitles_async;

    try {
        m_mc = std::make_unique<media_compressor>();

        media_compressor::options_t opts{
            media_compressor::quality_t::vbr_medium,
            media_compressor::flag_none,
            media_compressor::stream_t{
                stream_index,
                media_compressor::media_type_t::subtitles,
                {},
                {},
                0},
            {}};

        m_mc->decompress_to_data_format_async(
            {file_path.toStdString()}, opts, [&]() { emit data_received(); },
            [this]() { emit processing_done(); });
    } catch (const std::runtime_error& err) {
        qCritical() << "media converter error:" << err.what();
        m_mc.reset();
        m_task = task_t::none;
        return false;
    }

    set_state(state_t::importing_subtitles);

    return true;
}

bool media_converter::export_to_audio_async(
    const QStringList& input_file_paths, const QString& output_file_path,
    media_compressor::format_t format, media_compressor::quality_t quality) {
    qDebug() << "media converter task:" << task_t::export_to_audio_async;
    clear();
    m_task = task_t::export_to_audio_async;

    try {
        m_mc = std::make_unique<media_compressor>();

        media_compressor::options_t opts{
            quality, media_compressor::flag_none, {}, {}};

        std::vector<std::string> input_files;
        std::transform(input_file_paths.cbegin(), input_file_paths.cend(),
                       std::back_inserter(input_files),
                       [](const auto& file) { return file.toStdString(); });

        m_mc->compress_to_file_async(
            input_files, output_file_path.toStdString(), format,
            std::move(opts),
            [this](auto done, auto total) {
                emit file_progress_updated(done, total);
            },
            [this]() { emit processing_done(); });
    } catch (const std::runtime_error& err) {
        qCritical() << "media converter error:" << err.what();
        m_mc.reset();
        m_task = task_t::none;
        return false;
    }

    set_state(state_t::exporting_to_audio);

    return true;
}

bool media_converter::export_to_audio_mix_async(
    const QString& input_main_file_path, int main_stream_index,
    const QStringList& input_file_paths, const QString& output_file_path,
    media_compressor::format_t format, media_compressor::quality_t quality,
    int volume_change) {
    qDebug() << "media converter task:" << task_t::export_to_audio_mix_async;
    clear();
    m_task = task_t::export_to_audio_mix_async;

    try {
        m_mc = std::make_unique<media_compressor>();

        media_compressor::options_t opts{
            quality,
            media_compressor::flag_none,
            media_compressor::stream_t{main_stream_index,
                                       media_compressor::media_type_t::audio,
                                       {},
                                       {},
                                       volume_change},
            {}};

        std::vector<std::string> input_files;
        std::transform(input_file_paths.cbegin(), input_file_paths.cend(),
                       std::back_inserter(input_files),
                       [](const auto& file) { return file.toStdString(); });

        m_mc->compress_mix_to_file_async(
            input_main_file_path.toStdString(), input_files,
            output_file_path.toStdString(), format, opts,
            [this](auto done, auto total) {
                emit file_progress_updated(done, total);
            },
            [this]() { emit processing_done(); });
    } catch (const std::runtime_error& err) {
        qCritical() << "media converter error:" << err.what();
        m_mc.reset();
        m_task = task_t::none;
        return false;
    }

    set_state(state_t::exporting_to_audio_mix);

    return true;
}

media_converter::media_converter(QObject* parent) : QObject{parent} {
    connect(this, &media_converter::data_received, this,
            &media_converter::handle_data, Qt::QueuedConnection);
    connect(this, &media_converter::processing_done, this,
            &media_converter::handle_processing_done, Qt::QueuedConnection);
    connect(this, &media_converter::file_progress_updated, this,
            &media_converter::handle_file_progress_updated,
            Qt::QueuedConnection);
}

void media_converter::set_state(state_t new_state) {
    if (m_state != new_state) {
        qDebug() << "media converter state:" << m_state << "=>" << new_state;

        if (m_state == state_t::cancelling && new_state == state_t::idle)
            m_cancelled = true;

        m_state = new_state;
        emit state_changed();
    }
}

void media_converter::set_progress(double new_progress) {
    if (m_progress != new_progress) {
        qDebug() << "media converter progress:" << new_progress;
        m_progress = new_progress;
        emit progress_changed();
    }
}

void media_converter::handle_processing_done() {
    bool is_error = m_mc ? m_mc->error() : false;
    m_mc.reset();
    set_state(is_error ? state_t::error : state_t::idle);
}

void media_converter::handle_file_progress_updated(unsigned int done,
                                                   unsigned int total) {
    if (total > 0) set_progress(static_cast<double>(done) / total);
}

void media_converter::handle_data() {
    if (!m_mc) return;

    auto result = m_mc->get_all_data();
    m_data.append(result.second);

    if (result.first.total > 0)
        set_progress(static_cast<double>(result.first.bytes_read) /
                     result.first.total);
}

QString media_converter::string_data() const {
    return QString::fromStdString(m_data);
}

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
        case media_converter::state_t::importing_subtitles:
            d << "importing-subtitles";
            break;
        case media_converter::state_t::exporting_to_subtitles:
            d << "exporting-to-subtitles";
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
    }

    return d;
}

void media_converter::clear() {
    m_data.clear();
    m_progress = 0.0;
}

bool media_converter::export_to_subtitles_async(
    const QString& input_file_path, const QString& output_file_path,
    media_compressor::format_t format) {
    qDebug() << "media converter task:" << task_t::export_to_subtitles_async;
    m_task = task_t::export_to_subtitles_async;
    m_data.clear();
    m_progress = 0.0;

    try {
        m_mc = std::make_unique<media_compressor>();
        m_mc->compress_to_file_async(
            {input_file_path.toStdString()}, output_file_path.toStdString(),
            format,
            {media_compressor::quality_t::vbr_medium, false, false,
             media_compressor::stream_t{
                 -1, media_compressor::media_type_t::subtitles, {}, {}}},
            [&]() { emit processing_done(); });
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
    m_task = task_t::import_subtitles_async;
    m_data.clear();
    m_progress = 0.0;

    try {
        m_mc = std::make_unique<media_compressor>();
        m_mc->decompress_to_data_format_async(
            {file_path.toStdString()},
            {media_compressor::quality_t::vbr_medium, false, false,
             media_compressor::stream_t{
                 stream_index,
                 media_compressor::media_type_t::subtitles,
                 {},
                 {}}},
            [&]() { emit data_received(); }, [&]() { emit processing_done(); });
    } catch (const std::runtime_error& err) {
        qCritical() << "media converter error:" << err.what();
        m_mc.reset();
        m_task = task_t::none;
        return false;
    }

    set_state(state_t::importing_subtitles);

    return true;
}

media_converter::media_converter() : QObject{} {
    connect(this, &media_converter::data_received, this,
            &media_converter::handle_data, Qt::QueuedConnection);
    connect(this, &media_converter::processing_done, this,
            &media_converter::handle_processing_done, Qt::QueuedConnection);
}

void media_converter::set_state(state_t new_state) {
    if (m_state != new_state) {
        qDebug() << "media converter state:" << m_state << "=>" << new_state;
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
    m_mc.reset();
    set_state(state_t::idle);
}

void media_converter::handle_data() {
    if (!m_mc) return;

    auto result = m_mc->get_all_data();
    m_data.append(result.second);

    set_progress(static_cast<double>(result.first.bytes_read) /
                 result.first.total);
}

QString media_converter::string_data() const {
    return QString::fromStdString(m_data);
}

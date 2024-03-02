/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MEDIA_CONVERTER_H
#define MEDIA_CONVERTER_H

#include <QObject>
#include <QString>
#include <memory>
#include <string>

#include "media_compressor.hpp"

class media_converter : public QObject {
    Q_OBJECT
   public:
    enum class state_t {
        idle,
        cancelling,
        error,
        importing_subtitles,
        exporting_to_subtitles,
        exporting_to_audio,
        exporting_to_audio_mix
    };
    friend QDebug operator<<(QDebug d, state_t state);

    enum class task_t {
        none,
        import_subtitles_async,
        export_to_subtitles_async,
        export_to_audio_async,
        export_to_audio_mix_async
    };
    friend QDebug operator<<(QDebug d, task_t task);

    media_converter(QObject* parent = nullptr);

    inline auto state() const { return m_state; }
    inline auto task() const { return m_task; }
    inline auto progress() const { return m_progress; }
    inline auto cancelled() const { return m_cancelled; }
    QString string_data() const;

    bool import_subtitles_async(const QString& file_path, int stream_index);
    bool export_to_subtitles_async(const QString& input_file_path,
                                   const QString& output_file_path,
                                   media_compressor::format_t format);
    bool export_to_audio_mix_async(const QString& input_main_file_path,
                                   int main_stream_index,
                                   const QStringList& input_file_paths,
                                   const QString& output_file_path,
                                   media_compressor::format_t format,
                                   media_compressor::quality_t quality,
                                   int volume_change);
    bool export_to_audio_async(const QStringList& input_file_paths,
                               const QString& output_file_path,
                               media_compressor::format_t format,
                               media_compressor::quality_t quality);
    void clear();
    void cancel();

   signals:
    void progress_changed();
    void state_changed();

   private:
    std::unique_ptr<media_compressor> m_mc;
    std::string m_data;
    state_t m_state = state_t::idle;
    task_t m_task = task_t::none;
    double m_progress = 0.0;
    bool m_cancelled = false;

    Q_SIGNAL void data_received();
    Q_SIGNAL void processing_done();
    Q_SIGNAL void file_progress_updated(unsigned int done, unsigned int total);

    void set_state(state_t new_state);
    void set_progress(double new_progress);
    void handle_processing_done();
    void handle_file_progress_updated(unsigned int done, unsigned int total);
    void handle_data();
};

#endif  // MEDIA_CONVERTER_H

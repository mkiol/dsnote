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
    enum class state_t { idle, importing_subtitles };
    friend QDebug operator<<(QDebug d, state_t state);

    enum class task_t { none, import_subtitles_async };
    friend QDebug operator<<(QDebug d, task_t task);

    media_converter();

    inline auto state() const { return m_state; }
    inline auto task() const { return m_task; }
    inline auto progress() const { return m_progress; }
    QString string_data() const;

    bool import_subtitles_async(const QString& file_path, int stream_index);
    void clear();

   signals:
    void progress_changed();
    void state_changed();

   private:
    std::unique_ptr<media_compressor> m_mc;
    std::string m_data;
    state_t m_state = state_t::idle;
    task_t m_task = task_t::none;
    double m_progress = 0.0;

    Q_SIGNAL void data_received();
    Q_SIGNAL void processing_done();

    void set_state(state_t new_state);
    void set_progress(double new_progress);
    void handle_processing_done();
    void handle_data();
};

#endif  // MEDIA_CONVERTER_H

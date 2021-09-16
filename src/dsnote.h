#ifndef SKRAPER_H
#define SKRAPER_H

/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QObject>
#include <QVariantList>
#include <memory>
#include <optional>
#include <utility>
#include <string>

#include "models_manager.h"
#include "deepspeech_wrapper.h"
#include "audio_source.h"

class dsnote_app : public QObject
{
    Q_OBJECT
    Q_PROPERTY (QString intermediate_text READ intermediate_text NOTIFY intermediate_text_changed)
    Q_PROPERTY (int active_lang_idx READ active_lang_idx NOTIFY langs_changed)
    Q_PROPERTY (QString active_lang READ active_lang NOTIFY langs_changed)
    Q_PROPERTY (QVariantList langs READ langs NOTIFY langs_changed)
    Q_PROPERTY (QVariantList available_langs READ available_langs NOTIFY langs_changed)
    Q_PROPERTY (bool speech READ speech WRITE set_speech NOTIFY speech_changed)
    Q_PROPERTY (bool busy READ busy NOTIFY busy_changed)
    Q_PROPERTY (double progress READ progress NOTIFY progress_changed)
    Q_PROPERTY (source_type audio_source_type READ audio_source_type NOTIFY audio_source_type_changed)

public:
    enum source_type { SourceNone, SourceMic, SourceFile };
    Q_ENUM(source_type)
    enum error_type { ErrorGeneric, ErrorMicSource, ErrorFileSource };
    Q_ENUM(error_type)

    dsnote_app();
    Q_INVOKABLE void download_lang(const QString& id);
    Q_INVOKABLE void delete_lang(const QString& id);
    Q_INVOKABLE void set_active_lang_idx(int idx);
    Q_INVOKABLE void set_file_source(const QString& source_file);
    Q_INVOKABLE void set_file_source(const QUrl& source_file);
    Q_INVOKABLE void cancel_file_source();

signals:
    void active_lang_changed();
    void langs_changed();
    void lang_download_progress(const QString& id, double progress);
    void intermediate_text_changed();
    void text_changed();
    void speech_changed();
    void speech_clear();
    void busy_changed();
    void buff_ready();
    void progress_changed();
    void audio_source_type_changed();
    void error(error_type type);
    void transcribe_done();

private:
    enum class speech_mode_req_type { settings, automatic, manual };

    std::unique_ptr<deepspeech_wrapper> ds;
    std::unique_ptr<audio_source> source;
    models_manager manager;
    int active_lang_idx_value = -1;
    QString active_lang_value;
    QVariantList available_langs_list;
    QString intermediate_text_value;
    double progress_value = -1;

    QVariantList langs() const;
    QVariantList available_langs() const;
    void handle_models_changed();
    void handle_text_decoded(const std::string &text);
    void handle_intermediate_text_decoded(const std::string &text);
    void handle_audio_available();
    void handle_speech_status_changed(bool speech_detected);
    void handle_processing_changed(bool processing);
    void handle_speech_clear();
    void handle_audio_error();
    void handle_audio_ended();
    void restart_ds(speech_mode_req_type speech_mode_req);
    void restart_ds(const QString& model_file, const QString& scorer_file = {},
                    speech_mode_req_type speech_mode_req = speech_mode_req_type::settings);
    void restart_audio_source(const QString& source_file = {});
    void stop();
    [[nodiscard]] inline bool busy() const { return manager.busy(); }
    [[nodiscard]] bool speech() const;
    [[nodiscard]] inline double progress() const { return progress_value; }
    [[nodiscard]] source_type audio_source_type() const;
    void set_progress(double progress);
    std::optional<std::pair<QString, QString>> setup_active_lang();
    inline bool recording() const { return source ? true : false; };
    inline int active_lang_idx() const { return active_lang_idx_value; }
    inline QString active_lang() const { return active_lang_value; }
    inline QString intermediate_text() const { return intermediate_text_value; }
    inline void set_speech(bool value) { if (ds && ds->ok()) ds->set_speech_status(value); }
};

#endif // SKRAPER_H

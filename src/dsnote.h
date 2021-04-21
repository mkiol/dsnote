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
#include "mic_source.h"

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

public:
    dsnote_app();
    Q_INVOKABLE void download_lang(const QString& id);
    Q_INVOKABLE void delete_lang(const QString& id);
    Q_INVOKABLE void set_active_lang_idx(int idx);

signals:
    void active_lang_changed();
    void langs_changed();
    void lang_download_progress(const QString& id, double progress);
    void intermediate_text_changed();
    void speech_changed();
    void busy_changed();

private:
    std::unique_ptr<deepspeech_wrapper> ds;
    std::unique_ptr<mic_source> mic;
    models_manager manager;
    int active_lang_idx_value = -1;
    QString active_lang_value;
    QVariantList available_langs_list;
    QString intermediate_text_value;

    QVariantList langs() const;
    QVariantList available_langs() const;
    void handle_models_changed();
    void handle_text_decoded(const std::string &text);
    void handle_intermediate_text_decoded(const std::string &text);
    void handle_audio_available();
    void handle_speech_status_changed(bool speech_detected);
    void restart_ds(const QString& model_file, const QString& scorer_file = {});
    void restart_recording();
    void stop();
    [[nodiscard]] inline bool busy() const { return manager.busy(); }
    [[nodiscard]] bool speech() const;
    std::optional<std::pair<QString, QString>> setup_active_lang();
    inline bool recording() const { return mic ? true : false; };
    inline int active_lang_idx() const { return active_lang_idx_value; }
    inline QString active_lang() const { return active_lang_value; }
    inline QString intermediate_text() const { return intermediate_text_value; }
    inline void set_speech(bool value) { if (ds && ds->ok()) ds->set_speech_status(value); }
};

#endif // SKRAPER_H

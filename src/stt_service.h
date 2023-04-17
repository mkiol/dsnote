/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef STT_SERVICE_H
#define STT_SERVICE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "audio_source.h"
#include "dbus_stt_adaptor.h"
#include "engine_wrapper.hpp"
#include "models_manager.h"
#include "singleton.h"

class stt_service : public QObject, public singleton<stt_service> {
    Q_OBJECT

    // DBus
    Q_PROPERTY(int State READ dbus_state CONSTANT)
    Q_PROPERTY(int Speech READ speech CONSTANT)
    Q_PROPERTY(QVariantMap Langs READ available_langs CONSTANT)
    Q_PROPERTY(QString DefaultLang READ default_lang WRITE set_default_lang)
    Q_PROPERTY(QVariantMap Models READ available_models CONSTANT)
    Q_PROPERTY(QString DefaultModel READ default_model WRITE set_default_model)
    Q_PROPERTY(int CurrentTask READ current_task_id CONSTANT)
    Q_PROPERTY(QVariantMap Translations READ translations CONSTANT)

   public:
    enum class state_t {
        unknown = 0,
        not_configured = 1,
        busy = 2,
        idle = 3,
        listening_manual = 4,
        listening_auto = 5,
        transcribing_file = 6,
        listening_single_sentence = 7
    };

    enum class speech_mode_t { automatic = 0, manual = 1, single_sentence = 2 };

    enum class source_t { none = 0, mic = 1, file = 2 };

    enum class error_t { generic = 0, mic_source = 1, file_source = 2 };

    explicit stt_service(QObject *parent = nullptr);

    Q_INVOKABLE void download_model(const QString &id);
    Q_INVOKABLE void delete_model(const QString &id);

    Q_INVOKABLE int start_listen(stt_service::speech_mode_t mode,
                                 const QString &lang, bool translate);
    Q_INVOKABLE int stop_listen(int task);
    Q_INVOKABLE int transcribe_file(const QString &file, const QString &lang,
                                    bool translate);
    Q_INVOKABLE int cancel(int task);

    QString default_model() const;
    QString default_lang() const;
    void set_default_model(const QString &model_id) const;
    void set_default_lang(const QString &lang_id) const;
    QVariantMap available_models() const;
    QVariantMap available_langs() const;
    int speech() const;
    state_t state() const;
    int current_task_id() const;
    double transcribe_file_progress(int task) const;

   signals:
    void models_changed();
    void model_download_progress(const QString &id, double progress);
    void speech_changed();
    void state_changed();
    void buff_ready();
    void transcribe_file_progress_changed(double progress, int task);
    void error(stt_service::error_t type);
    void file_transcribe_finished(int task);
    void intermediate_text_decoded(const QString &text, const QString &lang,
                                   int task);
    void text_decoded(const QString &text, const QString &lang, int task);
    void current_task_changed();
    void sentence_timeout(int task_id);
    void engine_eof(int task_id);
    void engine_error(int task_id);
    void engine_shutdown();
    void default_model_changed();
    void default_lang_changed();

    // DBus
    void ErrorOccured(int code);
    void FileTranscribeFinished(int task);
    void FileTranscribeProgress(double progress, int task);
    void IntermediateTextDecoded(const QString &text, const QString &lang,
                                 int task);
    void StatePropertyChanged(int state);
    void TextDecoded(const QString &text, const QString &lang, int task);
    void SpeechPropertyChanged(int speech);
    void LangsPropertyChanged(const QVariantMap &langs);
    void DefaultLangPropertyChanged(const QString &lang);
    void ModelsPropertyChanged(const QVariantMap &models);
    void DefaultModelPropertyChanged(const QString &model);
    void CurrentTaskPropertyChanged(int task);

   private:
    struct model_data_t {
        QString model_id;
        QString lang_id;
        models_manager::model_engine engine = models_manager::model_engine::stt_ds;
        QString name;
    };

    struct model_files_t {
        QString model_id;
        QString lang_id;
        models_manager::model_engine engine = models_manager::model_engine::stt_ds;
        QString model_file;
        QString scorer_file;
    };

    struct task_t {
        int id = INVALID_TASK;
        speech_mode_t speech_mode;
        QString model_id;
        bool translate = false;
    };

    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral("org.mkiol.Stt")};
    inline static const QString DBUS_SERVICE_PATH{QStringLiteral("/")};
    static const int SUCCESS = 0;
    static const int FAILURE = -1;
    static const int INVALID_TASK = -1;
    static const int DS_RESTART_TIME = 2000;           // 2s
    static const int KEEPALIVE_TIME = 60000;           // 60s
    static const int KEEPALIVE_TASK_TIME = 10000;      // 10s
    static const int SINGLE_SENTENCE_TIMEOUT = 10000;  // 10s

    int m_last_task_id = INVALID_TASK;
    std::unique_ptr<engine_wrapper> m_engine;
    std::unique_ptr<audio_source> m_source;
    std::map<QString, model_data_t> m_available_models_map;
    double m_progress = -1;
    state_t m_state = state_t::unknown;
    SttAdaptor m_stt_adaptor;
    QTimer m_keepalive_timer;
    QTimer m_keepalive_current_task_timer;
    int m_last_intermediate_text_task = INVALID_TASK;
    std::optional<task_t> m_previous_task;
    std::optional<task_t> m_current_task;
    std::optional<task_t> m_pending_task;

    void handle_models_changed();
    void handle_sentence_timeout();
    void handle_sentence_timeout(int task_id);
    void handle_engine_eof();
    void handle_engine_eof(int task_id);
    void handle_engine_error();
    void handle_engine_error(int task_id);
    void handle_text_decoded(const std::string &text);
    void handle_text_decoded(const QString &text, const QString &model_id,
                             int task_id);
    void handle_intermediate_text_decoded(const std::string &text);
    void handle_audio_available();
    void handle_speech_detection_status_changed(
        engine_wrapper::speech_detection_status_t status);
    void handle_processing_changed(bool processing);
    void handle_audio_error();
    void handle_audio_ended();
    QString restart_engine(speech_mode_t speech_mode, const QString &model_id,
                           bool translate);
    void restart_audio_source(const QString &source_file = {});
    void stop();
    source_t audio_source_type() const;
    void set_progress(double progress);
    std::optional<model_files_t> choose_model_files(QString model_id = {});
    inline auto recording() const { return static_cast<bool>(m_source); };
    void refresh_status();
    static QString state_str(state_t state_value);
    void stop_engine_gracefully();
    void stop_engine();
    QString test_default_model(const QString &lang) const;
    int next_task_id();
    void handle_keepalive_timeout();
    void handle_task_timeout();
    QVariantMap translations() const;
    static QString lang_from_model_id(const QString &model_id);
    int dbus_state() const;
    void start_keepalive_current_task();
    void stop_keepalive_current_task();

    // DBus
    Q_INVOKABLE int StartListen(int mode, const QString &lang, bool translate);
    Q_INVOKABLE int StopListen(int task);
    Q_INVOKABLE int Cancel(int task);
    Q_INVOKABLE int TranscribeFile(const QString &file, const QString &lang,
                                   bool translate);
    Q_INVOKABLE double GetFileTranscribeProgress(int task);
    Q_INVOKABLE int KeepAliveService();
    Q_INVOKABLE int KeepAliveTask(int task);
    Q_INVOKABLE int Reload();
};

#endif  // STT_SERVICE_H

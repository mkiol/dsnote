/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SPEECH_SERVICE_H
#define SPEECH_SERVICE_H

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <utility>

#include "audio_source.h"
#include "config.h"
#include "dbus_speech_adaptor.h"
#include "models_manager.h"
#include "singleton.h"
#include "stt_engine.hpp"
#include "tts_engine.hpp"

class speech_service : public QObject, public singleton<speech_service> {
    Q_OBJECT

    // Speech DBus API
    Q_PROPERTY(int State READ dbus_state CONSTANT)
    Q_PROPERTY(int Speech READ speech CONSTANT)
    Q_PROPERTY(QVariantMap SttLangs READ available_stt_langs CONSTANT)
    Q_PROPERTY(QVariantList SttLangList READ available_stt_lang_list CONSTANT)
    Q_PROPERTY(QVariantMap TtsLangs READ available_tts_langs CONSTANT)
    Q_PROPERTY(QVariantList TtsLangList READ available_tts_lang_list CONSTANT)
    Q_PROPERTY(QVariantMap TttLangs READ available_ttt_langs CONSTANT)
    Q_PROPERTY(
        QVariantList SttTtsLangList READ available_stt_tts_lang_list CONSTANT)
    Q_PROPERTY(
        QString DefaultSttLang READ default_stt_lang WRITE set_default_stt_lang)
    Q_PROPERTY(
        QString DefaultTtsLang READ default_tts_lang WRITE set_default_tts_lang)
    Q_PROPERTY(QVariantMap SttModels READ available_stt_models CONSTANT)
    Q_PROPERTY(QVariantMap TtsModels READ available_tts_models CONSTANT)
    Q_PROPERTY(QVariantMap TttModels READ available_ttt_models CONSTANT)
    Q_PROPERTY(QString DefaultSttModel READ default_stt_model WRITE
                   set_default_stt_model)
    Q_PROPERTY(QString DefaultTtsModel READ default_tts_model WRITE
                   set_default_tts_model)
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
        listening_single_sentence = 7,
        playing_speech = 8
    };
    friend QDebug operator<<(QDebug d, state_t state_value);

    enum class speech_mode_t { automatic = 0, manual = 1, single_sentence = 2 };

    enum class source_t { none = 0, mic = 1, file = 2 };

    enum class error_t { generic = 0, mic_source = 1, file_source = 2 };

    struct tts_partial_result_t {
        QString text;
        QString wav_file_path;
        bool last = false;
        int task_id = INVALID_TASK;
    };

    explicit speech_service(QObject *parent = nullptr);
    ~speech_service() override;

    Q_INVOKABLE void download_model(const QString &id);
    Q_INVOKABLE void delete_model(const QString &id);

    Q_INVOKABLE int stt_start_listen(speech_service::speech_mode_t mode,
                                     const QString &lang, bool translate);
    Q_INVOKABLE int stt_stop_listen(int task);
    Q_INVOKABLE int stt_transcribe_file(const QString &file,
                                        const QString &lang, bool translate);
    Q_INVOKABLE int tts_play_speech(const QString &text, const QString &lang);
    Q_INVOKABLE int tts_stop_speech(int task);
    Q_INVOKABLE int cancel(int task);

    QString default_stt_model() const;
    QString default_stt_lang() const;
    void set_default_stt_model(const QString &model_id) const;
    void set_default_stt_lang(const QString &lang_id) const;
    QVariantMap available_stt_models() const;
    QVariantMap available_stt_langs() const;
    QVariantList available_stt_lang_list() const;
    QString default_ttt_model() const;
    QString default_ttt_lang() const;
    QVariantMap available_ttt_models() const;
    QVariantMap available_ttt_langs() const;
    QString default_tts_model() const;
    QString default_tts_lang() const;
    void set_default_tts_model(const QString &model_id) const;
    void set_default_tts_lang(const QString &lang_id) const;
    QVariantMap available_tts_models() const;
    QVariantMap available_tts_langs() const;
    QVariantList available_tts_lang_list() const;
    QVariantList available_stt_tts_lang_list() const;
    inline auto speech() const { return m_speech_state; }
    state_t state() const;
    int current_task_id() const;
    double stt_transcribe_file_progress(int task) const;

   signals:
    void models_changed();
    void model_download_progress(const QString &id, double progress);
    void speech_changed();
    void state_changed();
    void buff_ready();
    void stt_transcribe_file_progress_changed(double progress, int task);
    void error(speech_service::error_t type);
    void stt_file_transcribe_finished(int task);
    void stt_intermediate_text_decoded(const QString &text, const QString &lang,
                                       int task);
    void stt_text_decoded(const QString &text, const QString &lang, int task);
    void tts_play_speech_finished(int task);
    void tts_speech_encoded(const tts_partial_result_t &result);
    void tts_partial_speech_playing(const QString &text, int task);
    void requet_update_speech_state();
    void current_task_changed();
    void sentence_timeout(int task_id);
    void stt_engine_eof(int task_id);
    void stt_engine_error(int task_id);
    void tts_engine_error(int task_id);
    void stt_engine_shutdown();
    void default_stt_model_changed();
    void default_stt_lang_changed();
    void default_tts_model_changed();
    void default_tts_lang_changed();

    // DBus
    void StatePropertyChanged(int state);
    void ErrorOccured(int code);
    void CurrentTaskPropertyChanged(int task);
    void SpeechPropertyChanged(int speech);
    void SttFileTranscribeFinished(int task);
    void SttFileTranscribeProgress(double progress, int task);
    void SttIntermediateTextDecoded(const QString &text, const QString &lang,
                                    int task);
    void SttTextDecoded(const QString &text, const QString &lang, int task);
    void TtsPlaySpeechFinished(int task);
    void TtsPartialSpeechPlaying(const QString &text, int task);
    void SttLangsPropertyChanged(const QVariantMap &langs);
    void SttLangListPropertyChanged(const QVariantList &langs);
    void DefaultSttLangPropertyChanged(const QString &lang);
    void SttModelsPropertyChanged(const QVariantMap &models);
    void DefaultSttModelPropertyChanged(const QString &model);
    void TtsLangsPropertyChanged(const QVariantMap &langs);
    void TtsLangListPropertyChanged(const QVariantList &langs);
    void SttTtsLangListPropertyChanged(const QVariantList &langs);
    void DefaultTtsLangPropertyChanged(const QString &lang);
    void TtsModelsPropertyChanged(const QVariantMap &models);
    void DefaultTtsModelPropertyChanged(const QString &model);
    void TttLangsPropertyChanged(const QVariantMap &langs);
    void TttModelsPropertyChanged(const QVariantMap &models);

   private:
    enum class engine_t { stt, tts };

    struct model_data_t {
        QString model_id;
        QString lang_id;
        models_manager::model_engine engine =
            models_manager::model_engine::stt_ds;
        QString name;
    };

    struct model_config_t {
        QString lang_id;
        QString model_id;
        models_manager::model_engine engine =
            models_manager::model_engine::stt_ds;
        QString model_file;
        QString scorer_file;
        QString vocoder_file;
        QString speaker;
        QString ttt_model_id;
        models_manager::model_engine ttt_engine =
            models_manager::model_engine::ttt_hftc;
        QString ttt_model_file;
    };

    struct task_t {
        int id = INVALID_TASK;
        engine_t engine = engine_t::stt;
        QString model_id;
        speech_mode_t speech_mode = speech_mode_t::single_sentence;
        bool translate = false;
    };

    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_SERVICE)};
    inline static const QString DBUS_SERVICE_PATH{QStringLiteral("/")};
    static const int SUCCESS = 0;
    static const int FAILURE = -1;
    static const int INVALID_TASK = -1;
    static const int DS_RESTART_TIME = 2000;           // 2s
    static const int KEEPALIVE_TIME = 60000;           // 60s
    static const int KEEPALIVE_TASK_TIME = 10000;      // 10s
    static const int SINGLE_SENTENCE_TIMEOUT = 10000;  // 10s

    int m_last_task_id = INVALID_TASK;
    std::unique_ptr<stt_engine> m_stt_engine;
    std::unique_ptr<tts_engine> m_tts_engine;
    std::unique_ptr<audio_source> m_source;
    std::map<QString, model_data_t> m_available_stt_models_map;
    std::map<QString, model_data_t> m_available_tts_models_map;
    std::map<QString, model_data_t> m_available_ttt_models_map;
    double m_progress = -1;
    state_t m_state = state_t::unknown;
    SpeechAdaptor m_dbus_service_adaptor;
    QTimer m_keepalive_timer;
    QTimer m_keepalive_current_task_timer;
    int m_last_intermediate_text_task = INVALID_TASK;
    std::optional<task_t> m_previous_task;
    std::optional<task_t> m_current_task;
    std::optional<task_t> m_pending_task;
    QMediaPlayer m_player;
    int m_speech_state = 0;
    std::queue<tts_partial_result_t> m_tts_queue;

    void handle_models_changed();
    void handle_tts_models_changed();
    void handle_stt_sentence_timeout();
    void handle_stt_sentence_timeout(int task_id);
    void handle_stt_engine_eof();
    void handle_stt_engine_eof(int task_id);
    void handle_stt_engine_error();
    void handle_stt_engine_error(int task_id);
    void handle_tts_engine_error();
    void handle_tts_engine_error(int task_id);
    void handle_tts_engine_state_changed(tts_engine::state_t state);
    void handle_stt_text_decoded(const std::string &text);
    void handle_stt_text_decoded(const QString &text, const QString &model_id,
                                 int task_id);
    void handle_stt_intermediate_text_decoded(const std::string &text);
    void handle_tts_speech_encoded(const std::string &text,
                                   const std::string &wav_file_path, bool last);
    void handle_tts_speech_encoded(const tts_partial_result_t &result);
    void handle_player_state_changed(QMediaPlayer::State new_state);
    void handle_audio_available();
    void handle_stt_speech_detection_status_changed(
        stt_engine::speech_detection_status_t status);
    void handle_processing_changed(bool processing);
    void handle_audio_error();
    void handle_audio_ended();
    QString restart_stt_engine(speech_mode_t speech_mode,
                               const QString &model_id, bool translate);
    QString restart_tts_engine(const QString &model_id);
    void restart_audio_source(const QString &source_file = {});
    void stop_stt();
    source_t audio_source_type() const;
    void set_progress(double progress);
    std::optional<model_config_t> choose_model_config(engine_t engine_type,
                                                      QString model_id = {});
    inline auto recording() const { return static_cast<bool>(m_source); };
    void refresh_status();
    void stop_stt_engine_gracefully();
    void stop_stt_engine();
    void stop_tts_engine();
    QString test_default_stt_model(const QString &lang) const;
    QString test_default_tts_model(const QString &lang) const;
    QString test_default_ttt_model(const QString &lang) const;
    int next_task_id();
    void handle_keepalive_timeout();
    void handle_task_timeout();
    QVariantMap translations() const;
    static QString lang_from_model_id(const QString &model_id);
    int dbus_state() const;
    void start_keepalive_current_task();
    void stop_keepalive_current_task();
    QVariantMap available_models(
        const std::map<QString, model_data_t> &available_models_map) const;
    QVariantMap available_langs(
        const std::map<QString, model_data_t> &available_models_map) const;
    QVariantList available_lang_list(
        const std::map<QString, model_data_t> &available_models_map) const;
    std::set<QString> available_lang_set(
        const std::map<QString, model_data_t> &available_models_map) const;
    static QString test_default_model(
        const QString &lang,
        const std::map<QString, model_data_t> &available_models_map);
    void set_state(state_t new_state);
    void update_speech_state();
    static void remove_cached_wavs();
    void handle_tts_queue();
    static void setup_modules();

    // DBus
    Q_INVOKABLE int Cancel(int task);
    Q_INVOKABLE int KeepAliveService();
    Q_INVOKABLE int KeepAliveTask(int task);
    Q_INVOKABLE int Reload();
    Q_INVOKABLE int SttStartListen(int mode, const QString &lang,
                                   bool translate);
    Q_INVOKABLE int SttStopListen(int task);
    Q_INVOKABLE int SttTranscribeFile(const QString &file, const QString &lang,
                                      bool translate);
    Q_INVOKABLE double SttGetFileTranscribeProgress(int task);
    Q_INVOKABLE int TtsPlaySpeech(const QString &text, const QString &lang);
    Q_INVOKABLE int TtsStopSpeech(int task);
};

Q_DECLARE_METATYPE(speech_service::tts_partial_result_t)

#endif  // SPEECH_SERVICE_H

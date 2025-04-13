/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SPEECH_SERVICE_H
#define SPEECH_SERVICE_H

#include <QDebug>
#include <QIODevice>
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
#include <variant>
#include <vector>

#include "audio_source.h"
#include "config.h"
#include "dbus_speech_adaptor.h"
#include "mnt_engine.hpp"
#include "models_manager.h"
#include "singleton.h"
#include "stt_engine.hpp"
#include "text_repair_engine.hpp"
#include "tts_engine.hpp"

QDebug operator<<(QDebug d, const stt_engine::config_t &config);
QDebug operator<<(QDebug d, const tts_engine::config_t &config);

class speech_service : public QObject, public singleton<speech_service> {
    Q_OBJECT

    // Speech DBus API
    Q_PROPERTY(int State READ dbus_state CONSTANT)
    Q_PROPERTY(int TaskState READ task_state CONSTANT)
    Q_PROPERTY(QVariantMap SttLangs READ available_stt_langs CONSTANT)
    Q_PROPERTY(QVariantList SttLangList READ available_stt_lang_list CONSTANT)
    Q_PROPERTY(QVariantMap TtsLangs READ available_tts_langs CONSTANT)
    Q_PROPERTY(QVariantList TtsLangList READ available_tts_lang_list CONSTANT)
    Q_PROPERTY(QVariantMap TttLangs READ available_ttt_langs CONSTANT)
    Q_PROPERTY(QVariantList MntLangList READ available_mnt_lang_list CONSTANT)
    Q_PROPERTY(QVariantMap MntLangs READ available_mnt_langs CONSTANT)
    Q_PROPERTY(
        QVariantList SttTtsLangList READ available_stt_tts_lang_list CONSTANT)
    Q_PROPERTY(
        QString DefaultSttLang READ default_stt_lang WRITE set_default_stt_lang)
    Q_PROPERTY(
        QString DefaultTtsLang READ default_tts_lang WRITE set_default_tts_lang)
    Q_PROPERTY(
        QString DefaultMntLang READ default_mnt_lang WRITE set_default_mnt_lang)
    Q_PROPERTY(QString DefaultMntOutLang READ default_mnt_out_lang WRITE
                   set_default_mnt_out_lang)
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
        playing_speech = 8,
        writing_speech_to_file = 9,
        translating = 10,
        repairing_text = 11
    };
    friend QDebug operator<<(QDebug d, state_t state_value);

    enum class speech_mode_t {
        automatic = 0,
        manual = 1,
        single_sentence = 2,
        play_speech = 3,
        speech_to_file = 4,
        translate = 5
    };

    enum class source_t { none = 0, mic = 1, file = 2 };

    enum class error_t : unsigned int {
        generic = 0,
        mic_source = 1,
        file_source = 2,
        stt_engine = 3,
        tts_engine = 4,
        mnt_engine = 5,
        mnt_runtime = 6,
        text_repair_engine = 7
    };

    struct tts_partial_result_t {
        QString text;
        QString audio_file_path;
        tts_engine::audio_format_t audio_format =
            tts_engine::audio_format_t::wav;
        bool remove_audio_file = false;
        double progress = 0.0;
        bool last = false;
        int task_id = INVALID_TASK;
    };

    explicit speech_service(QObject *parent = nullptr);
    ~speech_service() override;

    Q_INVOKABLE void download_model(const QString &id);
    Q_INVOKABLE void delete_model(const QString &id);

    Q_INVOKABLE int stt_start_listen(speech_service::speech_mode_t mode,
                                     QString lang, QString out_lang,
                                     const QVariantMap &options);
    Q_INVOKABLE int stt_stop_listen(int task);
    Q_INVOKABLE int stt_transcribe_file(const QString &file, QString lang,
                                        QString out_lang,
                                        const QVariantMap &options);
    Q_INVOKABLE int tts_play_speech(const QString &text, QString lang,
                                    const QVariantMap &options);
    Q_INVOKABLE int tts_pause_speech(int task);
    Q_INVOKABLE int tts_resume_speech(int task);
    Q_INVOKABLE int tts_stop_speech(int task);
    Q_INVOKABLE int tts_speech_to_file(const QString &text, QString lang,
                                       const QVariantMap &options);
    Q_INVOKABLE int ttt_repair_text(const QString &text,
                                    const QVariantMap &options);
    Q_INVOKABLE int mnt_translate(const QString &text, QString lang,
                                  QString out_lang, const QVariantMap &options);
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
    QString default_mnt_lang() const;
    QString default_mnt_out_lang() const;
    QVariantMap available_mnt_models() const;
    QVariantMap available_mnt_langs() const;
    void set_default_mnt_lang(const QString &lang_id) const;
    void set_default_mnt_out_lang(const QString &lang_id) const;
    QString default_tts_model() const;
    QString default_tts_lang() const;
    void set_default_tts_model(const QString &model_id) const;
    void set_default_tts_lang(const QString &lang_id) const;
    QVariantMap available_tts_models() const;
    QVariantMap available_tts_langs() const;
    QVariantList available_tts_lang_list() const;
    QVariantList available_stt_tts_lang_list() const;
    QVariantList available_mnt_lang_list() const;
    inline auto task_state() const { return m_task_state; }
    state_t state() const;
    int current_task_id() const;
    double stt_transcribe_file_progress(int task) const;
    double tts_speech_to_file_progress(int task) const;
    double mnt_translate_progress(int task) const;
    QVariantMap mnt_out_langs(QString in_lang) const;
    QVariantMap features_availability();
    static void remove_cached_files();

   signals:
    void models_changed();
    void model_download_progress(const QString &id, double progress);
    void task_state_changed();
    void state_changed();
    void buff_ready();
    void stt_transcribe_file_progress_changed(double progress, int task);
    void tts_speech_to_file_progress_changed(double progress, int task);
    void error(speech_service::error_t type);
    void stt_file_transcribe_finished(int task);
    void stt_intermediate_text_decoded(const QString &text, const QString &lang,
                                       int task);
    void stt_text_decoded(const QString &text, const QString &lang, int task);
    void tts_play_speech_finished(int task);
    void tts_speech_to_file_finished(QStringList files, int task);
    void tts_speech_encoded(const speech_service::tts_partial_result_t &result);
    void tts_partial_speech_playing(const QString &text, int task);
    void ttt_text_repaired(const QString &text, int task);
    void ttt_repair_text_finished(const QString &text, int task);
    void mnt_translate_progress_changed(double progress, int task);
    void mnt_engine_translate_progress_changed(int task);
    void mnt_translate_finished(const QString &in_text, const QString &in_lang,
                                const QString &out_text,
                                const QString &out_lang, int task);
    void requet_update_task_state();
    void mnt_engine_state_changed(mnt_engine::state_t state, int task_id);
    void tts_engine_state_changed(tts_engine::state_t state, int task_id);
    void text_repair_engine_state_changed(text_repair_engine::state_t state,
                                          int task_id);
    void current_task_changed();
    void sentence_timeout(int task_id);
    void stt_engine_eof(int task_id);
    void stt_engine_error(int task_id);
    void stt_engine_stopped(int task_id);
    void stt_engine_stopping(int task_id);
    void tts_engine_error(int task_id);
    void text_repair_engine_error(int task_id);
    void mnt_engine_error(mnt_engine::error_t error_type, int task_id);
    void stt_engine_shutdown();
    void stt_engine_state_changed(stt_engine::speech_detection_status_t state,
                                  int task_id);
    void default_stt_model_changed();
    void default_stt_lang_changed();
    void default_tts_model_changed();
    void default_tts_lang_changed();
    void default_mnt_lang_changed();
    void default_mnt_out_lang_changed();
    void features_availability_updated();

    // DBus
    void StatePropertyChanged(int state);
    void ErrorOccured(int code);
    void CurrentTaskPropertyChanged(int task);
    void TaskStatePropertyChanged(int speech);
    void SttFileTranscribeFinished(int task);
    void SttFileTranscribeProgress(double progress, int task);
    void SttIntermediateTextDecoded(const QString &text, const QString &lang,
                                    int task);
    void SttTextDecoded(const QString &text, const QString &lang, int task);
    void TtsPlaySpeechFinished(int task);
    void TtsSpeechToFileFinished(const QStringList &files, int task);
    void TtsPartialSpeechPlaying(const QString &text, int task);
    void TtsSpeechToFileProgress(double progress, int task);
    void TtrRepairTextFinished(const QString &text, int task);
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
    void MntLangsPropertyChanged(const QVariantMap &langs);
    void MntLangListPropertyChanged(const QVariantList &langs);
    void DefaultMntLangPropertyChanged(const QString &lang);
    void DefaultMntOutLangPropertyChanged(const QString &lang);
    void MntTranslateProgress(double progress, int task);
    void MntTranslateFinished(const QString &in_text, const QString &in_lang,
                              const QString &out_text, const QString &out_lang,
                              int task);
    void FeaturesAvailabilityUpdated();

   private:
    enum class engine_t { stt, tts, mnt, text_repair };

    struct lang_to_model_map_t {
        std::unordered_map<QString, QString> stt;
        std::unordered_map<QString, QString> tts;
        std::unordered_map<QString, QString> mnt;
        bool found_default_stt = false;
        bool found_default_tts = false;
        bool found_default_mnt = false;
    };

    struct model_data_t {
        QString model_id;
        QString lang_id;
        QString trg_lang_id;
        models_manager::model_engine_t engine =
            models_manager::model_engine_t::stt_ds;
        QString name;
        QString options;
    };

    struct mnt_model_config_t {
        QString lang_id;
        QString out_lang_id;
        QString model_id_first;
        QString model_file_first;
        QString model_id_second;
        QString model_file_second;
    };

    struct tts_model_config_t {
        QString lang_id;
        QString lang_code;
        QString model_id;
        models_manager::model_engine_t engine =
            models_manager::model_engine_t::tts_piper;
        QString model_file;
        QString vocoder_file;
        QString diacritizer_file;
        QString speaker;
    };

    struct ttt_model_config_t {
        QString model_id;
        QString model_file;
    };

    struct stt_model_config_t {
        QString lang_id;
        QString lang_code;
        QString model_id;
        models_manager::model_engine_t engine =
            models_manager::model_engine_t::stt_ds;
        QString model_file;
        QString scorer_file;
        QString openvino_file;
        std::optional<ttt_model_config_t> ttt;
    };

    struct text_repair_model_config_t {
        std::optional<ttt_model_config_t> diacritizer_he;
        std::optional<ttt_model_config_t> diacritizer_ar;
        std::optional<ttt_model_config_t> punctuation;
    };

    struct model_config_t {
        std::optional<stt_model_config_t> stt;
        std::optional<tts_model_config_t> tts;
        std::optional<mnt_model_config_t> mnt;
        std::optional<text_repair_model_config_t> text_repair;
        QString options;
    };

    struct task_t {
        int id = INVALID_TASK;
        engine_t engine = engine_t::stt;
        QString model_id;
        speech_mode_t speech_mode = speech_mode_t::single_sentence;
        QString out_lang;
        double progress = 0.0;
        std::vector<QString> files;
        QVariantMap options;
        bool paused = false;
        bool stt_clear_mic_audio_when_decoding = false;
    };

    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_SPEECH_SERVICE)};
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
    std::unique_ptr<text_repair_engine> m_text_repair_engine;
    std::unique_ptr<mnt_engine> m_mnt_engine;
    std::unique_ptr<audio_source> m_source;
    std::map<QString, model_data_t>
        m_available_stt_models_map;  // model-id => model data
    std::map<QString, model_data_t>
        m_available_tts_models_map;  // model-id => model data
    std::map<QString, model_data_t>
        m_available_ttt_models_map;  // model-id => model data
    std::map<QString, model_data_t>
        m_available_mnt_models_map;  // model-id => model data
    std::unordered_map<QString, QString>
        m_available_mnt_lang_to_model_id_map;  // lang-id => model-id
    double m_progress = -1;
    state_t m_state = state_t::unknown;
    SpeechAdaptor m_dbus_service_adaptor;
    QTimer m_keepalive_timer;
    QTimer m_keepalive_current_task_timer;
    QTimer m_features_availability_timer;
    int m_last_intermediate_text_task = INVALID_TASK;
    std::optional<task_t> m_previous_task;
    std::optional<task_t> m_current_task;
    QMediaPlayer m_player;
    int m_task_state = 0;
    std::queue<tts_partial_result_t> m_tts_queue;
    QVariantMap m_features_availability;
    bool m_models_changed_handled = false;

    inline bool feature_discovery_done() const {
        return !m_features_availability.isEmpty();
    }
    void handle_models_changed();
    void handle_tts_models_changed();
    void handle_stt_sentence_timeout(int task_id);
    void handle_stt_engine_eof(int task_id);
    void handle_stt_engine_error(int task_id);
    void handle_stt_engine_stopped(int task_id);
    void handle_stt_engine_stopping(int task_id);
    void handle_tts_engine_error(int task_id);
    void handle_text_repair_engine_error(int task_id);
    void handle_mnt_engine_error(mnt_engine::error_t error_type);
    void handle_mnt_engine_error(mnt_engine::error_t error_type, int task_id);
    void handle_tts_engine_state_changed(tts_engine::state_t state,
                                         int task_id);
    void handle_text_repair_engine_state_changed(
        text_repair_engine::state_t state, int task_id);
    void handle_mnt_engine_state_changed(mnt_engine::state_t state,
                                         int task_id);
    void handle_mnt_progress_changed(int task_id);
    void handle_mnt_translate_finished(const std::string &in_text,
                                       const std::string &in_lang,
                                       std::string &&out_text,
                                       const std::string &out_lang);
    void handle_ttt_text_repaired(const QString &text, int task_id);
    void handle_stt_text_decoded(const std::string &text,
                                 const std::string &lang);
    void handle_stt_text_decoded(const QString &text, const QString &lang,
                                 int task_id);
    void handle_stt_intermediate_text_decoded(const std::string &text,
                                              const std::string &lang);
    void handle_tts_speech_encoded(const std::string &text,
                                   const std::string &audio_file_path,
                                   tts_engine::audio_format_t format,
                                   double progress, bool last);
    void handle_tts_speech_encoded(tts_partial_result_t result);
    void handle_speech_to_file(const tts_partial_result_t &result);
    void handle_player_state_changed(QMediaPlayer::State new_state);
    void handle_audio_available();
    void handle_stt_engine_state_changed(
        stt_engine::speech_detection_status_t status, int task_id);
    void handle_processing_changed(bool processing);
    void handle_audio_error();
    void handle_audio_ended();
    QString restart_stt_engine(speech_mode_t speech_mode,
                               const QString &model_id,
                               const QString &out_lang_id,
                               const QVariantMap &options);
    QString restart_tts_engine(const QString &model_id,
                               const QVariantMap &options);
    QString restart_mnt_engine(const QString &model_or_lang_id,
                               const QString &out_lang_id,
                               const QVariantMap &options);
    bool restart_text_repair_engine(const QVariantMap &options);
    struct stt_source_file_props_t {
        QString file;
        int stream_index = -1;
    };
    void restart_audio_source(
        const std::variant<QString, stt_source_file_props_t> &config);
    void stop_stt();
    source_t audio_source_type() const;
    void set_progress(double progress);
    std::optional<speech_service::model_config_t> choose_model_config_by_id(
        const std::vector<models_manager::model_t> &models,
        engine_t engine_type, const QString &model_or_lang_id,
        const QString &out_lang_id);
    std::optional<speech_service::model_config_t> choose_model_config_by_lang(
        const std::vector<models_manager::model_t> &models,
        engine_t engine_type, const QString &model_or_lang_id);
    std::optional<speech_service::model_config_t> choose_model_config_by_first(
        const std::vector<models_manager::model_t> &models,
        engine_t engine_type);
    std::optional<model_config_t> choose_model_config(engine_t engine_type,
                                                      QString model_id = {},
                                                      QString out_lang_id = {});
    inline auto recording() const { return static_cast<bool>(m_source); }
    void refresh_status();
    void stop_stt_engine_gracefully();
    void stop_stt_engine();
    void stop_tts_engine();
    void stop_text_repair_engine();
    void stop_mnt_engine();
    QString test_default_stt_model(const QString &lang) const;
    QString test_default_tts_model(const QString &lang) const;
    QString test_default_mnt_model(const QString &lang) const;
    QString test_default_mnt_lang(const QString &lang) const;
    QString test_default_mnt_out_lang(const QString &lang) const;
    QString test_mnt_out_lang(const QString &in_lang,
                              const QString &out_lang) const;
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
    QVariantMap available_trg_langs(
        const std::map<QString, model_data_t> &available_models_map) const;
    QVariantList available_lang_list(
        const std::map<QString, model_data_t> &available_models_map) const;
    std::set<QString> available_lang_set(
        const std::map<QString, model_data_t> &available_models_map) const;
    static QString test_default_model(
        const QString &lang,
        const std::map<QString, model_data_t> &available_models_map);
    void set_state(state_t new_state);
    void update_task_state();
    void handle_tts_queue();
    static std::vector<std::reference_wrapper<const model_data_t>>
    model_data_for_lang(
        const QString &lang_id,
        const std::map<QString, model_data_t> &available_models_map);
    void fill_available_models_map(
        const std::vector<models_manager::model_t> &models);
    static bool matched_engine_type(engine_t engine_type,
                                    models_manager::model_engine_t engine);
    static unsigned int tts_speech_speed_from_options(
        const QVariantMap &options);
    static void setup_modules();
    static void setup_env();
    void clean_tts_queue();
    static bool get_bool_value_from_options(const QString &name,
                                            bool default_value,
                                            const QVariantMap &options);
    static int get_int_value_from_options(const QString &name,
                                          int default_value,
                                          const QVariantMap &options);
    static QString get_string_value_from_options(const QString &name,
                                                 const QString &default_value,
                                                 const QVariantMap &options);

    // DBus
    Q_INVOKABLE int Cancel(int task);
    Q_INVOKABLE int KeepAliveService();
    Q_INVOKABLE int KeepAliveTask(int task);
    Q_INVOKABLE int Reload();
    Q_INVOKABLE int SttStartListen(int mode, const QString &lang,
                                   const QString &out_lang);
    Q_INVOKABLE int SttStartListen2(int mode, const QString &lang,
                                    const QString &out_lang,
                                    const QVariantMap &options);
    Q_INVOKABLE int SttStopListen(int task);
    Q_INVOKABLE int SttTranscribeFile(const QString &file, const QString &lang,
                                      const QString &out_lang,
                                      const QVariantMap &options);
    Q_INVOKABLE double SttGetFileTranscribeProgress(int task);
    Q_INVOKABLE int TtsPlaySpeech(const QString &text, const QString &lang);
    Q_INVOKABLE int TtsPlaySpeech2(const QString &text, const QString &lang,
                                   const QVariantMap &options);
    Q_INVOKABLE int TtsPauseSpeech(int task);
    Q_INVOKABLE int TtsResumeSpeech(int task);
    Q_INVOKABLE int TtsStopSpeech(int task);
    Q_INVOKABLE int TtsSpeechToFile(const QString &text, const QString &lang,
                                    const QVariantMap &options);
    Q_INVOKABLE double TtsGetSpeechToFileProgress(int task);
    Q_INVOKABLE int MntTranslate(const QString &text, const QString &lang,
                                 const QString &out_lang);
    Q_INVOKABLE int MntTranslate2(const QString &text, const QString &lang,
                                  const QString &out_lang,
                                  const QVariantMap &options);
    Q_INVOKABLE QVariantMap MntGetOutLangs(const QString &lang);
    Q_INVOKABLE int TttRepairText(const QString &text,
                                  const QVariantMap &options);
    Q_INVOKABLE QVariantMap FeaturesAvailability();
};

Q_DECLARE_METATYPE(speech_service::tts_partial_result_t)
Q_DECLARE_METATYPE(speech_service::error_t)
Q_DECLARE_METATYPE(mnt_engine::state_t)
Q_DECLARE_METATYPE(mnt_engine::error_t)
Q_DECLARE_METATYPE(tts_engine::state_t)
Q_DECLARE_METATYPE(stt_engine::speech_detection_status_t)
Q_DECLARE_METATYPE(text_repair_engine::state_t)

#endif  // SPEECH_SERVICE_H

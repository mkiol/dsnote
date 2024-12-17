/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DSNOTE_APP_H
#define DSNOTE_APP_H

#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include <optional>
#include <queue>

#ifdef USE_X11_FEATURES
#include <qhotkey.h>
#include "fake_keyboard.hpp"
#endif

#ifdef USE_DESKTOP
#include "tray_icon.hpp"
#endif

#include "audio_device_manager.hpp"
#include "config.h"
#include "dbus_notifications_inf.h"
#include "dbus_speech_inf.h"
#include "media_converter.hpp"
#include "recorder.hpp"
#include "settings.h"

// name, name_str
#define ACTION_TABLE                                                  \
    X(start_listening, "start-listening")                             \
    X(start_listening_translate, "start-listening-translate")         \
    X(start_listening_active_window, "start-listening-active-window") \
    X(start_listening_translate_active_window,                        \
      "start-listening-translate-active-window")                      \
    X(start_listening_clipboard, "start-listening-clipboard")         \
    X(start_listening_translate_clipboard,                            \
      "start-listening-translate-clipboard")                          \
    X(stop_listening, "stop-listening")                               \
    X(start_reading, "start-reading")                                 \
    X(start_reading_clipboard, "start-reading-clipboard")             \
    X(start_reading_text, "start-reading-text")                       \
    X(pause_resume_reading, "pause-resume-reading")                   \
    X(cancel, "cancel")                                               \
    X(switch_to_next_stt_model, "switch-to-next-stt-model")           \
    X(switch_to_next_tts_model, "switch-to-next-tts-model")           \
    X(switch_to_prev_stt_model, "switch-to-prev-stt-model")           \
    X(switch_to_prev_tts_model, "switch-to-prev-tts-model")           \
    X(set_stt_model, "set-stt-model")                                 \
    X(set_tts_model, "set-tts-model")

class dsnote_app : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString intermediate_text READ intermediate_text NOTIFY
                   intermediate_text_changed)
    Q_PROPERTY(QString note READ note WRITE set_note NOTIFY note_changed)
    Q_PROPERTY(QString translated_text READ translated_text WRITE
                   set_translated_text NOTIFY translated_text_changed)
    Q_PROPERTY(bool can_undo_note READ can_undo_note NOTIFY
                   can_undo_or_redu_note_changed)
    Q_PROPERTY(bool can_redo_note READ can_redo_note NOTIFY
                   can_undo_or_redu_note_changed)
    Q_PROPERTY(int last_cursor_position READ last_cursor_position WRITE
                   set_last_cursor_position NOTIFY last_cursor_position_changed)

    // player
    Q_PROPERTY(bool player_ready READ player_ready NOTIFY player_ready_changed)
    Q_PROPERTY(
        bool player_playing READ player_playing NOTIFY player_playing_changed)
    Q_PROPERTY(long long player_duration READ player_duration NOTIFY
                   player_duration_changed)
    Q_PROPERTY(long long player_position READ player_position NOTIFY
                   player_position_changed)
    Q_PROPERTY(
        int player_current_voice_ref_idx READ player_current_voice_ref_idx
            NOTIFY player_current_voice_ref_idx_changed)

    // recorder
    Q_PROPERTY(bool recorder_recording READ recorder_recording NOTIFY
                   recorder_recording_changed)
    Q_PROPERTY(long long recorder_duration READ recorder_duration NOTIFY
                   recorder_duration_changed)
    Q_PROPERTY(bool recorder_processing READ recorder_processing NOTIFY
                   recorder_processing_changed)

    // stt
    Q_PROPERTY(int active_stt_model_idx READ active_stt_model_idx NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString active_stt_model READ active_stt_model NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString active_stt_model_name READ active_stt_model_name NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QVariantList available_stt_models READ available_stt_models
                   NOTIFY available_stt_models_changed)
    Q_PROPERTY(bool stt_translate_needed READ stt_translate_needed NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString stt_auto_lang_id READ stt_auto_lang_id NOTIFY
                   stt_auto_lang_changed)
    Q_PROPERTY(QString stt_auto_lang_name READ stt_auto_lang_name NOTIFY
                   stt_auto_lang_changed)

    // tts ref voices
    Q_PROPERTY(int active_tts_ref_voice_idx READ active_tts_ref_voice_idx NOTIFY
                   active_tts_ref_voice_changed)
    Q_PROPERTY(QString active_tts_ref_voice READ active_tts_ref_voice NOTIFY
                   active_tts_ref_voice_changed)
    Q_PROPERTY(QString active_tts_ref_voice_name READ active_tts_ref_voice_name
                   NOTIFY active_tts_ref_voice_changed)
    Q_PROPERTY(bool tts_ref_voice_needed READ tts_ref_voice_needed NOTIFY
                   active_tts_model_changed)

    Q_PROPERTY(int active_tts_for_in_mnt_ref_voice_idx READ
                   active_tts_for_in_mnt_ref_voice_idx NOTIFY
                       active_tts_for_in_mnt_ref_voice_changed)
    Q_PROPERTY(QString active_tts_for_in_mnt_ref_voice READ
                   active_tts_for_in_mnt_ref_voice NOTIFY
                       active_tts_for_in_mnt_ref_voice_changed)
    Q_PROPERTY(QString active_tts_for_in_mnt_ref_voice_name READ
                   active_tts_for_in_mnt_ref_voice_name NOTIFY
                       active_tts_for_in_mnt_ref_voice_changed)
    Q_PROPERTY(bool tts_for_in_mnt_ref_voice_needed READ
                   tts_for_in_mnt_ref_voice_needed NOTIFY
                       active_tts_model_for_in_mnt_changed)

    Q_PROPERTY(int active_tts_for_out_mnt_ref_voice_idx READ
                   active_tts_for_out_mnt_ref_voice_idx NOTIFY
                       active_tts_for_out_mnt_ref_voice_changed)
    Q_PROPERTY(QString active_tts_for_out_mnt_ref_voice READ
                   active_tts_for_out_mnt_ref_voice NOTIFY
                       active_tts_for_out_mnt_ref_voice_changed)
    Q_PROPERTY(QString active_tts_for_out_mnt_ref_voice_name READ
                   active_tts_for_out_mnt_ref_voice_name NOTIFY
                       active_tts_for_out_mnt_ref_voice_changed)
    Q_PROPERTY(bool tts_for_out_mnt_ref_voice_needed READ
                   tts_for_out_mnt_ref_voice_needed NOTIFY
                       active_tts_model_for_out_mnt_changed)

    Q_PROPERTY(
        QVariantList available_tts_ref_voices READ available_tts_ref_voices
            NOTIFY available_tts_ref_voices_changed)

    // tts
    Q_PROPERTY(int active_tts_model_idx READ active_tts_model_idx NOTIFY
                   active_tts_model_changed)
    Q_PROPERTY(QString active_tts_model READ active_tts_model NOTIFY
                   active_tts_model_changed)
    Q_PROPERTY(QString active_tts_model_name READ active_tts_model_name NOTIFY
                   active_tts_model_changed)
    Q_PROPERTY(QVariantList available_tts_models READ available_tts_models
                   NOTIFY available_tts_models_changed)

    // mnt
    Q_PROPERTY(int active_mnt_lang_idx READ active_mnt_lang_idx NOTIFY
                   active_mnt_lang_changed)
    Q_PROPERTY(QVariantList available_mnt_langs READ available_mnt_langs NOTIFY
                   available_mnt_langs_changed)
    Q_PROPERTY(QString active_mnt_lang READ active_mnt_lang NOTIFY
                   active_mnt_lang_changed)
    Q_PROPERTY(QString active_mnt_lang_name READ active_mnt_lang_name NOTIFY
                   active_mnt_lang_changed)
    Q_PROPERTY(int active_mnt_out_lang_idx READ active_mnt_out_lang_idx NOTIFY
                   active_mnt_out_lang_changed)
    Q_PROPERTY(QVariantList available_mnt_out_langs READ available_mnt_out_langs
                   NOTIFY available_mnt_out_langs_changed)
    Q_PROPERTY(QString active_mnt_out_lang READ active_mnt_out_lang NOTIFY
                   active_mnt_out_lang_changed)
    Q_PROPERTY(QString active_mnt_out_lang_name READ active_mnt_out_lang_name
                   NOTIFY active_mnt_out_lang_changed)
    Q_PROPERTY(double translate_progress READ translate_progress NOTIFY
                   translate_progress_changed)

    // tts for mnt
    Q_PROPERTY(
        int active_tts_model_for_in_mnt_idx READ active_tts_model_for_in_mnt_idx
            NOTIFY active_tts_model_for_in_mnt_changed)
    Q_PROPERTY(
        QString active_tts_model_for_in_mnt READ active_tts_model_for_in_mnt
            NOTIFY active_tts_model_for_in_mnt_changed)
    Q_PROPERTY(QString active_tts_model_for_in_mnt_name READ
                   active_tts_model_for_in_mnt_name NOTIFY
                       active_tts_model_for_in_mnt_changed)
    Q_PROPERTY(QVariantList available_tts_models_for_in_mnt READ
                   available_tts_models_for_in_mnt NOTIFY
                       available_tts_models_for_in_mnt_changed)
    Q_PROPERTY(int active_tts_model_for_out_mnt_idx READ
                   active_tts_model_for_out_mnt_idx NOTIFY
                       active_tts_model_for_out_mnt_changed)
    Q_PROPERTY(
        QString active_tts_model_for_out_mnt READ active_tts_model_for_out_mnt
            NOTIFY active_tts_model_for_out_mnt_changed)
    Q_PROPERTY(QString active_tts_model_for_out_mnt_name READ
                   active_tts_model_for_out_mnt_name NOTIFY
                       active_tts_model_for_out_mnt_changed)
    Q_PROPERTY(QVariantList available_tts_models_for_out_mnt READ
                   available_tts_models_for_out_mnt NOTIFY
                       available_tts_models_for_out_mnt_changed)

    // audio sources
    Q_PROPERTY(QStringList audio_sources READ audio_sources NOTIFY
                   audio_sources_changed)
    Q_PROPERTY(int audio_source_idx READ audio_source_idx WRITE
                   set_audio_source_idx NOTIFY audio_source_changed)
    Q_PROPERTY(QString audio_source READ audio_source WRITE set_audio_source
                   NOTIFY audio_source_changed)

    Q_PROPERTY(service_task_state_t task_state READ task_state NOTIFY
                   task_state_changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
    Q_PROPERTY(
        bool stt_configured READ stt_configured NOTIFY stt_configured_changed)
    Q_PROPERTY(
        bool tts_configured READ tts_configured NOTIFY tts_configured_changed)
    Q_PROPERTY(
        bool ttt_diacritizer_ar_configured READ ttt_diacritizer_ar_configured
            NOTIFY ttt_diacritizer_ar_configured_changed)
    Q_PROPERTY(
        bool ttt_diacritizer_he_configured READ ttt_diacritizer_he_configured
            NOTIFY ttt_diacritizer_he_configured_changed)
    Q_PROPERTY(bool ttt_punctuation_configured READ ttt_punctuation_configured
                   NOTIFY ttt_punctuation_configured_changed)
    Q_PROPERTY(
        bool mnt_configured READ mnt_configured NOTIFY mnt_configured_changed)
    Q_PROPERTY(bool connected READ connected NOTIFY connected_changed)
    Q_PROPERTY(bool another_app_connected READ another_app_connected NOTIFY
                   another_app_connected_changed)
    Q_PROPERTY(double transcribe_progress READ transcribe_progress NOTIFY
                   transcribe_progress_changed)
    Q_PROPERTY(double speech_to_file_progress READ speech_to_file_progress
                   NOTIFY speech_to_file_progress_changed)
    Q_PROPERTY(double mc_progress READ mc_progress NOTIFY mc_progress_changed)
    Q_PROPERTY(
        service_state_t state READ service_state NOTIFY service_state_changed)
    Q_PROPERTY(
        QVariantMap translations READ translations NOTIFY connected_changed)
    Q_PROPERTY(
        QString trans_rules_test_text READ trans_rules_test_text WRITE
            set_trans_rules_test_text NOTIFY trans_rules_test_text_changed)

    // features
#define FEATURE_OPT(name) \
    Q_PROPERTY(bool feature_##name READ feature_##name NOTIFY features_changed)
    FEATURE_OPT(whispercpp_stt)
    FEATURE_OPT(whispercpp_gpu)
    FEATURE_OPT(fasterwhisper_stt)
    FEATURE_OPT(fasterwhisper_gpu)
    FEATURE_OPT(whisperspeech_tts)
    FEATURE_OPT(whisperspeech_gpu)
    FEATURE_OPT(coqui_tts)
    FEATURE_OPT(coqui_gpu)
    FEATURE_OPT(punctuator)
    FEATURE_OPT(diacritizer_he)
    FEATURE_OPT(global_shortcuts)
    FEATURE_OPT(text_active_window)
    FEATURE_OPT(translator)
#undef FEATURE_OPT

    Q_PROPERTY(auto_text_format_t auto_text_format READ auto_text_format NOTIFY
                   auto_text_format_changed)

   public:
    enum service_state_t {
        StateUnknown = 0,
        StateNotConfigured = 1,
        StateBusy = 2,
        StateIdle = 3,
        StateListeningManual = 4,
        StateListeningAuto = 5,
        StateTranscribingFile = 6,
        StateListeningSingleSentence = 7,
        StatePlayingSpeech = 8,
        StateWritingSpeechToFile = 9,
        StateTranslating = 10,
        StateRepairingText = 11,
        StateImporting = 20,
        StateExporting = 21
    };
    Q_ENUM(service_state_t)
    friend QDebug operator<<(QDebug d, service_state_t state);

    enum service_task_state_t {
        TaskStateIdle = 0,
        TaskStateSpeechDetected = 1,
        TaskStateProcessing = 2,
        TaskStateInitializing = 3,
        TaskStateSpeechPlaying = 4,
        TaskStateSpeechPaused = 5,
        TaskStateCancelling = 6
    };
    Q_ENUM(service_task_state_t)
    friend QDebug operator<<(QDebug d, service_task_state_t type);

    enum error_t : unsigned int {
        ErrorGeneric = 0,
        ErrorMicSource = 1,
        ErrorFileSource = 2,
        ErrorSttEngine = 3,
        ErrorTtsEngine = 4,
        ErrorMntEngine = 5,
        ErrorMntRuntime = 6,
        ErrorTextRepairEngine =
            7,  // has to be the same as speech_service::error_t
        ErrorExportFileGeneral = 10,
        ErrorImportFileGeneral = 11,
        ErrorImportFileNoStreams = 12,
        ErrorSttNotConfigured = 13,
        ErrorTtsNotConfigured = 14,
        ErrorMntNotConfigured = 15,
        ErrorContentDownload = 16,
        ErrorNoService = 100
    };
    Q_ENUM(error_t)
    friend QDebug operator<<(QDebug d, error_t type);

    enum class file_import_result_t {
        ok_streams_selection,
        ok_import_audio,
        ok_import_subtitles,
        ok_import_text,
        error_no_supported_streams,
        error_requested_stream_not_found,
        error_import_audio_stt_not_configured,
        error_import_subtitles_error,
        error_import_text_error
    };
    friend QDebug operator<<(QDebug d, file_import_result_t type);

    enum class auto_text_format_t {
        AutoTextFormatRaw = 0,
        AutoTextFormatSubRip = 3
    };
    Q_ENUM(auto_text_format_t)
    friend QDebug operator<<(QDebug d, auto_text_format_t format);

    dsnote_app(QObject *parent = nullptr);
    Q_INVOKABLE void set_active_stt_model_idx(int idx);
    Q_INVOKABLE void set_active_tts_model_idx(int idx);
    Q_INVOKABLE void set_active_next_stt_model();
    Q_INVOKABLE void set_active_next_tts_model();
    Q_INVOKABLE void set_active_prev_stt_model();
    Q_INVOKABLE void set_active_prev_tts_model();
    Q_INVOKABLE void set_active_tts_ref_voice_idx(int idx);
    Q_INVOKABLE void set_active_tts_for_in_mnt_ref_voice_idx(int idx);
    Q_INVOKABLE void set_active_tts_for_out_mnt_ref_voice_idx(int idx);
    Q_INVOKABLE void delete_tts_ref_voice(int idx);
    Q_INVOKABLE void rename_tts_ref_voice(int idx, const QString &new_name);
    Q_INVOKABLE void set_active_mnt_lang_idx(int idx);
    Q_INVOKABLE void set_active_mnt_out_lang_idx(int idx);
    Q_INVOKABLE void set_active_tts_model_for_in_mnt_idx(int idx);
    Q_INVOKABLE void set_active_tts_model_for_out_mnt_idx(int idx);
    Q_INVOKABLE void import_file(const QString &file_path, int stream_index,
                                 bool replace);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void listen();
    Q_INVOKABLE void listen_translate();
    Q_INVOKABLE void listen_to_active_window();
    Q_INVOKABLE void listen_translate_to_active_window();
    Q_INVOKABLE void listen_to_clipboard();
    Q_INVOKABLE void listen_translate_to_clipboard();
    Q_INVOKABLE void stop_listen();
    Q_INVOKABLE void play_speech();
    Q_INVOKABLE void play_speech_selected(int start, int end);
    Q_INVOKABLE void play_speech_from_text(const QString &text,
                                           const QString &model_id);
    Q_INVOKABLE void play_speech_translator(bool transtalated);
    Q_INVOKABLE void play_speech_translator_selected(int start, int end,
                                                     bool transtalated);
    Q_INVOKABLE void restore_diacritics_ar();
    Q_INVOKABLE void restore_diacritics_he();
    Q_INVOKABLE void restore_punctuation();
    Q_INVOKABLE void pause_speech();
    Q_INVOKABLE void resume_speech();
    Q_INVOKABLE void translate();
    Q_INVOKABLE void translate_selected(int start, int end);
    Q_INVOKABLE void translate_delayed();
    Q_INVOKABLE void speech_to_file(const QString &dest_file,
                                    const QString &title_tag = {},
                                    const QString &track_tag = {});
    Q_INVOKABLE void speech_to_file_translator(bool transtalated,
                                               const QString &dest_file,
                                               const QString &title_tag = {},
                                               const QString &track_tag = {});
    Q_INVOKABLE void export_to_text_file(const QString &dest_file,
                                         bool translation);
    Q_INVOKABLE void export_to_audio_mix(const QString &input_file,
                                         int input_stream_index,
                                         const QString &dest_file,
                                         const QString &title_tag,
                                         const QString &track_tag);
    Q_INVOKABLE void export_to_audio_mix_translator(bool transtalated,
                                                    const QString &input_file,
                                                    int input_stream_index,
                                                    const QString &dest_file,
                                                    const QString &title_tag,
                                                    const QString &track_tag);
    Q_INVOKABLE void import_files(const QStringList &input_files, bool replace);
    Q_INVOKABLE void import_files_url(const QList<QUrl> &input_urls,
                                      bool replace);
    Q_INVOKABLE void stop_play_speech();
    Q_INVOKABLE void copy_to_clipboard();
    Q_INVOKABLE void copy_translation_to_clipboard();
    Q_INVOKABLE void copy_text_to_clipboard(const QString &text);
    Q_INVOKABLE QVariantMap file_info(const QString &file) const;
    Q_INVOKABLE void undo_or_redu_note();
    Q_INVOKABLE void make_undo();
    Q_INVOKABLE void update_note(const QString &text, bool replace);
    Q_INVOKABLE void switch_translated_text();
    Q_INVOKABLE void close_desktop_notification();
    Q_INVOKABLE void show_desktop_notification(const QString &summary,
                                               const QString &body,
                                               bool permanent = false);
    Q_INVOKABLE void execute_action_name(const QString &action_name,
                                         const QString &extra);
    Q_INVOKABLE QVariantList features_availability();
    Q_INVOKABLE QString download_content(const QUrl &url);
    Q_INVOKABLE void player_import_from_url(const QUrl &url);
    Q_INVOKABLE void player_play_voice_ref_idx(int idx);
    Q_INVOKABLE void player_play(long long start, long long stop);
    Q_INVOKABLE void player_stop();
    Q_INVOKABLE void player_set_position(long long position);
    Q_INVOKABLE void player_export_ref_voice(long long start, long long stop,
                                             const QString &name);
    Q_INVOKABLE void player_stop_voice_ref();
    Q_INVOKABLE void player_reset();
    Q_INVOKABLE void recorder_start();
    Q_INVOKABLE void recorder_stop();
    Q_INVOKABLE void recorder_reset();
    Q_INVOKABLE void show_tray();
    Q_INVOKABLE void hide_tray();
    Q_INVOKABLE void set_app_window(QObject *app_window);
    Q_INVOKABLE QString special_key_name(int key) const;
    /* used in QML, returns list: [0] bool:matched, [0] string:out-text */
    Q_INVOKABLE QVariantList test_trans_rule(const QString &text,
                                             const QString &pattern,
                                             const QString &replace,
                                             unsigned int type);
    Q_INVOKABLE void update_trans_rule(int index, unsigned int flags,
                                       const QString &name,
                                       const QString &pattern,
                                       const QString &replace,
                                       const QString &langs, unsigned int type);
    Q_INVOKABLE bool trans_rule_re_pattern_valid(const QString &pattern);
    [[nodiscard]] Q_INVOKABLE QList<QStringList> available_stt_models_info()
        const;
    [[nodiscard]] Q_INVOKABLE QList<QStringList> available_tts_models_info()
        const;

   signals:
    void active_stt_model_changed();
    void available_stt_models_changed();
    void active_tts_model_changed();
    void available_tts_models_changed();
    void available_ttt_models_changed();
    void available_mnt_langs_changed();
    void available_mnt_out_langs_changed();
    void active_mnt_lang_changed();
    void active_mnt_out_lang_changed();
    void active_tts_model_for_in_mnt_changed();
    void active_tts_model_for_out_mnt_changed();
    void available_tts_models_for_in_mnt_changed();
    void available_tts_models_for_out_mnt_changed();
    void intermediate_text_changed();
    void text_changed();
    void text_decoded_to_clipboard();
    void text_decoded_to_active_window();
    void task_state_changed();
    void busy_changed();
    void stt_configured_changed();
    void tts_configured_changed();
    void ttt_diacritizer_ar_configured_changed();
    void ttt_diacritizer_he_configured_changed();
    void ttt_punctuation_configured_changed();
    void mnt_configured_changed();
    void connected_changed();
    void another_app_connected_changed();
    void transcribe_progress_changed();
    void speech_to_file_progress_changed();
    void translate_progress_changed();
    void error(dsnote_app::error_t type);
    void transcribe_done();
    void text_repair_done();
    void save_note_to_file_done();
    void service_state_changed();
    void note_copied();
    void translated_text_changed();
    void note_changed();
    void can_undo_or_redu_note_changed();
    void can_open_next_file();
    void features_availability_updated();
    void features_changed();
    void activate_requested();
    void active_tts_ref_voice_changed();
    void active_tts_for_in_mnt_ref_voice_changed();
    void active_tts_for_out_mnt_ref_voice_changed();
    void available_tts_ref_voices_changed();
    void player_ready_changed();
    void player_playing_changed();
    void player_duration_changed();
    void player_position_changed();
    void recorder_recording_changed();
    void recorder_duration_changed();
    void recorder_processing_changed();
    void recorder_new_stream_name(QString name);
    void recorder_new_probs(QVariantList probs);
    void tray_activated();
    void player_current_voice_ref_idx_changed();
    void import_file_multiple_streams(QString file_path, QStringList streams,
                                      bool replace);
    void auto_text_format_changed();
    void mc_progress_changed();
    void audio_devices_changed();
    void audio_source_changed();
    void audio_sources_changed();
    void stt_auto_lang_changed();
    void last_cursor_position_changed();
    void trans_rules_test_text_changed();

   private:
    enum class action_t : uint8_t {
#define X(name, str) name,
        ACTION_TABLE
#undef X
    };
    friend QDebug operator<<(QDebug d, action_t type);

    enum class text_repair_task_type_t {
        none = 0,
        restore_diacritics_ar = 1,
        restore_diacritics_he = 2,
        restore_punctuation = 3,
    };

    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_SPEECH_SERVICE)};
    inline static const QString DBUS_SERVICE_PATH{QStringLiteral("/")};
    static const int SUCCESS = 0;
    static const int FAILURE = -1;
    static const int INVALID_TASK = -1;
    static const int KEEPALIVE_TIME = 1000;  // 1s
    static const int MAX_INIT_ATTEMPTS = 120;
    inline static const auto *const s_imported_ref_file_name =
        "tmp_imported_ref_voice.wav";
    inline static const auto *const s_ref_voices_dir_name = "ref_voices";

    struct task_t {
        int current = INVALID_TASK;
        int previous = INVALID_TASK;
        inline void set(int id) {
            previous = current;
            current = id;
        }
        inline bool operator==(int id) const {
            return id > INVALID_TASK && (current == id || previous == id);
        }
        inline bool operator!=(int id) const { return !this->operator==(id); }
        inline operator bool() const { return current > INVALID_TASK; }
        inline void reset() { this->set(INVALID_TASK); }
    };

    struct desktop_notification_t {
        unsigned int id = 0;
        QString summary;
        QString body;
        bool permanent = false;
        bool close_request = false;
    };

    enum class text_destination_t {
        note_add,
        note_replace,
        active_window,
        clipboard
    };

    enum class mc_state_t { idle, extracting_subtitles };

    enum class stt_translate_req_t { conf, on, off };

    struct dest_file_info_t {
        QString input_path;
        QString output_path;
        int input_stream_index = -1;
        QString title_tag;
        QString track_tag;
        settings::audio_format_t audio_format =
            settings::audio_format_t::AudioFormatWav;
    };

    enum class stt_request_t : uint8_t {
        listen,
        listen_translate,
        listen_active_window,
        listen_translate_active_window,
        listen_clipboard,
        listen_translate_clipboard,
        transcribe_file
    };
    friend QDebug operator<<(QDebug d, dsnote_app::stt_request_t request);

    struct trans_rule_t {
        settings::trans_rule_flags_t flags =
            settings::trans_rule_flags_t::TransRuleNone;
        settings::trans_rule_type_t type =
            settings::trans_rule_type_t::TransRuleTypeNone;
        QString name;
        QString pattern;
        QString replace;
    };
    friend QDebug operator<<(QDebug d, const trans_rule_t &rule);

    struct trans_rule_result_t {
        std::underlying_type_t<settings::trans_rule_flags_t> action_flags =
            settings::trans_rule_flags_t::TransRuleNone;
    };

    QString m_active_stt_model;
    QVariantMap m_available_stt_models_map;
    QString m_active_tts_model;
    QString m_active_tts_ref_voice;
    QString m_active_tts_for_in_mnt_ref_voice;
    QString m_active_tts_for_out_mnt_ref_voice;
    QVariantMap m_available_tts_models_map;
    QVariantMap m_available_tts_ref_voices_map;
    QVariantMap m_available_ttt_models_map;
    QString m_active_mnt_lang;
    QString m_active_mnt_out_lang;
    QVariantMap m_available_mnt_langs_map;
    QVariantMap m_available_mnt_out_langs_map;
    QString m_active_tts_model_for_in_mnt;
    QVariantMap m_available_tts_models_for_in_mnt_map;
    QString m_active_tts_model_for_out_mnt;
    QVariantMap m_available_tts_models_for_out_mnt_map;
    QString m_intermediate_text;
    double m_speech_to_file_progress = -1.0;
    double m_translate_progress = -1.0;
    double m_transcribe_progress = -1.0;
    OrgMkiolSpeechInterface m_dbus_service;
    OrgFreedesktopNotificationsInterface m_dbus_notifications;
    service_state_t m_service_state = service_state_t::StateUnknown;
    service_task_state_t m_task_state = service_task_state_t::TaskStateIdle;
    task_t m_primary_task;
    int m_current_task = INVALID_TASK;
    int m_init_attempts = -1;
    QTimer m_keepalive_timer;
    QTimer m_keepalive_current_task_timer;
    QTimer m_translator_delay_timer;
    QTimer m_open_files_delay_timer;
    QTimer m_action_delay_timer;
    QTimer m_desktop_notification_delay_timer;
    QTimer m_auto_format_delay_timer;
    bool m_stt_configured = false;
    bool m_tts_configured = false;
    bool m_mnt_configured = false;
    bool m_ttt_diacritizer_ar_configured = false;
    bool m_ttt_diacritizer_he_configured = false;
    bool m_ttt_punctuation_configured = false;
    QString m_stt_auto_lang_id;
    dest_file_info_t m_dest_file_info;
    QString m_translated_text;
    QString m_prev_text;
    bool m_undo_flag = false;  // true => undo, false => redu
    std::queue<QString> m_files_to_open;
    std::optional<std::pair<action_t, QString>> m_pending_action;
    text_destination_t m_text_destination = text_destination_t::note_add;
    std::optional<desktop_notification_t> m_desktop_notification;
    QVariantMap m_features_availability;
    bool m_service_reload_called = false;
    bool m_service_reload_update_done = false;
    std::unique_ptr<QMediaPlayer> m_player;
    long long m_player_requested_play_position = -1;
    long long m_player_stop_position = -1;
    int m_player_current_voice_ref_idx = -1;
    std::unique_ptr<recorder> m_recorder;
    auto_text_format_t m_auto_text_format =
        auto_text_format_t::AutoTextFormatRaw;
    media_converter m_mc;
    audio_device_manager m_audio_dm;
    QStringList m_audio_sources;
    QObject *m_app_window = nullptr;
    int m_last_cursor_position = -1;
    std::optional<stt_request_t> m_current_stt_request;
    std::optional<stt_request_t> m_pending_stt_request;
    QString m_trans_rules_test_text;
#ifdef USE_X11_FEATURES
    struct hotkeys_t {
#define X(name, key) QHotkey name;
        HOTKEY_TABLE
#undef X
    };

    hotkeys_t m_hotkeys;
    std::optional<fake_keyboard> m_fake_keyboard;
#endif
#ifdef USE_DESKTOP
    tray_icon m_tray;
#endif
    [[nodiscard]] QVariantList available_stt_models() const;
    [[nodiscard]] QVariantList available_tts_models() const;
    [[nodiscard]] QVariantList available_tts_ref_voices() const;
    [[nodiscard]] QVariantList available_mnt_langs() const;
    [[nodiscard]] QVariantList available_mnt_out_langs() const;
    [[nodiscard]] QVariantList available_tts_models_for_in_mnt() const;
    [[nodiscard]] QVariantList available_tts_models_for_out_mnt() const;
    void handle_models_changed();
    void handle_stt_text_decoded(QString text, const QString &lang, int task);
    void handle_tts_partial_speech(const QString &text, int task);
    bool busy() const;
    void update_configured_state();
    bool stt_configured() const;
    bool tts_configured() const;
    bool ttt_diacritizer_ar_configured() const;
    bool ttt_diacritizer_he_configured() const;
    bool ttt_punctuation_configured() const;
    bool mnt_configured() const;
    bool connected() const;
    double transcribe_progress() const;
    double speech_to_file_progress() const;
    double translate_progress() const;
    bool another_app_connected() const;
    static service_task_state_t make_new_task_state(
        int task_state_from_service);
    void update_progress();
    void update_service_state();
    void update_task_state();
    void update_active_stt_model();
    void update_available_stt_models();
    void update_active_tts_model();
    void update_available_tts_models();
    void update_available_tts_ref_voices();
    void update_active_mnt_lang();
    void update_available_mnt_langs();
    void update_active_mnt_out_lang();
    void update_available_mnt_out_langs();
    void update_active_tts_model_for_in_mnt();
    void update_available_tts_models_for_in_mnt();
    void update_active_tts_model_for_out_mnt();
    void update_available_tts_models_for_out_mnt();
    void update_current_task();
    void update_listen();
    void set_service_state(service_state_t new_service_state);
    void set_task_state(service_task_state_t new_task_state);
    void handle_stt_intermediate_text(const QString &text, const QString &lang,
                                      int task);
    [[nodiscard]] int active_stt_model_idx() const;
    QString active_stt_model() { return m_active_stt_model; }
    QString active_stt_model_name() const;
    [[nodiscard]] int active_tts_model_idx() const;
    QString active_tts_model() { return m_active_tts_model; }
    QString active_tts_model_name() const;
    [[nodiscard]] int active_tts_ref_voice_idx() const;
    QString active_tts_ref_voice() { return m_active_tts_ref_voice; }
    QString active_tts_ref_voice_name() const;
    [[nodiscard]] int active_tts_for_in_mnt_ref_voice_idx() const;
    QString active_tts_for_in_mnt_ref_voice() {
        return m_active_tts_for_in_mnt_ref_voice;
    }
    QString active_tts_for_in_mnt_ref_voice_name() const;
    [[nodiscard]] int active_tts_for_out_mnt_ref_voice_idx() const;
    QString active_tts_for_out_mnt_ref_voice() {
        return m_active_tts_for_out_mnt_ref_voice;
    }
    QString active_tts_for_out_mnt_ref_voice_name() const;
    QString active_mnt_lang() { return m_active_mnt_lang; }
    QString active_mnt_lang_name() const;
    [[nodiscard]] int active_mnt_lang_idx() const;
    QString active_mnt_out_lang() { return m_active_mnt_out_lang; }
    QString active_mnt_out_lang_name() const;
    [[nodiscard]] int active_mnt_out_lang_idx() const;
    [[nodiscard]] int active_tts_model_for_in_mnt_idx() const;
    QString active_tts_model_for_in_mnt() {
        return m_active_tts_model_for_in_mnt;
    }
    QString active_tts_model_for_in_mnt_name() const;
    [[nodiscard]] int active_tts_model_for_out_mnt_idx() const;
    QString active_tts_model_for_out_mnt() {
        return m_active_tts_model_for_out_mnt;
    }
    QString active_tts_model_for_out_mnt_name() const;
    QString intermediate_text() const { return m_intermediate_text; }
    void update_active_stt_lang_idx();
    void update_active_tts_lang_idx();
    void update_active_mnt_lang_idx();
    void update_active_mnt_out_lang_idx();
    service_state_t service_state() const { return m_service_state; }
    auto task_state() const { return m_task_state; }
    auto auto_text_format() const { return m_auto_text_format; }
    auto mc_progress() const { return m_mc.progress(); }
    QStringList audio_sources() const;
    QString audio_source();
    void update_audio_sources();
    void set_audio_source(QString value);
    int audio_source_idx();
    void set_audio_source_idx(int value);
    void do_keepalive();
    void handle_keepalive_task_timeout();
    void handle_service_error(int code);
    void handle_stt_file_transcribe_finished(int task);
    void handle_stt_file_transcribe_progress(double new_progress, int task);
    void handle_tts_play_speech_finished(int task);
    void handle_ttt_repair_text_finished(const QString &text, int task);
    void handle_tts_speech_to_file_finished(const QStringList &files, int task);
    void handle_tts_speech_to_file_progress(double new_progress, int task);
    void handle_stt_default_model_changed(const QString &model);
    void set_active_stt_model(const QString &model);
    void set_active_stt_model_or_lang(const QString &model_or_lang);
    void set_active_tts_model_or_lang(const QString &model_or_lang);
    void handle_tts_default_model_changed(const QString &model);
    void set_active_tts_model(const QString &model);
    void handle_current_task_changed(int task);
    void handle_task_state_changed(int state);
    void handle_state_changed(int status);
    void handle_stt_models_changed(const QVariantMap &models);
    void handle_tts_models_changed(const QVariantMap &models);
    void handle_mnt_langs_changed(const QVariantMap &langs);
    void handle_mnt_default_lang_changed(const QString &lang);
    void handle_mnt_default_out_lang_changed(const QString &lang);
    void handle_translator_settings_changed();
    void handle_note_changed();
    void set_active_mnt_lang(const QString &lang);
    void set_active_mnt_out_lang(const QString &lang);
    void set_active_tts_model_for_in_mnt(const QString &model);
    void set_active_tts_model_for_out_mnt(const QString &model);
    void handle_ttt_models_changed(const QVariantMap &models);
    void handle_mnt_translate_progress(double new_progress, int task);
    void handle_mnt_translate_finished(const QString &in_text,
                                       const QString &in_lang,
                                       const QString &out_text,
                                       const QString &out_lang, int task);
    void handle_mc_state_changed();
    void handle_mc_progress_changed();
    void connect_service_signals();
    void start_keepalive();
    QVariantMap translations() const;
    static std::pair<QString, int> insert_to_note(QString note,
                                                  QString new_text,
                                                  const QString &lang,
                                                  settings::insert_mode_t mode,
                                                  int last_cursor_position);
    QString note() const;
    void set_note(const QString &text);
    bool can_undo_note() const;
    bool can_redo_note() const;
    bool can_undo_or_redu_note() const;
    QString translated_text() const;
    void set_translated_text(const QString text);
    void listen_internal(stt_translate_req_t translate_req);
    void speech_to_file_internal(QString text, const QString &model_id,
                                 const QString &dest_file,
                                 const QString &title_tag, const QString &track,
                                 const QString &ref_voice,
                                 settings::text_format_t text_format,
                                 settings::audio_format_t audio_format,
                                 settings::audio_quality_t audio_quality);
    void play_speech_internal(QString text, const QString &model_id,
                              const QString &ref_voice,
                              settings::text_format_t text_format);
    void export_to_file_internal(const QString &text, const QString &dest_file);
    void copy_to_clipboard_internal(const QString &text);
    void handle_translate_delayed();
    void transcribe_file(const QString &file_path, int stream_index,
                         bool replace);
    file_import_result_t import_file_internal(const QString &file_path,
                                              int stream_index, bool replace);
    void open_next_file();
    void reset_files_queue();
    void register_hotkeys();
    void execute_action(action_t action, const QString &extra);
    void execute_pending_action();
    void process_pending_desktop_notification();
    void handle_desktop_notification_closed(uint id, uint reason);
    void handle_desktop_notification_action_invoked(uint id,
                                                    const QString &action_key);
    bool feature_available(const QString &name, bool default_value) const;

#define FEATURE_OPT(name) bool feature_##name() const;
    FEATURE_OPT(whispercpp_stt)
    FEATURE_OPT(whispercpp_gpu)
    FEATURE_OPT(fasterwhisper_stt)
    FEATURE_OPT(fasterwhisper_gpu)
    FEATURE_OPT(whisperspeech_tts)
    FEATURE_OPT(whisperspeech_gpu)
    FEATURE_OPT(coqui_tts)
    FEATURE_OPT(coqui_gpu)
    FEATURE_OPT(punctuator)
    FEATURE_OPT(diacritizer_he)
    FEATURE_OPT(global_shortcuts)
    FEATURE_OPT(text_active_window)
    FEATURE_OPT(translator)
#undef FEATURE_OPT

    void request_reload();
    bool stt_translate_needed_by_id(const QString &id) const;
    bool tts_ref_voice_needed_by_id(const QString &id) const;
    bool stt_translate_needed() const;
    bool tts_ref_voice_needed() const;
    bool tts_for_in_mnt_ref_voice_needed() const;
    bool tts_for_out_mnt_ref_voice_needed() const;
    static QString cache_dir();
    static QString import_ref_voice_file_path();
    bool player_ready() const;
    bool player_playing() const;
    void create_player();
    long long player_position() const;
    long long player_duration() const;
    void create_mic_recorder();
    bool recorder_recording() const;
    long long recorder_duration() const;
    bool recorder_processing() const;
    QString tts_ref_voice_unique_name(QString name, bool add_numner) const;
    void update_tray_state();
    void update_tray_task_state();
    void update_auto_text_format_delayed();
    void update_auto_text_format();
    void translate_internal(const QString &text);
    bool import_text_file(const QString &input_file, bool replace);
    void export_to_subtitles(const QString &dest_file,
                             settings::text_file_format_t format,
                             const QString &text);
    void export_to_audio_mix_internal(const QString &main_input_file,
                                      int main_stream_index,
                                      const QStringList &input_files,
                                      const QString &dest_file);
    void export_to_audio_internal(const QStringList &input_files,
                                  const QString &dest_file);
    void player_import_rec();
    void player_set_path(const QString &wav_file_path);
    void player_import_from_rec_path(const QString &path);
    static QStringList make_streams_names(
        const std::vector<media_compressor::stream_t> &streams);
    QString tts_ref_voice_auto_name() const;
    int player_current_voice_ref_idx() const {
        return m_player_current_voice_ref_idx;
    }
    void repair_text(text_repair_task_type_t task_type);
    void switch_mnt_langs();
    QString stt_auto_lang_id() const { return m_stt_auto_lang_id; }
    void update_stt_auto_lang(QString lang_id);
    QString stt_auto_lang_name() const;
    void set_last_cursor_position(int position);
    int last_cursor_position() const { return m_last_cursor_position; }
#ifdef USE_DESKTOP
    void execute_tray_action(tray_icon::action_t action, int value);
#endif
    enum class transform_text_target_t { tts, stt };
    static trans_rule_result_t transform_text(QString &text,
                                              transform_text_target_t target,
                                              const QString &lang);
    static settings::trans_rule_flags_t apply_trans_rule(
        QString &text, const trans_rule_t &rule);
    QString trans_rules_test_text() const;
    void set_trans_rules_test_text(const QString &text);
    static QString lang_from_model_id(const QString &model_id);
};

#endif  // DSNOTE_APP_H

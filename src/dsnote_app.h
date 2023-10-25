/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
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

#ifdef USE_DESKTOP
#include <qhotkey.h>

#include "fake_keyboard.hpp"
#endif

#include "config.h"
#include "dbus_notifications_inf.h"
#include "dbus_speech_inf.h"
#include "settings.h"

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

    // stt
    Q_PROPERTY(int active_stt_model_idx READ active_stt_model_idx NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString active_stt_model READ active_stt_model NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString active_stt_model_name READ active_stt_model_name NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QVariantList available_stt_models READ available_stt_models
                   NOTIFY available_stt_models_changed)

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

    Q_PROPERTY(service_task_state_t task_state READ task_state NOTIFY
                   task_state_changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
    Q_PROPERTY(
        bool stt_configured READ stt_configured NOTIFY stt_configured_changed)
    Q_PROPERTY(
        bool tts_configured READ tts_configured NOTIFY tts_configured_changed)
    Q_PROPERTY(
        bool ttt_configured READ ttt_configured NOTIFY ttt_configured_changed)
    Q_PROPERTY(
        bool mnt_configured READ mnt_configured NOTIFY mnt_configured_changed)
    Q_PROPERTY(bool connected READ connected NOTIFY connected_changed)
    Q_PROPERTY(bool another_app_connected READ another_app_connected NOTIFY
                   another_app_connected_changed)
    Q_PROPERTY(double transcribe_progress READ transcribe_progress NOTIFY
                   transcribe_progress_changed)
    Q_PROPERTY(double speech_to_file_progress READ speech_to_file_progress
                   NOTIFY speech_to_file_progress_changed)
    Q_PROPERTY(
        service_state_t state READ service_state NOTIFY service_state_changed)
    Q_PROPERTY(
        QVariantMap translations READ translations NOTIFY connected_changed)

    // features
    Q_PROPERTY(
        bool feature_gpu_stt READ feature_gpu_stt NOTIFY features_changed)
    Q_PROPERTY(
        bool feature_gpu_tts READ feature_gpu_tts NOTIFY features_changed)
    Q_PROPERTY(
        bool feature_punctuator READ feature_punctuator NOTIFY features_changed)
    Q_PROPERTY(bool feature_diacritizer_he READ feature_diacritizer_he NOTIFY
                   features_changed)
    Q_PROPERTY(bool feature_global_shortcuts READ feature_global_shortcuts
                   NOTIFY features_changed)
    Q_PROPERTY(bool feature_text_active_window READ feature_text_active_window
                   NOTIFY features_changed)

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
        StateTranslating = 10
    };
    Q_ENUM(service_state_t)
    friend QDebug operator<<(QDebug d, service_state_t state);

    enum service_task_state_t {
        TaskStateIdle = 0,
        TaskStateSpeechDetected = 1,
        TaskStateProcessing = 2,
        TaskStateInitializing = 3,
        TaskStateSpeechPlaying = 4,
        TaskStateSpeechPaused = 5
    };
    Q_ENUM(service_task_state_t)
    friend QDebug operator<<(QDebug d, service_task_state_t type);

    enum error_t {
        ErrorGeneric = 0,
        ErrorMicSource = 1,
        ErrorFileSource = 2,
        ErrorSttEngine = 3,
        ErrorTtsEngine = 4,
        ErrorMntEngine = 5,
        ErrorSaveNoteToFile = 6,
        ErrorLoadNoteFromFile = 7,
        ErrorNoService = 100
    };
    Q_ENUM(error_t)
    friend QDebug operator<<(QDebug d, error_t type);

    dsnote_app(QObject *parent = nullptr);
    Q_INVOKABLE void set_active_stt_model_idx(int idx);
    Q_INVOKABLE void set_active_tts_model_idx(int idx);
    Q_INVOKABLE void set_active_mnt_lang_idx(int idx);
    Q_INVOKABLE void set_active_mnt_out_lang_idx(int idx);
    Q_INVOKABLE void set_active_tts_model_for_in_mnt_idx(int idx);
    Q_INVOKABLE void set_active_tts_model_for_out_mnt_idx(int idx);
    Q_INVOKABLE void transcribe_file(const QString &source_file, bool replace);
    Q_INVOKABLE void transcribe_file(const QUrl &source_file, bool replace);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void listen();
    Q_INVOKABLE void listen_to_active_window();
    Q_INVOKABLE void listen_to_clipboard();
    Q_INVOKABLE void stop_listen();
    Q_INVOKABLE void play_speech();
    Q_INVOKABLE void play_speech_from_clipboard();
    Q_INVOKABLE void play_speech_translator(bool transtalated);
    Q_INVOKABLE void pause_speech();
    Q_INVOKABLE void resume_speech();
    Q_INVOKABLE void translate();
    Q_INVOKABLE void translate_delayed();
    Q_INVOKABLE void speech_to_file(const QString &dest_file,
                                    const QString &title_tag = {},
                                    const QString &track_tag = {});
    Q_INVOKABLE void speech_to_file(const QUrl &dest_file,
                                    const QString &title_tag = {},
                                    const QString &track_tag = {});
    Q_INVOKABLE void speech_to_file_translator(bool transtalated,
                                               const QString &dest_file,
                                               const QString &title_tag = {},
                                               const QString &track_tag = {});
    Q_INVOKABLE void speech_to_file_translator(bool transtalated,
                                               const QUrl &dest_file,
                                               const QString &title_tag = {},
                                               const QString &track_tag = {});
    Q_INVOKABLE void save_note_to_file(const QString &dest_file);
    Q_INVOKABLE void save_note_to_file_translator(const QString &dest_file);
    Q_INVOKABLE bool load_note_from_file(const QString &input_file,
                                         bool replace);
    Q_INVOKABLE void open_files(const QStringList &input_files, bool replace);
    Q_INVOKABLE void open_files_url(const QList<QUrl> &input_urls,
                                    bool replace);
    Q_INVOKABLE void stop_play_speech();
    Q_INVOKABLE void copy_to_clipboard();
    Q_INVOKABLE void copy_translation_to_clipboard();
    Q_INVOKABLE bool file_exists(const QString &file) const;
    Q_INVOKABLE bool file_exists(const QUrl &file) const;
    Q_INVOKABLE void undo_or_redu_note();
    Q_INVOKABLE void make_undo();
    Q_INVOKABLE void update_note(const QString &text, bool replace);
    Q_INVOKABLE void close_desktop_notification();
    Q_INVOKABLE void show_desktop_notification(const QString &summary,
                                               const QString &body,
                                               bool permanent = false);
    Q_INVOKABLE void execute_action_name(const QString &action_name);
    Q_INVOKABLE QVariantList features_availability();

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
    void ttt_configured_changed();
    void mnt_configured_changed();
    void connected_changed();
    void another_app_connected_changed();
    void transcribe_progress_changed();
    void speech_to_file_progress_changed();
    void error(dsnote_app::error_t type);
    void transcribe_done();
    void speech_to_file_done();
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

   private:
    enum class action_t {
        start_listening,
        start_listening_active_window,
        start_listening_clipboard,
        stop_listening,
        start_reading,
        start_reading_clipboard,
        pause_resume_reading,
        cancel
    };
    friend QDebug operator<<(QDebug d, action_t type);

    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_SPEECH_SERVICE)};
    inline static const QString DBUS_SERVICE_PATH{QStringLiteral("/")};
    static const int SUCCESS = 0;
    static const int FAILURE = -1;
    static const int INVALID_TASK = -1;
    static const int KEEPALIVE_TIME = 1000;  // 1s
    static const int MAX_INIT_ATTEMPTS = 120;

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

    enum class stt_text_destination_t {
        note_add,
        note_replace,
        active_window,
        clipboard
    };

    QString m_active_stt_model;
    QVariantMap m_available_stt_models_map;
    QString m_active_tts_model;
    QVariantMap m_available_tts_models_map;
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
    double m_transcribe_progress = -1.0;
    OrgMkiolSpeechInterface m_dbus_service;
    OrgFreedesktopNotificationsInterface m_dbus_notifications;
    service_state_t m_service_state = service_state_t::StateUnknown;
    service_task_state_t m_task_state = service_task_state_t::TaskStateIdle;
    task_t m_primary_task;
    task_t m_side_task;
    int m_current_task = INVALID_TASK;
    int m_init_attempts = -1;
    QTimer m_keepalive_timer;
    QTimer m_keepalive_current_task_timer;
    QTimer m_translator_delay_timer;
    QTimer m_open_files_delay_timer;
    QTimer m_action_delay_timer;
    QTimer m_desktop_notification_delay_timer;
    bool m_stt_configured = false;
    bool m_tts_configured = false;
    bool m_ttt_configured = false;
    bool m_mnt_configured = false;
    QString m_dest_file;
    QString m_dest_file_title_tag;
    QString m_dest_file_track_tag;
    QString m_translated_text;
    QString m_prev_text;
    bool m_undo_flag = false;  // true => undo, false => redu
    std::queue<QString> m_files_to_open;
    std::optional<action_t> m_pending_action;
    stt_text_destination_t m_stt_text_destination =
        stt_text_destination_t::note_add;
    std::optional<desktop_notification_t> m_desktop_notification;
    QVariantMap m_features_availability;
#ifdef USE_DESKTOP
    struct hotkeys_t {
        QHotkey start_listening;
        QHotkey start_listening_active_window;
        QHotkey start_listening_clipboard;
        QHotkey stop_listening;
        QHotkey start_reading;
        QHotkey start_reading_clipboard;
        QHotkey pause_resume_reading;
        QHotkey cancel;
    };

    hotkeys_t m_hotkeys;
    std::optional<fake_keyboard> m_fake_keyboard;
#endif

    [[nodiscard]] QVariantList available_stt_models() const;
    [[nodiscard]] QVariantList available_tts_models() const;
    [[nodiscard]] QVariantList available_mnt_langs() const;
    [[nodiscard]] QVariantList available_mnt_out_langs() const;
    [[nodiscard]] QVariantList available_tts_models_for_in_mnt() const;
    [[nodiscard]] QVariantList available_tts_models_for_out_mnt() const;
    void handle_models_changed();
    void handle_stt_text_decoded(const QString &text, const QString &lang,
                                 int task);
    void handle_tts_partial_speech(const QString &text, int task);
    bool busy() const;
    void update_configured_state();
    bool stt_configured() const;
    bool tts_configured() const;
    bool ttt_configured() const;
    bool mnt_configured() const;
    bool connected() const;
    double transcribe_progress() const;
    double speech_to_file_progress() const;
    bool another_app_connected() const;
    void update_progress();
    void update_service_state();
    void update_task_state();
    void update_active_stt_model();
    void update_available_stt_models();
    void update_active_tts_model();
    void update_available_tts_models();
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
    void set_task_state(service_task_state_t speech);
    void handle_stt_intermediate_text(const QString &text, const QString &lang,
                                      int task);
    [[nodiscard]] int active_stt_model_idx() const;
    inline QString active_stt_model() { return m_active_stt_model; }
    QString active_stt_model_name() const;
    [[nodiscard]] int active_tts_model_idx() const;
    inline QString active_tts_model() { return m_active_tts_model; }
    QString active_tts_model_name() const;
    inline QString active_mnt_lang() { return m_active_mnt_lang; }
    QString active_mnt_lang_name() const;
    [[nodiscard]] int active_mnt_lang_idx() const;
    inline QString active_mnt_out_lang() { return m_active_mnt_out_lang; }
    QString active_mnt_out_lang_name() const;
    [[nodiscard]] int active_mnt_out_lang_idx() const;
    [[nodiscard]] int active_tts_model_for_in_mnt_idx() const;
    inline QString active_tts_model_for_in_mnt() {
        return m_active_tts_model_for_in_mnt;
    }
    QString active_tts_model_for_in_mnt_name() const;
    [[nodiscard]] int active_tts_model_for_out_mnt_idx() const;
    inline QString active_tts_model_for_out_mnt() {
        return m_active_tts_model_for_out_mnt;
    }
    QString active_tts_model_for_out_mnt_name() const;
    inline QString intermediate_text() const { return m_intermediate_text; }
    void update_active_stt_lang_idx();
    void update_active_tts_lang_idx();
    void update_active_mnt_lang_idx();
    void update_active_mnt_out_lang_idx();
    inline service_state_t service_state() const { return m_service_state; }
    inline auto task_state() const { return m_task_state; }
    void do_keepalive();
    void handle_keepalive_task_timeout();
    void handle_service_error(int code);
    void handle_stt_file_transcribe_finished(int task);
    void handle_stt_file_transcribe_progress(double new_progress, int task);
    void handle_tts_play_speech_finished(int task);
    void handle_tts_speech_to_file_finished(const QString &file, int task);
    void handle_tts_speech_to_file_progress(double new_progress, int task);
    void handle_stt_default_model_changed(const QString &model);
    void set_active_stt_model(const QString &model);
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
    void handle_mnt_translate_finished(const QString &in_text,
                                       const QString &in_lang,
                                       const QString &out_text,
                                       const QString &out_lang, int task);
    void connect_service_signals();
    void start_keepalive();
    void check_transcribe_taks();
    QVariantMap translations() const;
    static QString insert_to_note(QString note, QString new_text,
                                  const QString &lang,
                                  settings::insert_mode_t mode);
    QString note() const;
    void set_note(const QString text);
    bool can_undo_note() const;
    bool can_redo_note() const;
    bool can_undo_or_redu_note() const;
    QString translated_text() const;
    void set_translated_text(const QString text);
    void listen_internal();
    void speech_to_file_internal(const QString &text, const QString &model_id,
                                 const QString &dest_file,
                                 const QString &title_tag,
                                 const QString &track);
    void play_speech_internal(const QString &text, const QString &model_id);
    void save_note_to_file_internal(const QString &text,
                                    const QString &dest_file);
    void copy_to_clipboard_internal(const QString &text);
    void handle_translate_delayed();
    void open_next_file();
    void reset_files_queue();
    void register_hotkeys();
    void execute_action(action_t action);
    void execute_pending_action();
    void process_pending_desktop_notification();
    void handle_desktop_notification_closed(uint id, uint reason);
    void handle_desktop_notification_action_invoked(uint id,
                                                    const QString &action_key);
    bool feature_available(const QString &name) const;
    bool feature_gpu_stt() const;
    bool feature_gpu_tts() const;
    bool feature_punctuator() const;
    bool feature_diacritizer_he() const;
    bool feature_global_shortcuts() const;
    bool feature_text_active_window() const;
};

#endif  // DSNOTE_APP_H

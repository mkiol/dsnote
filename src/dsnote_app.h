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

#include "config.h"
#include "dbus_speech_inf.h"
#include "settings.h"

class dsnote_app : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString intermediate_text READ intermediate_text NOTIFY
                   intermediate_text_changed)

    Q_PROPERTY(int active_stt_model_idx READ active_stt_model_idx NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString active_stt_model READ active_stt_model NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString active_stt_model_name READ active_stt_model_name NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QVariantList available_stt_models READ available_stt_models
                   NOTIFY available_stt_models_changed)

    Q_PROPERTY(int active_tts_model_idx READ active_tts_model_idx NOTIFY
                   active_tts_model_changed)
    Q_PROPERTY(QString active_tts_model READ active_tts_model NOTIFY
                   active_tts_model_changed)
    Q_PROPERTY(QString active_tts_model_name READ active_tts_model_name NOTIFY
                   active_tts_model_changed)
    Q_PROPERTY(QVariantList available_tts_models READ available_tts_models
                   NOTIFY available_tts_models_changed)

    Q_PROPERTY(service_speech_state_t speech READ speech NOTIFY speech_changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
    Q_PROPERTY(
        bool stt_configured READ stt_configured NOTIFY stt_configured_changed)
    Q_PROPERTY(
        bool tts_configured READ tts_configured NOTIFY tts_configured_changed)
    Q_PROPERTY(
        bool ttt_configured READ ttt_configured NOTIFY ttt_configured_changed)
    Q_PROPERTY(bool connected READ connected NOTIFY connected_changed)
    Q_PROPERTY(bool another_app_connected READ another_app_connected NOTIFY
                   another_app_connected_changed)
    Q_PROPERTY(double transcribe_progress READ transcribe_progress NOTIFY
                   transcribe_progress_changed)
    Q_PROPERTY(double speech_to_file_progress READ speech_to_file_progress
                   NOTIFY speech_to_file_progress_changed)
    Q_PROPERTY(
        service_state_t state READ service_state NOTIFY service_state_changed)
    Q_PROPERTY(QVariantMap translate READ translate NOTIFY connected_changed)

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
        StateWritingSpeechToFile = 9
    };
    Q_ENUM(service_state_t)
    friend QDebug operator<<(QDebug d, service_state_t state);

    enum service_speech_state_t {
        SpeechStateNoSpeech = 0,
        SpeechStateSpeechDetected = 1,
        SpeechStateSpeechDecodingEncoding = 2,
        SpeechStateSpeechInitializing = 3,
        SpeechStateSpeechPlaying = 4
    };
    Q_ENUM(service_speech_state_t)
    friend QDebug operator<<(QDebug d, service_speech_state_t type);

    enum error_t {
        ErrorGeneric = 0,
        ErrorMicSource = 1,
        ErrorFileSource = 2,
        ErrorNoService = 3
    };
    Q_ENUM(error_t)
    friend QDebug operator<<(QDebug d, error_t type);

    dsnote_app(QObject *parent = nullptr);
    Q_INVOKABLE void set_active_stt_model_idx(int idx);
    Q_INVOKABLE void set_active_tts_model_idx(int idx);
    Q_INVOKABLE void transcribe_file(const QString &source_file);
    Q_INVOKABLE void transcribe_file(const QUrl &source_file);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void listen();
    Q_INVOKABLE void stop_listen();
    Q_INVOKABLE void play_speech();
    Q_INVOKABLE void speech_to_file(const QString &dest_file);
    Q_INVOKABLE void speech_to_file(const QUrl &dest_file);
    Q_INVOKABLE void stop_play_speech();
    Q_INVOKABLE void copy_to_clipboard();
    Q_INVOKABLE bool file_exists(const QString &file) const;
    Q_INVOKABLE bool file_exists(const QUrl &file) const;

   signals:
    void active_stt_model_changed();
    void available_stt_models_changed();
    void active_tts_model_changed();
    void available_tts_models_changed();
    void available_ttt_models_changed();
    void intermediate_text_changed();
    void text_changed();
    void speech_changed();
    void busy_changed();
    void stt_configured_changed();
    void tts_configured_changed();
    void ttt_configured_changed();
    void connected_changed();
    void another_app_connected_changed();
    void transcribe_progress_changed();
    void speech_to_file_progress_changed();
    void error(dsnote_app::error_t type);
    void transcribe_done();
    void speech_to_file_done();
    void service_state_changed();
    void note_copied();

   private:
    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral(APP_DBUS_SERVICE)};
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

    QString m_active_stt_model;
    QVariantMap m_available_stt_models_map;
    QString m_active_tts_model;
    QVariantMap m_available_tts_models_map;
    QVariantMap m_available_ttt_models_map;
    QString m_intermediate_text;
    double m_speech_to_file_progress = -1.0;
    double m_transcribe_progress = -1.0;
    OrgMkiolSpeechInterface m_dbus_service;
    service_state_t m_service_state = service_state_t::StateUnknown;
    service_speech_state_t m_speech_state =
        service_speech_state_t::SpeechStateNoSpeech;
    task_t m_primary_task;
    task_t m_side_task;
    int m_current_task = INVALID_TASK;
    int m_init_attempts = -1;
    QTimer m_keepalive_timer;
    QTimer m_keepalive_current_task_timer;
    bool m_stt_configured = false;
    bool m_tts_configured = false;
    bool m_ttt_configured = false;
    QString m_dest_file;

    [[nodiscard]] QVariantList available_stt_models() const;
    [[nodiscard]] QVariantList available_tts_models() const;
    void handle_models_changed();
    void handle_stt_text_decoded(const QString &text, const QString &lang,
                                 int task);
    void handle_tts_partial_speech(const QString &text, int task);
    bool busy() const;
    void update_configured_state();
    bool stt_configured() const;
    bool tts_configured() const;
    bool ttt_configured() const;
    bool connected() const;
    double transcribe_progress() const;
    double speech_to_file_progress() const;
    bool another_app_connected() const;
    void update_progress();
    void update_service_state();
    void update_speech();
    void update_active_stt_model();
    void update_available_stt_models();
    void update_active_tts_model();
    void update_available_tts_models();
    void update_current_task();
    void update_listen();
    void set_speech(service_speech_state_t speech);
    void handle_stt_intermediate_text(const QString &text, const QString &lang,
                                      int task);
    [[nodiscard]] int active_stt_model_idx() const;
    inline QString active_stt_model() { return m_active_stt_model; }
    QString active_stt_model_name() const;
    [[nodiscard]] int active_tts_model_idx() const;
    inline QString active_tts_model() { return m_active_tts_model; }
    QString active_tts_model_name() const;
    inline QString intermediate_text() const { return m_intermediate_text; }
    void update_active_stt_lang_idx();
    void update_active_tts_lang_idx();
    inline service_state_t service_state() const { return m_service_state; }
    inline auto speech() const { return m_speech_state; }
    void do_keepalive();
    void handle_keepalive_task_timeout();
    void handle_service_error(int code);
    void handle_stt_file_transcribe_finished(int task);
    void handle_stt_file_transcribe_progress(double new_progress, int task);
    void handle_tts_play_speech_finished(int task);
    void handle_tts_speech_to_file_finished(const QString &file, int task);
    void handle_tts_speech_to_file_progress(double new_progress, int task);
    void handle_stt_default_model_changed(const QString &model);
    void handle_tts_default_model_changed(const QString &model);
    void handle_current_task_changed(int task);
    void handle_speech_changed(int speech);
    void handle_state_changed(int status);
    void handle_stt_models_changed(const QVariantMap &models);
    void handle_tts_models_changed(const QVariantMap &models);
    void handle_ttt_models_changed(const QVariantMap &models);
    void connect_service_signals();
    void start_keepalive();
    void check_transcribe_taks();
    QVariantMap translate() const;
    static QString insert_to_note(QString note, QString new_text,
                                  settings::insert_mode_t mode);
};

#endif  // DSNOTE_APP_H

/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DSNOTE_APP_H
#define DSNOTE_APP_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "dbus_stt_inf.h"
#include "settings.h"

class dsnote_app : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString intermediate_text READ intermediate_text NOTIFY
                   intermediate_text_changed)

    Q_PROPERTY(int active_stt_model_idx READ active_stt_model_idx NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QString active_stt_model READ active_stt_model NOTIFY
                   active_stt_model_changed)
    Q_PROPERTY(QVariantList available_stt_models READ available_stt_models
                   NOTIFY available_stt_models_changed)

    Q_PROPERTY(stt_speech_state_t speech READ speech NOTIFY speech_changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
    Q_PROPERTY(bool configured READ configured NOTIFY configured_changed)
    Q_PROPERTY(bool connected READ connected NOTIFY connected_changed)
    Q_PROPERTY(bool another_app_connected READ another_app_connected NOTIFY
                   another_app_connected_changed)
    Q_PROPERTY(double transcribe_progress READ transcribe_progress NOTIFY
                   transcribe_progress_changed)
    Q_PROPERTY(stt_state_t state READ stt_state NOTIFY stt_state_changed)
    Q_PROPERTY(QVariantMap translate READ translate NOTIFY connected_changed)

   public:
    enum stt_state_t {
        SttUnknown = 0,
        SttNotConfigured = 1,
        SttBusy = 2,
        SttIdle = 3,
        SttListeningManual = 4,
        SttListeningAuto = 5,
        SttTranscribingFile = 6,
        SttListeningSingleSentence = 7
    };
    Q_ENUM(stt_state_t)
    friend QDebug operator<<(QDebug d, stt_state_t state);

    enum stt_speech_state_t {
        SttNoSpeech = 0,
        SttSpeechDetected = 1,
        SttSpeechDecoding = 2,
        SttSpeechInitializing = 3
    };
    Q_ENUM(stt_speech_state_t)
    friend QDebug operator<<(QDebug d, stt_speech_state_t type);

    enum error_t {
        ErrorGeneric = 0,
        ErrorMicSource = 1,
        ErrorFileSource = 2,
        ErrorNoSttService = 3
    };
    Q_ENUM(error_t)
    friend QDebug operator<<(QDebug d, error_t type);

    dsnote_app(QObject *parent = nullptr);
    Q_INVOKABLE void set_active_stt_model_idx(int idx);
    Q_INVOKABLE void transcribe_file(const QString &source_file);
    Q_INVOKABLE void transcribe_file(const QUrl &source_file);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void listen();
    Q_INVOKABLE void stop_listen();

   signals:
    void active_stt_model_changed();
    void available_stt_models_changed();
    void intermediate_text_changed();
    void text_changed();
    void speech_changed();
    void busy_changed();
    void configured_changed();
    void connected_changed();
    void another_app_connected_changed();
    void transcribe_progress_changed();
    void error(dsnote_app::error_t type);
    void transcribe_done();
    void stt_state_changed();

   private:
    inline static const QString DBUS_SERVICE_NAME{
        QStringLiteral("org.mkiol.Stt")};
    inline static const QString DBUS_SERVICE_PATH{QStringLiteral("/")};
    static const int SUCCESS = 0;
    static const int FAILURE = -1;
    static const int INVALID_TASK = -1;
    static const int KEEPALIVE_TIME = 1000;  // 1s
    static const int MAX_INIT_ATTEMPTS = 5;

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
    QString m_intermediate_text;
    double m_transcribe_progress = -1.0;
    OrgMkiolSttInterface m_stt;
    stt_state_t m_stt_state = stt_state_t::SttUnknown;
    stt_speech_state_t m_speech_state = stt_speech_state_t::SttNoSpeech;
    task_t m_listen_task;
    task_t m_transcribe_task;
    int m_current_task = INVALID_TASK;
    int m_init_attempts = -1;
    QTimer m_keepalive_timer;
    QTimer m_keepalive_current_task_timer;

    [[nodiscard]] QVariantList available_stt_models() const;
    void handle_models_changed();
    void handle_text_decoded(const QString &text, const QString &lang,
                             int task);
    inline bool busy() const {
        return m_stt_state == stt_state_t::SttBusy || another_app_connected();
    }
    inline bool configured() const {
        return m_stt_state != stt_state_t::SttUnknown &&
               m_stt_state != stt_state_t::SttNotConfigured;
    }
    inline bool connected() const {
        return m_stt_state != stt_state_t::SttUnknown;
    }
    inline double transcribe_progress() const { return m_transcribe_progress; }
    inline bool another_app_connected() const {
        return m_current_task != INVALID_TASK &&
               m_listen_task != m_current_task &&
               m_transcribe_task != m_current_task;
    }
    void update_progress();
    void update_stt_state();
    void update_speech();
    void update_active_stt_model();
    void update_available_stt_models();
    void update_listen();
    void update_current_task();
    void set_speech(stt_speech_state_t speech);
    void set_intermediate_text(const QString &text, const QString &lang,
                               int task);
    [[nodiscard]] int active_stt_model_idx() const;
    inline QString active_stt_model() { return m_active_stt_model; }
    inline QString intermediate_text() const { return m_intermediate_text; }
    void update_active_lang_idx();
    inline stt_state_t stt_state() const { return m_stt_state; }
    inline auto speech() const { return m_speech_state; }
    void do_keepalive();
    void handle_keepalive_task_timeout();
    void handle_stt_error(int code);
    void handle_stt_file_transcribe_finished(int task);
    void handle_stt_file_transcribe_progress(double new_progress, int task);
    void handle_stt_default_model_changed(const QString &model);
    void handle_stt_current_task_changed(int task);
    void handle_stt_speech_changed(int speech);
    void handle_stt_state_changed(int status);
    void handle_stt_models_changed(const QVariantMap &models);
    void connect_dbus_signals();
    void start_keepalive();
    void check_transcribe_taks();
    QVariantMap translate() const;
    static QString insert_to_note(QString note, QString new_text,
                                  settings::insert_mode_t mode);
};

#endif  // DSNOTE_APP_H

/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
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

class dsnote_app : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString intermediate_text READ intermediate_text NOTIFY
                   intermediate_text_changed)
    Q_PROPERTY(
        int active_lang_idx READ active_lang_idx NOTIFY active_lang_changed)
    Q_PROPERTY(QString active_lang READ active_lang NOTIFY active_lang_changed)
    Q_PROPERTY(QVariantList available_langs READ available_langs NOTIFY
                   available_langs_changed)
    Q_PROPERTY(bool speech READ speech NOTIFY speech_changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
    Q_PROPERTY(bool configured READ configured NOTIFY configured_changed)
    Q_PROPERTY(bool connected READ connected NOTIFY connected_changed)
    Q_PROPERTY(bool another_app_connected READ another_app_connected NOTIFY
                   another_app_connected_changed)
    Q_PROPERTY(double transcribe_progress READ transcribe_progress NOTIFY
                   transcribe_progress_changed)
    Q_PROPERTY(stt_state_type state READ stt_state NOTIFY stt_state_changed)
    Q_PROPERTY(QVariantMap translate READ translate NOTIFY connected_changed)

   public:
    enum stt_state_type {
        SttUnknown = 0,
        SttNotConfigured = 1,
        SttBusy = 2,
        SttIdle = 3,
        SttListeningManual = 4,
        SttListeningAuto = 5,
        SttTranscribingFile = 6,
        SttListeningSingleSentence = 7
    };
    Q_ENUM(stt_state_type)

    enum error_type {
        ErrorGeneric = 0,
        ErrorMicSource = 1,
        ErrorFileSource = 2,
        ErrorNoSttService = 3
    };
    Q_ENUM(error_type)

    dsnote_app(QObject *parent = nullptr);
    Q_INVOKABLE void set_active_lang_idx(int idx);
    Q_INVOKABLE void transcribe_file(const QString &source_file);
    Q_INVOKABLE void transcribe_file(const QUrl &source_file);
    Q_INVOKABLE void cancel_transcribe();
    Q_INVOKABLE void listen();
    Q_INVOKABLE void stop_listen();

   signals:
    void active_lang_changed();
    void available_langs_changed();
    void intermediate_text_changed();
    void text_changed();
    void speech_changed();
    void busy_changed();
    void configured_changed();
    void connected_changed();
    void another_app_connected_changed();
    void transcribe_progress_changed();
    void error(dsnote_app::error_type type);
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

    struct task_type {
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

    QString active_lang_value;
    QVariantMap available_langs_map;
    QString intermediate_text_value;
    double transcribe_progress_value = -1.0;
    OrgMkiolSttInterface stt;
    stt_state_type stt_state_value = stt_state_type::SttUnknown;
    bool speech_value = false;
    task_type listen_task;
    task_type transcribe_task;
    int current_task_value = INVALID_TASK;
    int init_attempts = -1;
    QTimer keepalive_timer;
    QTimer keepalive_current_task_timer;

    [[nodiscard]] QVariantList available_langs() const;
    void handle_models_changed();
    void handle_text_decoded(const QString &text, const QString &lang,
                             int task);
    [[nodiscard]] inline bool busy() const {
        return stt_state_value == stt_state_type::SttBusy ||
               another_app_connected();
    }
    [[nodiscard]] inline bool configured() const {
        return stt_state_value != stt_state_type::SttUnknown &&
               stt_state_value != stt_state_type::SttNotConfigured;
    }
    [[nodiscard]] inline bool connected() const {
        return stt_state_value != stt_state_type::SttUnknown;
    }
    [[nodiscard]] inline double transcribe_progress() const {
        return transcribe_progress_value;
    }
    [[nodiscard]] inline bool another_app_connected() const {
        return current_task_value != INVALID_TASK &&
               listen_task != current_task_value &&
               transcribe_task != current_task_value;
    }
    void update_progress();
    void update_stt_state();
    void update_speech();
    void update_active_lang();
    void update_available_langs();
    void update_listen();
    void update_current_task();
    void set_intermediate_text(const QString &text, const QString &lang,
                               int task);
    [[nodiscard]] int active_lang_idx() const;
    [[nodiscard]] inline QString active_lang() { return active_lang_value; }
    [[nodiscard]] inline QString intermediate_text() const {
        return intermediate_text_value;
    }
    void update_active_lang_idx();
    [[nodiscard]] inline stt_state_type stt_state() const {
        return stt_state_value;
    }
    [[nodiscard]] inline bool speech() const { return speech_value; }
    void do_keepalive();
    void handle_keepalive_task_timeout();
    void connect_dbus_signals();
    void start_keepalive();
    void check_transcribe_taks();
    QVariantMap translate() const;
};

#endif  // DSNOTE_APP_H

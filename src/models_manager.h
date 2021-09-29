/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MODELS_MANAGER_H
#define MODELS_MANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QJsonArray>

#include <map>
#include <vector>
#include <tuple>
#include <atomic>
#include <thread>

class models_manager : public QObject
{
    Q_OBJECT
    Q_PROPERTY (bool busy READ busy NOTIFY busy_changed)
public:
    struct lang_t {
        QString id;
        QString lang_id;
        QString name;
        QString model_file;
        QString scorer_file;
        bool available = false;
        bool downloading = false;
        double download_progress = 0.0;
    };

    models_manager(QObject *parent = nullptr);
    ~models_manager();
    [[nodiscard]] bool ok() const;
    std::vector<lang_t> available_langs() const;
    std::vector<lang_t> langs() const;
    [[nodiscard]] bool model_exists(const QString& id) const;
    void download_model(const QString& id);
    void delete_model(const QString& id);
    [[nodiscard]] inline bool busy() const { return busy_value; }

signals:
    void download_progress(const QString& id, double progress);
    void download_finished(const QString& id);
    void download_error(const QString& id);
    void models_changed();
    void busy_changed();

private:
    enum class download_type { none, all, model, scorer };
    enum class comp_type { none, xz, gz };

    struct model_t {
        QString lang_id;
        QString name;
        QString file_name;
        QString md5;
        comp_type comp = comp_type::none;
        std::vector<QUrl> urls;
        qint64 size = 0;
        QString scorer_file_name;
        QString scorer_md5;
        comp_type scorer_comp = comp_type::none;
        std::vector<QUrl> scorer_urls;
        qint64 scorer_size = 0;
        bool available = false;
        download_type current_dl = download_type::none;
        double download_progress = 0.0;
        qint64 downloaded_part_data = 0;
    };

    static const QString lang_models_file;

    std::map<QString, model_t> models;
    QNetworkAccessManager nam;
    std::atomic_bool busy_value = false;
    std::thread thread;

    void parse_models_file();
    static std::map<QString, model_t> parse_models_file(bool reset);
    static QString file_name_from_id(const QString& id);
    static QString scorer_file_name_from_id(const QString& id);
    void download(const QString& id, download_type type, int part = -1);
    void handle_download_progress(qint64 received, qint64 real_total);
    void handle_download_finished();
    void handle_download_ready_read();
    void handle_ssl_errors(const QList<QSslError> &errors);
    static QString make_checksum(const QString& file);
    static QString model_path(const QString& file_name);
    static void init_config();
    static void backup_config(const QString& lang_models_file);
    static bool xz_decode(const QString& file_in, const QString& file_out);
    static bool gz_decode(const QString& file_in, const QString& file_out);
    static bool join_part_files(const QString& file_out, int parts);
    bool check_download(const QString& file, const QString& checksum, comp_type comp, int parts = -1);
    static auto check_lang_file(const QJsonArray& langs);
    static comp_type str2comp(const QString& str);
    static QString download_filename(const QString& filename, comp_type comp, int part = -1);
};

#endif // MODELS_MANAGER_H

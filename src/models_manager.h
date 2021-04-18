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
#include <map>
#include <vector>
#include <tuple>

class models_manager : public QObject
{
    Q_OBJECT
public:
    struct lang_t {
        QString id;
        QString name;
        QString model_file;
        QString scorer_file;
        bool available = false;
        bool downloading = false;
    };

    models_manager(QObject *parent = nullptr);
    [[nodiscard]] bool ok() const;
    std::vector<lang_t> available_langs() const;
    std::vector<lang_t> langs() const;
    [[nodiscard]] bool model_exists(const QString& id) const;
    void download_model(const QString& id);
    void delete_model(const QString& id);

signals:
    void download_progress(const QString& id, double progress);
    void download_finished(const QString& id);
    void download_error(const QString& id);
    void models_changed();

private:
    enum class download_type { none, all, model, scorer };

    struct model_t {
        QString name;
        QString file_name;
        QString md5;
        QUrl url;
        qint64 size = 0;
        QString scorer_file_name;
        QString scorer_md5;
        QUrl scorer_url;
        qint64 scorer_size = 0;
        bool available = false;
        download_type current_dl = download_type::none;
    };

    static const QString lang_models_file;

    std::map<QString, model_t> models;
    QNetworkAccessManager nam;

    void parse_models_file(bool reset = false);
    static QString file_name_from_id(const QString& id);
    static QString scorer_file_name_from_id(const QString& id);
    void download(const QString& id, download_type type);
    void handle_download_progress(qint64 received, qint64 total);
    void handle_download_finished();
    void handle_download_ready_read();
    static QString md5(const QString& file);
    static QString model_path(const QString& file_name);
    static void init_config();
    static void backup_config(const QString& lang_models_file);
};

#endif // MODELS_MANAGER_H

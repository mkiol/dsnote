/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MODELS_MANAGER_H
#define MODELS_MANAGER_H

#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>
#include <atomic>
#include <map>
#include <set>
#include <thread>
#include <tuple>
#include <vector>

class models_manager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
   public:
    struct lang_t {
        QString id;
        QString name;
        bool available = false;
        bool downloading = false;
    };
    struct model_t {
        QString id;
        QString lang_id;
        QString name;
        QString model_file;
        QString scorer_file;
        bool experimental = false;
        bool available = false;
        bool downloading = false;
        double download_progress = 0.0;
    };

    explicit models_manager(QObject* parent = nullptr);
    ~models_manager();
    [[nodiscard]] bool ok() const;
    std::vector<model_t> available_models() const;
    std::vector<model_t> models(const QString& lang_id = {}) const;
    std::vector<lang_t> langs() const;
    [[nodiscard]] bool model_exists(const QString& id) const;
    void download_model(const QString& id);
    void cancel_model_download(const QString& id);
    void delete_model(const QString& id);
    [[nodiscard]] inline bool busy() const { return m_busy_value; }
    void reload();
    double model_download_progress(const QString& id) const;

   signals:
    void download_progress(const QString& id, double progress);
    void download_finished(const QString& id);
    void download_error(const QString& id);
    void models_changed();
    void busy_changed();

   private:
    enum class download_type { none, all, model, scorer, model_scorer };
    enum class comp_type { none, xz, gz, tar, tarxz };

    struct priv_model_t {
        QString lang_id;
        QString name;
        QString file_name;
        QString checksum;
        QString checksum_quick;
        comp_type comp = comp_type::none;
        std::vector<QUrl> urls;
        qint64 size = 0;
        QString scorer_file_name;
        QString scorer_checksum;
        QString scorer_checksum_quick;
        comp_type scorer_comp = comp_type::none;
        std::vector<QUrl> scorer_urls;
        qint64 scorer_size = 0;
        bool experimental = false;
        bool available = false;
        bool downloading = false;
        double download_progress = 0.0;
        qint64 downloaded_part_data = 0;
    };

    inline static const QString models_file{"models.json"};
    using langs_t = std::map<QString, QString>;
    using models_t = std::map<QString, priv_model_t>;

    models_t m_models;
    langs_t m_langs;
    QNetworkAccessManager m_nam;
    std::atomic_bool m_busy_value = false;
    std::thread m_thread;
    std::set<QString> models_to_cancel;
    bool m_pending_reload = false;

    bool parse_models_file_might_reset();
    static void parse_models_file(bool reset, langs_t* langs, models_t* models);
    static QString file_name_from_id(const QString& id);
    static QString scorer_file_name_from_id(const QString& id);
    void download(const QString& id, download_type type, int part = -1);
    void handle_download_progress(qint64 received, qint64 real_total);
    void handle_download_finished();
    void handle_download_ready_read();
    void handle_ssl_errors(const QList<QSslError>& errors);
    static QString make_checksum(const QString& file);
    static QString make_quick_checksum(const QString& file);
    static QString model_path(const QString& file_name);
    static void init_config();
    static void backup_config(const QString& lang_models_file);
    static bool xz_decode(const QString& file_in, const QString& file_out);
    static bool gz_decode(const QString& file_in, const QString& file_out);
    static bool tar_decode(const QString& file_in,
                           std::map<QString, QString>&& files_out);
    static bool join_part_files(const QString& file_out, int parts);
    bool handle_download(const QString& path, const QString& checksum,
                         const QString& path_in_archive, const QString& path_2,
                         const QString& checksum_2,
                         const QString& path_in_archive_2, comp_type comp,
                         int parts);
    static auto extract_models(const QJsonArray& models_jarray);
    static auto extract_langs(const QJsonArray& langs_jarray);
    static comp_type str2comp(const QString& str);
    static QString download_filename(const QString& filename, comp_type comp,
                                     int part = -1);
    static bool model_scorer_same_url(const priv_model_t& id);
    [[nodiscard]] bool lang_available(const QString& id) const;
    [[nodiscard]] bool lang_downloading(const QString& id) const;
    bool check_model_download_cancel(QNetworkReply* reply);
};

#endif  // MODELS_MANAGER_H

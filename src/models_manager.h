/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
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
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifndef QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH
#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH
namespace std {
template <>
struct hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
        return static_cast<size_t>(qHash(s));
    }
};
}  // namespace std
#endif

class models_manager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
   public:
    enum class model_engine { ds, vosk, whisper };

    struct lang_t {
        QString id;
        QString name;
        QString name_en;
        bool available = false;
        bool downloading = false;
    };

    struct model_t {
        QString id;
        model_engine engine = model_engine::ds;
        QString lang_id;
        QString name;
        QString model_file;
        QString scorer_file;
        int score = 2; /* 0-3 */
        bool default_for_lang = false;
        bool available = false;
        bool downloading = false;
        double download_progress = 0.0;
    };

    explicit models_manager(QObject* parent = nullptr);
    models_manager(const models_manager&) = delete;
    models_manager& operator=(const models_manager&) = delete;
    models_manager(models_manager&&) = delete;
    models_manager& operator=(models_manager&&) = delete;
    ~models_manager() override;
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
    void set_default_model_for_lang(const QString& model_id);

   signals:
    void download_progress(const QString& id, double progress);
    void download_finished(const QString& id);
    void download_error(const QString& id);
    void models_changed();
    void busy_changed();

   private:
    enum class download_type { none, all, model, scorer, model_scorer };
    enum class comp_type { none, xz, gz, tar, tarxz, zip };

    struct priv_model_t {
        model_engine engine = model_engine::ds;
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
        int score = 2; /* 0-3 */
        bool default_for_lang = false;
        bool available = false;
        bool downloading = false;
        double download_progress = 0.0;
        qint64 downloaded_part_data = 0;
    };

    inline static const QString models_file{QStringLiteral("models.json")};
    using langs_t = std::unordered_map<QString, std::pair<QString, QString>>;
    using models_t = std::unordered_map<QString, priv_model_t>;

    models_t m_models;
    langs_t m_langs;
    QNetworkAccessManager m_nam;
    std::atomic_bool m_busy_value = false;
    std::thread m_thread;
    std::set<QString> models_to_cancel;
    bool m_pending_reload = false;

    static QLatin1String download_type_str(download_type type);
    static QLatin1String comp_type_str(comp_type type);
    bool parse_models_file_might_reset();
    static void parse_models_file(bool reset, langs_t* langs, models_t* models);
    static QString file_name_from_id(const QString& id, model_engine engine);
    static QString scorer_file_name_from_id(const QString& id);
    void download(const QString& id, download_type type, int part = -1);
    void handle_download_progress(qint64 received, qint64 real_total);
    void handle_download_finished();
    void handle_download_ready_read();
    void handle_ssl_errors(const QList<QSslError>& errors);
    static QString model_path(const QString& file_name);
    static void init_config();
    static void backup_config(const QString& lang_models_file);
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
    static bool checksum_ok(const QString& checksum,
                            const QString& checksum_quick,
                            const QString& file_name);
    static model_engine engine_from_name(const QString& name);
    void update_default_model_for_lang(const QString& lang_id);
    bool extract_from_archive(const QString& archive_path, comp_type comp,
                              const QString& out_path, const QString& checksum,
                              const QString& path_in_archive,
                              const QString& out_path_2,
                              const QString& checksum_2,
                              const QString& path_in_archive_2);
    static bool check_checksum(const QString& path, const QString& checksum);
    static void remove_empty_langs(langs_t& langs, const models_t& models);
};

#endif  // MODELS_MANAGER_H

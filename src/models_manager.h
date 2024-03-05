/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MODELS_MANAGER_H
#define MODELS_MANAGER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>
#include <atomic>
#include <functional>
#include <optional>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "singleton.h"

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

class models_manager : public QObject, public singleton<models_manager> {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
   public:
    enum class model_role_t { stt = 0, tts = 1, mnt = 2, ttt = 3 };
    friend QDebug operator<<(QDebug d, model_role_t role);

    enum class model_engine_t {
        stt_ds,
        stt_vosk,
        stt_whisper,
        stt_fasterwhisper,
        stt_april,
        ttt_hftc,
        ttt_tashkeel,
        ttt_unikud,
        tts_coqui,
        tts_piper,
        tts_espeak,
        tts_rhvoice,
        tts_mimic3,
        mnt_bergamot
    };
    friend QDebug operator<<(QDebug d, model_engine_t engine);

    enum feature_flags : unsigned int {
        no_flags = 0,
        generic_start = 1 << 0,
        fast_processing = generic_start,
        medium_processing = 1 << 1,
        slow_processing = 1 << 2,
        high_quality = 1 << 3,
        medium_quality = 1 << 4,
        low_quality = 1 << 5,
        engine_stt_ds = 1 << 6,
        engine_stt_vosk = 1 << 7,
        engine_stt_whisper = 1 << 8,
        engine_stt_fasterwhisper = 1 << 9,
        engine_stt_april = 1 << 10,
        engine_tts_espeak = 1 << 11,
        engine_tts_piper = 1 << 12,
        engine_tts_rhvoice = 1 << 13,
        engine_tts_coqui = 1 << 14,
        engine_tts_mimic3 = 1 << 15,
        generic_end = engine_tts_mimic3,
        stt_start = 1 << 20,
        stt_intermediate_results = stt_start,
        stt_punctuation = 1 << 21,
        stt_end = stt_punctuation,
        tts_start = 1 << 30,
        tts_voice_cloning = tts_start,
        tts_end = tts_voice_cloning
    };
    friend inline feature_flags operator|(feature_flags a, feature_flags b) {
        return static_cast<feature_flags>(static_cast<int>(a) |
                                          static_cast<int>(b));
    }
    friend QDebug operator<<(QDebug d, feature_flags flags);

    enum class sup_model_role_t { scorer, vocoder, diacritizer };
    friend QDebug operator<<(QDebug d, sup_model_role_t role);

    struct lang_t {
        QString id;
        QString name;
        QString name_en;
        bool available = false;
        bool downloading = false;
    };

    struct lang_basic_t {
        QString id;
        QString name;
        QString name_en;
    };

    struct sup_model_file_t {
        sup_model_role_t role = sup_model_role_t::scorer;
        QString file;
    };

    struct license_t {
        QString id;
        QString name;
        QUrl url;
        bool accept_required = false;
    };

    struct download_info_t {
        std::vector<QUrl> urls;
        size_t total_size = 0;
    };

    struct model_t {
        QString id;
        model_engine_t engine = model_engine_t::stt_ds;
        QString lang_id;
        QString lang_code;
        QString name;
        QString model_file;
        std::vector<sup_model_file_t> sup_files;
        QString speaker;
        QString trg_lang_id;
        int score = 2;
        QString options;
        license_t license;
        download_info_t download_info;
        bool default_for_lang = false;
        bool available = false;
        bool dl_multi = false;
        bool dl_off = false;
        feature_flags features = feature_flags::no_flags;
        bool downloading = false;
        double download_progress = 0.0;
    };

    struct models_availability_t {
        bool tts_coqui = false;
        bool tts_mimic3 = false;
        bool tts_mimic3_de = false;
        bool tts_mimic3_es = false;
        bool tts_mimic3_fr = false;
        bool tts_mimic3_it = false;
        bool tts_mimic3_ru = false;
        bool tts_mimic3_sw = false;
        bool tts_mimic3_fa = false;
        bool tts_mimic3_nl = false;
        bool tts_rhvoice = false;
        bool stt_fasterwhisper = false;
        bool stt_ds = false;
        bool stt_vosk = false;
        bool mnt_bergamot = false;
        bool ttt_hftc = false;
        bool option_r = false;
    };

    static model_role_t role_of_engine(model_engine_t engine);
    static std::optional<std::reference_wrapper<const sup_model_file_t>>
    sup_model_file_of_role(sup_model_role_t role,
                           const std::vector<sup_model_file_t>& sub_models);

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
    std::unordered_map<QString, lang_basic_t> langs_map() const;
    std::unordered_map<QString, lang_basic_t> available_langs_map() const;
    std::unordered_map<QString, lang_basic_t> available_langs_of_role_map(
        model_role_t role) const;
    [[nodiscard]] bool model_exists(const QString& id) const;
    [[nodiscard]] bool has_model_of_role(model_role_t role) const;
    void download_model(const QString& id);
    void cancel_model_download(const QString& id);
    void delete_model(const QString& id);
    [[nodiscard]] inline bool busy() const { return m_busy_value; }
    void reload();
    double model_download_progress(const QString& id) const;
    void set_default_model_for_lang(const QString& model_id);
    void generate_checksums();
    void update_models_using_availability(models_availability_t availability);
    static void reset_models();

   signals:
    void download_progress(const QString& id, double progress);
    void download_finished(const QString& id, bool download_not_needed);
    void download_started(const QString& id);
    void download_error(const QString& id);
    void models_changed();
    void busy_changed();
    void generate_next_checksum_request();

   private:
    enum class download_type { none, all, model, sup, model_sup };
    friend QDebug operator<<(QDebug d, download_type download_type);

    enum class comp_type {
        none,
        xz,
        gz,
        tar,
        tarxz,
        targz,
        zip,
        zipall,
        dir,
        dirgz
    };
    friend QDebug operator<<(QDebug d, comp_type comp_type);

    struct checksum_check_t {
        bool ok = false;
        QString real_checksum;
        QString real_checksum_quick;
        long long size = 0;
    };

    struct sup_model_t {
        sup_model_role_t role = sup_model_role_t::scorer;
        QString file_name;
        QString checksum;
        QString checksum_quick;
        comp_type comp = comp_type::none;
        std::vector<QUrl> urls;
        long long size = 0;
        size_t urls_hash = 0;
    };

    struct priv_model_t {
        model_engine_t engine = model_engine_t::stt_ds;
        QString lang_id;
        QString lang_code;
        QString name;
        QString file_name;
        QString checksum;
        QString checksum_quick;
        comp_type comp = comp_type::none;
        std::vector<QUrl> urls;
        long long size = 0;
        std::vector<sup_model_t> sup_models;
        QString speaker;
        QString trg_lang_id;
        QString alias_of;
        int score = -1; /* 0-5 */
        QString options;
        license_t license;
        bool hidden = false;
        bool default_for_lang = false;
        bool exists = false;
        bool available = false;
        bool dl_multi = false;
        bool dl_off = false;
        feature_flags features = feature_flags::no_flags;
        size_t urls_hash = 0;
        bool downloading = false;
        double download_progress = 0.0;
        qint64 downloaded_part_data = 0;
    };

    inline static const QString models_file{QStringLiteral("models.json")};
    inline static const int default_score = 2;
    inline static const int default_score_tts_espeak = 1;
    using langs_t = std::unordered_map<QString, std::pair<QString, QString>>;
    using models_t = std::unordered_map<QString, priv_model_t>;
    using langs_of_role_t = std::unordered_map<model_role_t, std::set<QString>>;

    models_t m_models;
    langs_t m_langs;
    langs_of_role_t m_langs_of_role;
    QNetworkAccessManager m_nam;
    std::atomic_bool m_busy_value = false;
    std::thread m_thread;
    std::set<QString> models_to_cancel;
    bool m_pending_reload = false;
    models_t m_models_for_gen_checksum;
    models_t::iterator m_it_for_gen_checksum;
    bool m_delayed_gen_checksum = false;
    std::optional<models_availability_t> m_models_availability;

    static QLatin1String download_type_str(download_type type);
    static QLatin1String comp_type_str(comp_type type);
    bool parse_models_file_might_reset();
    static void parse_models_file(
        bool reset, langs_t* langs, models_t* models,
        std::optional<models_availability_t> models_availability);
    static QString file_name_from_id(const QString& id, model_engine_t engine);
    static QString sup_file_name_from_id(const QString& id,
                                         sup_model_role_t role);
    void download(const QString& id, download_type type, int part,
                  size_t sup_idx);
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
    static auto extract_models(
        const QJsonArray& models_jarray,
        std::optional<models_availability_t> models_availability);
    static auto extract_langs(const QJsonArray& langs_jarray);
    static comp_type str2comp(const QString& str);
    static QString download_filename(QString filename, comp_type comp,
                                     int part = -1, const QUrl& url = {});
    static bool model_sup_same_url(const priv_model_t& id, size_t sup_idx);
    [[nodiscard]] bool lang_available(const QString& id) const;
    [[nodiscard]] bool lang_downloading(const QString& id) const;
    bool check_model_download_cancel(QNetworkReply* reply);
    static bool checksum_ok(const QString& checksum,
                            const QString& checksum_quick,
                            const QString& file_name);
    static bool model_checksum_ok(const priv_model_t& model);
    static bool sup_model_checksum_ok(const sup_model_t& model);
    static std::optional<size_t> sup_models_checksum_last_nok(
        const std::vector<sup_model_t>& models, size_t idx);
    static bool sup_models_exist(const std::vector<sup_model_t>& models);
    static model_engine_t engine_from_name(const QString& name);
    static feature_flags feature_from_name(const QString& name);
    static sup_model_role_t sup_model_role_from_name(const QString& name);
    void update_default_model_for_lang(const QString& lang_id);
    checksum_check_t extract_from_archive(
        const QString& archive_path, comp_type comp, const QString& out_path,
        const QString& checksum, const QString& path_in_archive,
        const QString& out_path_2, const QString& checksum_2,
        const QString& path_in_archive_2);
    static checksum_check_t check_checksum(const QString& path,
                                           const QString& checksum);
    static void remove_empty_langs(langs_t& langs, const models_t& models);
    static langs_of_role_t make_langs_of_role(const models_t& models);
    static void remove_downloaded_files_on_error(const QString& path,
                                                 models_manager::comp_type comp,
                                                 int part);
    static qint64 total_size(const QString& path);
    void generate_next_checksum();
    void handle_generate_checksum(const checksum_check_t& check);
    static void print_priv_model(const QString& id, const priv_model_t& model);
    static std::vector<sup_model_file_t> sup_model_files(
        const std::vector<sup_model_t>& sub_models);
    static long long sup_models_total_size(
        const std::vector<sup_model_t>& sub_models);
    static void extract_sup_models(const QString& model_id,
                                   const QJsonObject& model_obj,
                                   std::vector<sup_model_t>& sup_models);
    void update_models_using_availability_internal();
    static void update_dl_multi(models_t& models);
    static void update_dl_off(models_t& models);
    static void update_url_hash(priv_model_t& model);
    static download_info_t make_download_info(const priv_model_t& model);
    static feature_flags add_explicit_feature_flags(
        const QString& model_id, model_engine_t engine,
        feature_flags existing_features);
    static feature_flags add_new_feature(feature_flags existing_features,
                                         feature_flags new_feature);
};

#endif  // MODELS_MANAGER_H

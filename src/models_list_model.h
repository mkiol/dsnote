/* Copyright (C) 2021-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MODELSLISTMODEL_H
#define MODELSLISTMODEL_H

#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>

#include "itemmodel.h"
#include "listmodel.h"
#include "models_manager.h"

class ModelsListModel : public SelectableItemModel {
    Q_OBJECT
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
    Q_PROPERTY(QString lang READ lang WRITE setLang NOTIFY langChanged)
    Q_PROPERTY(QString pack READ pack WRITE setPack NOTIFY packChanged)
    Q_PROPERTY(unsigned int roleFilterFlags READ roleFilterFlags WRITE
                   setRoleFilterFlags NOTIFY roleFilterFlagsChanged)
    Q_PROPERTY(unsigned int featureFilterFlags READ featureFilterFlags WRITE
                   setFeatureFilterFlags NOTIFY featureFilterFlagsChanged)
    Q_PROPERTY(
        unsigned int disabledFeatureFilterFlags READ disabledFeatureFilterFlags
            NOTIFY disabledFeatureFilterFlagsChanged)
    Q_PROPERTY(
        bool defaultFilters READ defaultFilters NOTIFY defaultFiltersChanged)
   public:
    enum ModelRole { Stt = 0, Tts = 1, Mnt = 2, Ttt = 3 };
    Q_ENUM(ModelRole)

    enum ModelRoleFilterFlags : unsigned int {
        RoleNone = 0U,
        RoleStart = 1U << 0U,
        RoleStt = RoleStart,
        RoleTts = 1U << 1U,
        RoleMnt = 1U << 2U,
        RoleOther = 1U << 3U,
        RoleEnd = RoleOther,
        RoleAll = RoleStt | RoleTts | RoleMnt | RoleOther,
        RoleDefault = RoleAll
    };
    Q_ENUM(ModelRoleFilterFlags)

    // must be the same as models_manager::feature_flags
    enum ModelFeatureFilterFlags : unsigned int {
        FeatureNone = 0U,
        FeatureGenericStart = 1U << 0U,
        FeatureFirst = FeatureGenericStart,
        FeatureFastProcessing = FeatureGenericStart,
        FeatureMediumProcessing = 1U << 1U,
        FeatureSlowProcessing = 1U << 2U,
        FeatureQualityHigh = 1U << 3U,
        FeatureQualityMedium = 1U << 4U,
        FeatureQualityLow = 1U << 5U,
        FeatureEngineSttStart = 1U << 6U,
        FeatureEngineSttDs = FeatureEngineSttStart,
        FeatureEngineSttVosk = 1U << 7U,
        FeatureEngineSttWhisper = 1U << 8U,
        FeatureEngineSttFasterWhisper = 1U << 9U,
        FeatureEngineSttApril = 1U << 10U,
        FeatureEngineSttEnd = FeatureEngineSttApril,
        FeatureEngineTtsStart = 1U << 11U,
        FeatureEngineTtsEspeak = FeatureEngineTtsStart,
        FeatureEngineTtsPiper = 1U << 12U,
        FeatureEngineTtsRhvoice = 1U << 13U,
        FeatureEngineTtsCoqui = 1U << 14U,
        FeatureEngineTtsMimic3 = 1U << 15U,
        FeatureEngineTtsWhisperSpeech = 1U << 16U,
        FeatureEngineTtsSam = 1U << 17U,
        FeatureEngineTtsParler = 1U << 18U,
        FeatureEngineTtsEnd = FeatureEngineTtsParler,
        FeatureEngineMnt = 1U << 20U,
        FeatureEngineOther = 1U << 21U,
        FeatureGenericEnd = FeatureEngineOther,
        FeatureHwOpenVino = 1U << 22U,
        FeatureSttStart = 1U << 23U,
        FeatureSttIntermediateResults = FeatureSttStart,
        FeatureSttPunctuation = 1U << 24U,
        FeatureSttEnd = FeatureSttPunctuation,
        FeatureTtsStart = 1U << 25U,
        FeatureTtsVoiceCloning = FeatureTtsStart,
        FeatureTtsPrompt = 1U << 26U,
        FeatureTtsEnd = FeatureTtsPrompt,
        FeatureLast = FeatureTtsEnd,
        FeatureAllSttEngines = FeatureEngineSttDs | FeatureEngineSttVosk |
                               FeatureEngineSttWhisper |
                               FeatureEngineSttFasterWhisper |
                               FeatureEngineSttApril,
        FeatureAllTtsEngines = FeatureEngineTtsEspeak | FeatureEngineTtsPiper |
                               FeatureEngineTtsRhvoice | FeatureEngineTtsCoqui |
                               FeatureEngineTtsMimic3 |
                               FeatureEngineTtsWhisperSpeech |
                               FeatureEngineTtsSam | FeatureEngineTtsParler,
        FeatureAll = FeatureFastProcessing | FeatureMediumProcessing |
                     FeatureSlowProcessing | FeatureQualityHigh |
                     FeatureQualityMedium | FeatureQualityLow |
                     FeatureAllSttEngines | FeatureAllTtsEngines |
                     FeatureSttIntermediateResults | FeatureSttPunctuation |
                     FeatureTtsVoiceCloning | FeatureTtsPrompt,
        FeatureDefault = FeatureFastProcessing | FeatureMediumProcessing |
                         FeatureSlowProcessing | FeatureQualityHigh |
                         FeatureQualityMedium | FeatureQualityLow |
                         FeatureAllSttEngines | FeatureAllTtsEngines,
        FeatureAdditional = FeatureSttIntermediateResults |
                            FeatureSttPunctuation | FeatureTtsVoiceCloning |
                            FeatureTtsPrompt

    };
    Q_ENUM(ModelFeatureFilterFlags)
    friend QDebug operator<<(QDebug d, ModelFeatureFilterFlags flags);

    explicit ModelsListModel(QObject *parent = nullptr);
    ~ModelsListModel() override;
    Q_INVOKABLE void addRoleFilterFlag(ModelRoleFilterFlags flag);
    Q_INVOKABLE void removeRoleFilterFlag(ModelRoleFilterFlags flag);
    Q_INVOKABLE void resetRoleFilterFlags();
    Q_INVOKABLE void addFeatureFilterFlag(ModelFeatureFilterFlags flag);
    Q_INVOKABLE void removeFeatureFilterFlag(ModelFeatureFilterFlags flag);
    Q_INVOKABLE void resetFeatureFilterFlags();
    Q_INVOKABLE unsigned int countForRole(ModelRole role) const;

   signals:
    void itemChanged(int idx);
    void downloadingChanged();
    void langChanged();
    void packChanged();
    void roleFilterFlagsChanged();
    void featureFilterFlagsChanged();
    void disabledFeatureFilterFlagsChanged();
    void defaultFiltersChanged();

   private:
    int m_changedItem = -1;
    bool m_downloading = false;
    QString m_lang;
    QString m_pack;
    unsigned int m_roleFilterFlags = ModelRoleFilterFlags::RoleDefault;
    unsigned int m_featureFilterFlags = ModelFeatureFilterFlags::FeatureDefault;
    unsigned int m_disabledFeatureFilterFlags =
        ModelFeatureFilterFlags::FeatureNone;

    QList<ListItem *> makeItems() override;
    static ListItem *makeItem(const models_manager::model_t &model);
    size_t firstChangedItemIdx(const QList<ListItem *> &oldItems,
                               const QList<ListItem *> &newItems) override;
    void updateItem(ListItem *oldItem, const ListItem *newItem) override;
    bool downloading() const { return m_downloading; }
    QString lang() const { return m_lang; }
    QString pack() const { return m_pack; }
    auto roleFilterFlags() const { return m_roleFilterFlags; }
    auto featureFilterFlags() const { return m_featureFilterFlags; }
    auto disabledFeatureFilterFlags() const {
        return m_disabledFeatureFilterFlags;
    }
    void setLang(const QString &lang);
    void setPack(const QString &pack);
    void setRoleFilterFlags(unsigned roleFilterFlags);
    void setFeatureFilterFlags(unsigned int featureFilterFlags);
    void updateDownloading(const std::vector<models_manager::model_t> &models);
    bool roleFilterPass(const models_manager::model_t &model) const;
    bool featureFilterPass(const models_manager::model_t &model) const;
    bool defaultFilters() const;
};

class ModelsListItem : public SelectableItem {
    Q_OBJECT
   public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        IdRole = Qt::UserRole,
        LangIdRole,
        PackIdRole,
        PackCountRole,
        PackAvailableCountRole,
        AvailableRole,
        DlMultiRole,
        DlOffRole,
        ScoreRole,
        FeaturesRole,
        DefaultRole,
        DownloadingRole,
        ProgressRole,
        ModelRole,
        LicenseIdRole,
        LicenseNameRole,
        LicenseUrlRole,
        LicenseAccceptRequiredRole,
        DownloadUrlsRole,
        DownloadSizeRole,
        InfoRole
    };

    struct License {
        QString id;
        QString name;
        QUrl url;
        bool accept_required = false;
    };

    struct DownloadInfo {
        QStringList urls;
        QString size;
    };

    explicit ModelsListItem(QObject *parent = nullptr)
        : SelectableItem{parent} {}
    ModelsListItem(const QString &id, QString name, QString lang_id,
                   QString pack_id, int pack_count, int pack_available_count,
                   ModelsListModel::ModelRole role, License license,
                   QString info, DownloadInfo download_info,
                   bool available = true, bool dl_multi = false,
                   bool dl_off = false, unsigned int features = 0,
                   int score = 2, bool default_for_lang = false,
                   bool downloading = false, double progress = 0.0,
                   QObject *parent = nullptr);
    QVariant data(int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString id() const override { return m_id; }
    QString name() const { return m_name; }
    QString lang_id() const { return m_lang_id; }
    QString pack_id() const { return m_pack_id; }
    int pack_count() const { return m_pack_count; }
    int pack_available_count() const { return m_pack_available_count; }
    ModelsListModel::ModelRole modelRole() const { return m_role; }
    bool available() const { return m_available; }
    bool dl_multi() const { return m_dl_multi; }
    bool dl_off() const { return m_dl_off; }
    auto features() const { return m_features; }
    int score() const { return m_score; }
    bool default_for_lang() const { return m_default_for_lang; }
    bool downloading() const { return m_downloading; }
    double progress() const { return m_progress; }
    QString license_id() const { return m_license.id; }
    QString license_name() const { return m_license.name; }
    QUrl license_url() const { return m_license.url; }
    bool license_accept_required() const { return m_license.accept_required; }
    QStringList download_urls() const { return m_download_info.urls; }
    QString download_size() const { return m_download_info.size; }
    void update(const ModelsListItem *item);
    QString info() const { return m_info; };

   private:
    QString m_id;
    QString m_name;
    QString m_lang_id;
    QString m_pack_id;
    QString m_info;
    int m_pack_count = 0;
    int m_pack_available_count = 0;
    ModelsListModel::ModelRole m_role = ModelsListModel::ModelRole::Stt;
    License m_license;
    DownloadInfo m_download_info;
    bool m_available = false;
    bool m_dl_multi = false;
    bool m_dl_off = false;
    unsigned int m_features = 0;
    int m_score = 2;
    bool m_default_for_lang = false;
    bool m_downloading = false;
    double m_progress = 0.0;
};

#endif  // MODELSLISTMODEL_H

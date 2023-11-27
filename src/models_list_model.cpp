/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "models_list_model.h"

#include <QDebug>
#include <QList>
#include <algorithm>
#include <array>

static int range_mask(int start_mask, int end_mask) {
    int mask = 0;
    for (int flag = start_mask; flag <= end_mask; flag <<= 1) mask |= flag;

    return mask;
}

static bool flag_set_in_range_mask(int flag, int start_mask, int end_mask) {
    return flag & range_mask(start_mask, end_mask);
}

ModelsListModel::ModelsListModel(QObject *parent)
    : SelectableItemModel{new ModelsListItem, parent} {
    connect(
        models_manager::instance(), &models_manager::models_changed, this,
        [this] { updateModel(); }, Qt::QueuedConnection);
    connect(
        models_manager::instance(), &models_manager::download_started, this,
        [this]([[maybe_unused]] const QString &id) { updateModel(); },
        Qt::QueuedConnection);
    connect(
        this, &ItemModel::busyChanged, this,
        [this] {
            if (!isBusy() && m_changedItem > -1) {
                emit itemChanged(m_changedItem);
                m_changedItem = -1;
            }
        },
        Qt::QueuedConnection);
}

ModelsListModel::~ModelsListModel() { m_worker.reset(); }

size_t ModelsListModel::firstChangedItemIdx(const QList<ListItem *> &oldItems,
                                            const QList<ListItem *> &newItems) {
    const auto &[it, _] = std::mismatch(
        oldItems.cbegin(), oldItems.cend(), newItems.cbegin(), newItems.cend(),
        [](const ListItem *a, const ListItem *b) {
            const auto *aa = qobject_cast<const ModelsListItem *>(a);
            const auto *bb = qobject_cast<const ModelsListItem *>(b);

            return aa->id() == bb->id() && aa->available() == bb->available() &&
                   aa->downloading() == bb->downloading() &&
                   aa->progress() == bb->progress() &&
                   aa->default_for_lang() == bb->default_for_lang();
        });

    auto idx = std::distance(oldItems.cbegin(), it);

    m_changedItem = static_cast<int>(idx);

    return idx;
}

void ModelsListModel::updateItem(ListItem *oldItem, const ListItem *newItem) {
    auto *oi = qobject_cast<ModelsListItem *>(oldItem);
    const auto *ni = qobject_cast<const ModelsListItem *>(newItem);

    oi->update(ni);
}

// inspiration: https://gist.github.com/dgoguerra/7194777
static QString size_to_human_size(size_t bytes) {
    std::array suffix = {"B", "KB", "MB", "GB", "TB"};

    double dbl_bytes = bytes;
    auto i = 0u;
    if (bytes > 1024) {
        for (i = 0; (bytes / 1024) > 0 && i < suffix.size() - 1;
             i++, bytes /= 1024)
            dbl_bytes = bytes / 1024.0;
    }

    return QStringLiteral("%1 %2")
        .arg(dbl_bytes, 0, 'f',
             i > 2   ? 2
             : i > 1 ? 1
                     : 0)
        .arg(suffix.at(i));
}

ListItem *ModelsListModel::makeItem(const models_manager::model_t &model) {
    auto role = [&] {
        switch (models_manager::role_of_engine(model.engine)) {
            case models_manager::model_role_t::stt:
                return ModelRole::Stt;
            case models_manager::model_role_t::ttt:
                return ModelRole::Ttt;
            case models_manager::model_role_t::tts:
                return ModelRole::Tts;
            case models_manager::model_role_t::mnt:
                return ModelRole::Mnt;
        }
        throw std::runtime_error{"unsupported model engine"};
    }();

    ModelsListItem::DownloadInfo download_info;
    download_info.size = size_to_human_size(model.download_info.total_size);
    for (const auto &url : model.download_info.urls)
        download_info.urls.push_back(url.toString());

    return new ModelsListItem{
        /*id=*/model.id,
        /*name=*/role == ModelRole::Mnt
            ? QStringLiteral("%1 / %2-%3")
                  .arg(model.name, model.lang_id, model.trg_lang_id)
            : QStringLiteral("%1 / %2").arg(model.name, model.lang_id),
        /*langId=*/model.lang_id,
        /*role=*/role,
        /*license=*/
        {model.license.id, model.license.name, model.license.url,
         model.license.accept_required},
        /*download_info=*/std::move(download_info),
        /*available=*/model.available,
        /*dl_multi=*/model.dl_multi,
        /*dl_off=*/model.dl_off,
        /*dl_off=*/model.features,
        /*score=*/model.score,
        /*default_for_lang=*/model.default_for_lang,
        /*downloading=*/model.downloading,
        /*progress=*/model.download_progress};
}

bool ModelsListModel::roleFilterPass(const models_manager::model_t &model) {
    switch (models_manager::role_of_engine(model.engine)) {
        case models_manager::model_role_t::stt:
            return m_roleFilterFlags & ModelRoleFilterFlags::RoleStt;
        case models_manager::model_role_t::tts:
            return m_roleFilterFlags & ModelRoleFilterFlags::RoleTts;
        case models_manager::model_role_t::mnt:
            return m_roleFilterFlags & ModelRoleFilterFlags::RoleMnt;
        case models_manager::model_role_t::ttt:
            return m_roleFilterFlags & ModelRoleFilterFlags::RoleOther;
    }
    
    return false;
}

bool ModelsListModel::genericFeatureFilterPass(
    const models_manager::model_t &model) {
    auto role = models_manager::role_of_engine(model.engine);
    if (role == models_manager::model_role_t::mnt ||
        role == models_manager::model_role_t::ttt)
        return true;

    for (int flag = ModelFeatureFilterFlags::FeatureGenericStart;
         flag <= ModelFeatureFilterFlags::FeatureGenericEnd; flag <<= 1) {
        if (!(m_featureFilterFlags & flag) && model.features & flag)
            return false;
    }

    return true;
}

bool ModelsListModel::featureFilterPass(const models_manager::model_t &model) {
    auto none_of = [&](int start_flag, int end_flag) {
        for (int flag = start_flag; flag <= end_flag; flag <<= 1)
            if (m_featureFilterFlags & flag) return false;
        return true;
    };

    if (none_of(ModelFeatureFilterFlags::FeatureSttStart,
                ModelFeatureFilterFlags::FeatureTtsEnd))
        return true;

    auto passFeature = [&](ModelFeatureFilterFlags start_flag,
                           ModelFeatureFilterFlags end_flag) {
        for (int flag = start_flag; flag <= end_flag; flag <<= 1) {
            if ((m_featureFilterFlags & flag) && (model.features & flag))
                return true;
        }

        return false;
    };

    auto role = models_manager::role_of_engine(model.engine);

    if (role == models_manager::model_role_t::stt)
        return passFeature(ModelFeatureFilterFlags::FeatureSttStart,
                           ModelFeatureFilterFlags::FeatureSttEnd);
    if (role == models_manager::model_role_t::tts)
        return passFeature(ModelFeatureFilterFlags::FeatureTtsStart,
                           ModelFeatureFilterFlags::FeatureTtsEnd);

    return false;
}

void ModelsListModel::addRoleFilterFlag(ModelRoleFilterFlags flag) {
    auto flags = m_roleFilterFlags | flag;
    setRoleFilterFlags(flags);
}

void ModelsListModel::removeRoleFilterFlag(ModelRoleFilterFlags flag) {
    auto flags = m_roleFilterFlags & ~(flag);
    setRoleFilterFlags(flags);
}

void ModelsListModel::resetRoleFilterFlags() {
    setRoleFilterFlags(ModelRoleFilterFlags::RoleDefault);
}

void ModelsListModel::addFeatureFilterFlag(ModelFeatureFilterFlags flag) {
    auto flags = m_featureFilterFlags | flag;
    setFeatureFilterFlags(flags);
}

void ModelsListModel::removeFeatureFilterFlag(ModelFeatureFilterFlags flag) {
    auto flags = m_featureFilterFlags & ~(flag);
    setFeatureFilterFlags(flags);
}

void ModelsListModel::resetFeatureFilterFlags() {
    setFeatureFilterFlags(ModelFeatureFilterFlags::FeatureDefault);
}

bool ModelsListModel::defaultFilters() const {
    return m_roleFilterFlags == ModelRoleFilterFlags::RoleDefault &&
           m_featureFilterFlags == ModelFeatureFilterFlags::FeatureDefault;
}

QList<ListItem *> ModelsListModel::makeItems() {
    QList<ListItem *> items;

    auto models = models_manager::instance()->models(m_lang);

    updateDownloading(models);

    auto phase = getFilter();

    int existing_not_generic_feature_flags = 0;
    auto add_not_generic_feature_flag_if_exists =
        [&existing_not_generic_feature_flags](int feature_flags) {
            for (int flag = ModelFeatureFilterFlags::FeatureSttStart;
                 flag <= ModelFeatureFilterFlags::FeatureTtsEnd; flag <<= 1)
                if (feature_flags & flag)
                    existing_not_generic_feature_flags |= flag;
        };

    if (phase.isEmpty()) {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (roleFilterPass(model)) {
                if (genericFeatureFilterPass(model)) {
                    add_not_generic_feature_flag_if_exists(model.features);
                    if (featureFilterPass(model))
                        items.push_back(makeItem(model));
                }
            }
        });
    } else {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (roleFilterPass(model)) {
                if (genericFeatureFilterPass(model) &&
                    (model.name.contains(phase, Qt::CaseInsensitive) ||
                     model.lang_id.contains(phase, Qt::CaseInsensitive) ||
                     model.trg_lang_id.contains(phase, Qt::CaseInsensitive) ||
                     QStringLiteral("%1-%2")
                         .arg(model.lang_id, model.trg_lang_id)
                         .contains(phase, Qt::CaseInsensitive))) {
                    add_not_generic_feature_flag_if_exists(model.features);
                    if (featureFilterPass(model))
                        items.push_back(makeItem(model));
                }
            }
        });
    }

    m_disabledFeatureFilterFlags = 0;
    for (int flag = ModelFeatureFilterFlags::FeatureSttStart;
         flag <= ModelFeatureFilterFlags::FeatureTtsEnd; flag <<= 1) {
        if ((flag & existing_not_generic_feature_flags) == 0)
            m_disabledFeatureFilterFlags |= flag;
    }

    emit disabledFeatureFilterFlagsChanged();

    return items;
}

void ModelsListModel::setLang(const QString &lang) {
    if (lang != m_lang) {
        m_lang = lang;
        updateModel();
        emit langChanged();
    }
}

void ModelsListModel::setRoleFilterFlags(int roleFilterFlags) {
    if (roleFilterFlags != m_roleFilterFlags) {
        auto old_default = defaultFilters();

        if (flag_set_in_range_mask(roleFilterFlags,
                                   ModelRoleFilterFlags::RoleStart,
                                   ModelRoleFilterFlags::RoleEnd))
            m_roleFilterFlags = roleFilterFlags;
        updateModel();
        emit roleFilterFlagsChanged();

        if (old_default != defaultFilters()) emit defaultFiltersChanged();
    }
}

void ModelsListModel::setFeatureFilterFlags(int featureFilterFlags) {
    if (featureFilterFlags != m_featureFilterFlags) {
        auto old_default = defaultFilters();

        if (flag_set_in_range_mask(
                featureFilterFlags,
                ModelFeatureFilterFlags::FeatureFastProcessing,
                ModelFeatureFilterFlags::FeatureSlowProcessing) &&
            flag_set_in_range_mask(
                featureFilterFlags, ModelFeatureFilterFlags::FeatureQualityHigh,
                ModelFeatureFilterFlags::FeatureQualityLow)) {
            m_featureFilterFlags = featureFilterFlags;
        }
        updateModel();
        emit featureFilterFlagsChanged();

        if (old_default != defaultFilters()) emit defaultFiltersChanged();
    }
}

void ModelsListModel::updateDownloading(
    const std::vector<models_manager::model_t> &models) {
    const bool new_downloading =
        !std::none_of(models.cbegin(), models.cend(),
                      [](const auto &model) { return model.downloading; });
    if (m_downloading != new_downloading) {
        m_downloading = new_downloading;
        emit downloadingChanged();
    }
}

ModelsListItem::ModelsListItem(const QString &id, QString name, QString langId,
                               ModelsListModel::ModelRole role, License license,
                               DownloadInfo download_info, bool available,
                               bool dl_multi, bool dl_off, int features,
                               int score, bool default_for_lang,
                               bool downloading, double progress,
                               QObject *parent)
    : SelectableItem{parent},
      m_id{id},
      m_name{std::move(name)},
      m_lang_id{std::move(langId)},
      m_role{role},
      m_license{std::move(license)},
      m_download_info{std::move(download_info)},
      m_available{available},
      m_dl_multi{dl_multi},
      m_dl_off{dl_off},
      m_features{features},
      m_score{score},
      m_default_for_lang{default_for_lang},
      m_downloading{downloading},
      m_progress{progress} {
    m_selectable = false;
}

QHash<int, QByteArray> ModelsListItem::roleNames() const {
    QHash<int, QByteArray> names;
    names[IdRole] = QByteArrayLiteral("id");
    names[NameRole] = QByteArrayLiteral("name");
    names[LangIdRole] = QByteArrayLiteral("lang_id");
    names[ModelRole] = QByteArrayLiteral("role");
    names[AvailableRole] = QByteArrayLiteral("available");
    names[DlMultiRole] = QByteArrayLiteral("dl_multi");
    names[DlOffRole] = QByteArrayLiteral("dl_off");
    names[ScoreRole] = QByteArrayLiteral("score");
    names[FeaturesRole] = QByteArrayLiteral("features");
    names[DefaultRole] = QByteArrayLiteral("default_for_lang");
    names[DownloadingRole] = QByteArrayLiteral("downloading");
    names[ProgressRole] = QByteArrayLiteral("progress");
    names[LicenseIdRole] = QByteArrayLiteral("license_id");
    names[LicenseNameRole] = QByteArrayLiteral("license_name");
    names[LicenseUrlRole] = QByteArrayLiteral("license_url");
    names[LicenseAccceptRequiredRole] =
        QByteArrayLiteral("license_accept_required");
    names[DownloadUrlsRole] = QByteArrayLiteral("download_urls");
    names[DownloadSizeRole] = QByteArrayLiteral("download_size");
    return names;
}

QVariant ModelsListItem::data(int role) const {
    switch (role) {
        case IdRole:
            return id();
        case NameRole:
            return name();
        case LangIdRole:
            return lang_id();
        case ModelRole:
            return modelRole();
        case AvailableRole:
            return available();
        case DlMultiRole:
            return dl_multi();
        case DlOffRole:
            return dl_off();
        case ScoreRole:
            return score();
        case FeaturesRole:
            return features();
        case DefaultRole:
            return default_for_lang();
        case DownloadingRole:
            return downloading();
        case ProgressRole:
            return progress();
        case LicenseIdRole:
            return license_id();
        case LicenseNameRole:
            return license_name();
        case LicenseUrlRole:
            return license_url();
        case LicenseAccceptRequiredRole:
            return license_accept_required();
        case DownloadUrlsRole:
            return download_urls();
        case DownloadSizeRole:
            return download_size();
    }

    return {};
}

void ModelsListItem::update(const ModelsListItem *item) {
    if (m_downloading != item->downloading() ||
        m_dl_multi != item->dl_multi() || m_dl_off != item->dl_off() ||
        m_available != item->available() || m_progress != item->progress() ||
        m_default_for_lang != item->default_for_lang()) {
        m_downloading = item->downloading();
        m_available = item->available();
        m_dl_multi = item->dl_multi();
        m_dl_off = item->dl_off();
        m_progress = item->progress();
        m_default_for_lang = item->default_for_lang();
        m_dl_multi = item->dl_multi();
        m_dl_off = item->dl_off();
        emit itemDataChanged();
    }
}

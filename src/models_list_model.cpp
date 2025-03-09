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

QDebug operator<<(QDebug d, ModelsListModel::ModelFeatureFilterFlags flags) {
    auto print_flag_name = [&d](ModelsListModel::ModelFeatureFilterFlags flag) {
        switch (flag) {
            case ModelsListModel::ModelFeatureFilterFlags::FeatureNone:
                d << "none";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureFastProcessing:
                d << "fast-processing";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureMediumProcessing:
                d << "medium-processing";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureSlowProcessing:
                d << "slow-processing";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureQualityHigh:
                d << "quality-high";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureQualityMedium:
                d << "quality-medium";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureQualityLow:
                d << "quality-low";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureEngineSttDs:
                d << "engine-stt-ds";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureEngineSttVosk:
                d << "engine-stt-vosk";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineSttWhisper:
                d << "engine-stt-whisper";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineSttFasterWhisper:
                d << "engine-stt-faster-whisper";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineSttApril:
                d << "engine-stt-april";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineTtsEspeak:
                d << "engine-tts-espeak";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineTtsPiper:
                d << "engine-tts-piper";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineTtsRhvoice:
                d << "engine-tts-rhvoice";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineTtsCoqui:
                d << "engine-tts-coqui";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineTtsMimic3:
                d << "engine-tts-mimic3";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineTtsWhisperSpeech:
                d << "engine-tts-whisper-speech";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureEngineTtsParler:
                d << "engine-tts-parler";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureEngineTtsSam:
                d << "engine-tts-sam";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureEngineMnt:
                d << "engine-mnt";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureEngineOther:
                d << "engine-other";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureHwOpenVino:
                d << "hw-openvino";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureSttIntermediateResults:
                d << "stt-intermediate-results";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureSttPunctuation:
                d << "stt-punctuation";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::
                FeatureTtsVoiceCloning:
                d << "tts-voice-cloning";
                break;
            case ModelsListModel::ModelFeatureFilterFlags::FeatureTtsPrompt:
                d << "tts-prompt";
                break;
            default:
                break;
        }
    };

    if (flags == ModelsListModel::ModelFeatureFilterFlags::FeatureNone) {
        print_flag_name(flags);
    } else {
        for (unsigned int flag =
                 ModelsListModel::ModelFeatureFilterFlags::FeatureFirst;
             flag <= ModelsListModel::ModelFeatureFilterFlags::FeatureLast;
             flag <<= 1U) {
            if ((flags & flag) > 0) {
                print_flag_name(
                    static_cast<ModelsListModel::ModelFeatureFilterFlags>(
                        flag));
            }
        }
    }

    return d;
}

static unsigned int range_mask(unsigned int start_mask, unsigned int end_mask) {
    unsigned int mask = 0;
    for (unsigned int flag = start_mask; flag <= end_mask; flag <<= 1U)
        mask |= flag;

    return mask;
}

static bool flag_set_in_range_mask(unsigned int flag, unsigned int start_mask,
                                   unsigned int end_mask) {
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
    auto i = 0U;
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
        /*lang_id=*/model.lang_id,
        /*pack_id=*/model.pack_id,
        /*pack_count=*/static_cast<int>(model.pack_count),
        /*pack_available_count=*/static_cast<int>(model.pack_available_count),
        /*role=*/role,
        /*license=*/
        {model.license.id, model.license.name, model.license.url,
         model.license.accept_required},
        /*download_info=*/std::move(download_info),
        /*available=*/model.available,
        /*dl_multi=*/model.dl_multi,
        /*dl_off=*/model.dl_off,
        /*features=*/model.features,
        /*score=*/model.score,
        /*default_for_lang=*/model.default_for_lang,
        /*downloading=*/model.downloading,
        /*progress=*/model.download_progress};
}

bool ModelsListModel::roleFilterPass(
    const models_manager::model_t &model) const {
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

bool ModelsListModel::featureFilterPass(
    const models_manager::model_t &model) const {
    for (unsigned int flag = ModelFeatureFilterFlags::FeatureFirst;
         flag <= ModelFeatureFilterFlags::FeatureLast; flag <<= 1U) {
        if (m_featureFilterFlags & flag && model.features & flag) return true;
    }

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

    auto models = models_manager::instance()->models(m_lang, m_pack);

    updateDownloading(models);

    auto phase = getFilter();

    unsigned int existing_not_generic_feature_flags = 0;

    if (phase.isEmpty()) {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (roleFilterPass(model)) {
                if (featureFilterPass(model)) {
                    items.push_back(makeItem(model));
                }
            }
        });
    } else {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (roleFilterPass(model)) {
                if (featureFilterPass(model) &&
                    (model.name.contains(phase, Qt::CaseInsensitive) ||
                     model.lang_id.contains(phase, Qt::CaseInsensitive) ||
                     model.trg_lang_id.contains(phase, Qt::CaseInsensitive) ||
                     QStringLiteral("%1-%2")
                         .arg(model.lang_id, model.trg_lang_id)
                         .contains(phase, Qt::CaseInsensitive))) {
                    items.push_back(makeItem(model));
                }
            }
        });
    }

    m_disabledFeatureFilterFlags = 0;
    for (unsigned int flag = ModelFeatureFilterFlags::FeatureSttStart;
         flag != 0 && flag <= ModelFeatureFilterFlags::FeatureTtsEnd;
         flag <<= 1U) {
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

void ModelsListModel::setPack(const QString &pack) {
    if (pack != m_pack) {
        m_pack = pack;
        updateModel();
        emit packChanged();
    }
}

void ModelsListModel::setRoleFilterFlags(unsigned int roleFilterFlags) {
    if (roleFilterFlags != m_roleFilterFlags) {
        auto old_default = defaultFilters();

        m_roleFilterFlags = roleFilterFlags;

        if (m_roleFilterFlags & ModelRoleFilterFlags::RoleStt) {
            if (!flag_set_in_range_mask(
                    m_featureFilterFlags,
                    ModelFeatureFilterFlags::FeatureEngineSttStart,
                    ModelFeatureFilterFlags::FeatureEngineSttEnd)) {
                m_featureFilterFlags |=
                    ModelFeatureFilterFlags::FeatureAllSttEngines;
                emit featureFilterFlagsChanged();
            }
        }
        if (m_roleFilterFlags & ModelRoleFilterFlags::RoleTts) {
            if (!flag_set_in_range_mask(
                    m_featureFilterFlags,
                    ModelFeatureFilterFlags::FeatureEngineTtsStart,
                    ModelFeatureFilterFlags::FeatureEngineTtsEnd)) {
                m_featureFilterFlags |=
                    ModelFeatureFilterFlags::FeatureAllTtsEngines;
                emit featureFilterFlagsChanged();
            }
        }

        setFeatureFilterFlags(m_featureFilterFlags &
                              ~(ModelFeatureFilterFlags::FeatureAdditional));

        updateModel();

        emit roleFilterFlagsChanged();

        if (old_default != defaultFilters()) emit defaultFiltersChanged();
    }
}

void ModelsListModel::setFeatureFilterFlags(unsigned int featureFilterFlags) {
    if (featureFilterFlags != m_featureFilterFlags) {
        auto old_default = defaultFilters();

        m_featureFilterFlags = featureFilterFlags;

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

unsigned int ModelsListModel::countForRole(ModelRole role) const {
    models_manager::model_role_t mm_role = models_manager::model_role_t::stt;
    switch (role) {
        case ModelRole::Stt:
            mm_role = models_manager::model_role_t::stt;
            break;
        case ModelRole::Tts:
            mm_role = models_manager::model_role_t::tts;
            break;
        case ModelRole::Mnt:
            mm_role = models_manager::model_role_t::mnt;
            break;
        case ModelRole::Ttt:
            mm_role = models_manager::model_role_t::ttt;
            break;
    }

    auto v = models_manager::instance()->count(m_lang, mm_role);

    return v;
}

ModelsListItem::ModelsListItem(
    const QString &id, QString name, QString lang_id, QString pack_id,
    int pack_count, int pack_available_count, ModelsListModel::ModelRole role,
    License license, DownloadInfo download_info, bool available, bool dl_multi,
    bool dl_off, unsigned int features, int score, bool default_for_lang,
    bool downloading, double progress, QObject *parent)
    : SelectableItem{parent},
      m_id{id},
      m_name{std::move(name)},
      m_lang_id{std::move(lang_id)},
      m_pack_id{std::move(pack_id)},
      m_pack_count{pack_count},
      m_pack_available_count{pack_available_count},
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
    names[PackIdRole] = QByteArrayLiteral("pack_id");
    names[PackCountRole] = QByteArrayLiteral("pack_count");
    names[PackAvailableCountRole] = QByteArrayLiteral("pack_available_count");
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
        case PackIdRole:
            return pack_id();
        case PackCountRole:
            return pack_count();
        case PackAvailableCountRole:
            return pack_available_count();
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
        default:
            break;
    }

    return {};
}

void ModelsListItem::update(const ModelsListItem *item) {
    if (m_downloading != item->downloading() ||
        m_dl_multi != item->dl_multi() || m_dl_off != item->dl_off() ||
        m_available != item->available() || m_progress != item->progress() ||
        m_pack_available_count != item->pack_available_count() ||
        m_default_for_lang != item->default_for_lang()) {
        m_downloading = item->downloading();
        m_available = item->available();
        m_dl_multi = item->dl_multi();
        m_dl_off = item->dl_off();
        m_progress = item->progress();
        m_default_for_lang = item->default_for_lang();
        m_dl_multi = item->dl_multi();
        m_dl_off = item->dl_off();
        m_pack_available_count = item->pack_available_count();

        emit itemDataChanged();
    }
}

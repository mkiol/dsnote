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

ListItem *ModelsListModel::makeItem(const models_manager::model_t &model) {
    auto role = [&] {
        switch (models_manager::role_of_engine(model.engine)) {
            case models_manager::model_role::stt:
                return ModelRole::Stt;
            case models_manager::model_role::ttt:
                return ModelRole::Ttt;
            case models_manager::model_role::tts:
                return ModelRole::Tts;
            case models_manager::model_role::mnt:
                return ModelRole::Mnt;
        }
        throw std::runtime_error{"unsupported model engine"};
    }();

    return new ModelsListItem{
        /*id=*/model.id,
        /*name=*/role == ModelRole::Mnt
            ? QStringLiteral("%1 / %2-%3")
                  .arg(model.name, model.lang_id, model.trg_lang_id)
            : QStringLiteral("%1 / %2").arg(model.name, model.lang_id),
        /*langId=*/model.lang_id,
        /*role=*/role,
        /*available=*/model.available,
        /*score=*/model.score,
        /*default_for_lang=*/model.default_for_lang,
        /*downloading=*/model.downloading,
        /*progress=*/model.download_progress};
}

bool ModelsListModel::roleFilterPass(const models_manager::model_t &model) {
    if (m_roleFilter == ModelRoleFilter::AllModels) return true;

    switch (models_manager::role_of_engine(model.engine)) {
        case models_manager::model_role::stt:
            return m_roleFilter == ModelRoleFilter::SttModels;
        case models_manager::model_role::tts:
            return m_roleFilter == ModelRoleFilter::TtsModels;
        case models_manager::model_role::mnt:
            return m_roleFilter == ModelRoleFilter::MntModels;
        case models_manager::model_role::ttt:
            return m_roleFilter == ModelRoleFilter::OtherModels;
    }

    return false;
}

QList<ListItem *> ModelsListModel::makeItems() {
    QList<ListItem *> items;

    auto models = models_manager::instance()->models(m_lang);

    updateDownloading(models);

    auto phase = getFilter();

    if (phase.isEmpty()) {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (roleFilterPass(model)) items.push_back(makeItem(model));
        });
    } else {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (roleFilterPass(model) &&
                (model.name.contains(phase, Qt::CaseInsensitive) ||
                 model.lang_id.contains(phase, Qt::CaseInsensitive) ||
                 model.trg_lang_id.contains(phase, Qt::CaseInsensitive) ||
                 QStringLiteral("%1-%2")
                     .arg(model.lang_id, model.trg_lang_id)
                     .contains(phase, Qt::CaseInsensitive))) {
                items.push_back(makeItem(model));
            }
        });
    }

    return items;
}

void ModelsListModel::setLang(const QString &lang) {
    if (lang != m_lang) {
        m_lang = lang;
        updateModel();
        emit langChanged();
    }
}

void ModelsListModel::setRoleFilter(ModelRoleFilter roleFilter) {
    if (roleFilter != m_roleFilter) {
        m_roleFilter = roleFilter;
        updateModel();
        emit roleFilterChanged();
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
                               ModelsListModel::ModelRole role, bool available,
                               int score, bool default_for_lang,
                               bool downloading, double progress,
                               QObject *parent)
    : SelectableItem{parent},
      m_id{id},
      m_name{std::move(name)},
      m_langId{std::move(langId)},
      m_role{role},
      m_available{available},
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
    names[ScoreRole] = QByteArrayLiteral("score");
    names[DefaultRole] = QByteArrayLiteral("default_for_lang");
    names[DownloadingRole] = QByteArrayLiteral("downloading");
    names[ProgressRole] = QByteArrayLiteral("progress");
    return names;
}

QVariant ModelsListItem::data(int role) const {
    switch (role) {
        case IdRole:
            return id();
        case NameRole:
            return name();
        case LangIdRole:
            return langId();
        case ModelRole:
            return modelRole();
        case AvailableRole:
            return available();
        case ScoreRole:
            return score();
        case DefaultRole:
            return default_for_lang();
        case DownloadingRole:
            return downloading();
        case ProgressRole:
            return progress();
    }

    return {};
}

void ModelsListItem::update(const ModelsListItem *item) {
    if (m_downloading != item->downloading() ||
        m_available != item->available() || m_progress != item->progress()) {
        m_downloading = item->downloading();
        m_available = item->available();
        m_progress = item->progress();
        emit itemDataChanged();
    }
}

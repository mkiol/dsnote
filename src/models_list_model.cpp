/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "models_list_model.h"

#include <QDebug>
#include <QList>
#include <algorithm>

ModelsListModel::ModelsListModel(models_manager &manager, QObject *parent)
    : SelectableItemModel{new ModelsListItem, parent}, m_manager{manager} {
    connect(
        &m_manager, &models_manager::models_changed, this,
        [this] { updateModel(); }, Qt::QueuedConnection);
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

void ModelsListModel::setShowExperimental(bool value) {
    if (m_showExperimental != value) {
        m_showExperimental = value;
        emit showExperimentalChanged();
        updateModel();
    }
}

void ModelsListModel::beforeUpdate(const QList<ListItem *> &oldItems,
                                   const QList<ListItem *> &newItems) {
    const auto &[it, _] = std::mismatch(
        oldItems.cbegin(), oldItems.cend(), newItems.cbegin(), newItems.cend(),
        [](const ListItem *a, const ListItem *b) {
            const auto aa = static_cast<const ModelsListItem *>(a);
            const auto bb = static_cast<const ModelsListItem *>(b);
            return aa->available() == bb->available() &&
                   aa->downloading() == bb->downloading() &&
                   aa->progress() == bb->progress();
        });
    m_changedItem = std::distance(oldItems.cbegin(), it);
}

ListItem *ModelsListModel::makeItem(const models_manager::model_t &model) {
    return new ModelsListItem{model.id,
                              QString{"%1 / %2"}.arg(model.name, model.lang_id),
                              model.lang_id,
                              model.available,
                              model.experimental,
                              model.downloading,
                              model.download_progress};
}

QList<ListItem *> ModelsListModel::makeItems() {
    QList<ListItem *> items;

    const auto models = m_manager.models(m_lang);

    updateDownloading(models);

    const auto phase = getFilter();

    if (phase.isEmpty()) {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (m_showExperimental || !model.experimental)
                items << makeItem(model);
        });
    } else {
        std::for_each(models.cbegin(), models.cend(), [&](const auto &model) {
            if (m_showExperimental || !model.experimental) {
                if (model.name.contains(phase, Qt::CaseInsensitive) ||
                    model.lang_id.contains(phase, Qt::CaseInsensitive)) {
                    items << makeItem(model);
                }
            }
        });
    }

    std::sort(
        items.begin(), items.end(), [](const ListItem *a, const ListItem *b) {
            const auto aa = static_cast<const ModelsListItem *>(a);
            const auto bb = static_cast<const ModelsListItem *>(b);
            if (aa->experimental() == bb->experimental()) {
                return aa->id().compare(bb->id(), Qt::CaseInsensitive) < 0;
            }
            return !aa->experimental() && bb->experimental();
        });
    return items;
}

void ModelsListModel::setLang(const QString &lang) {
    if (lang != m_lang) {
        m_lang = lang;
        updateModel();
        emit langChanged();
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

ModelsListItem::ModelsListItem(const QString &id, const QString &name,
                               const QString &langId, bool available,
                               bool experimental, bool downloading,
                               double progress, QObject *parent)
    : SelectableItem{parent},
      m_id{id},
      m_name{name},
      m_langId{langId},
      m_available{available},
      m_experimental{experimental},
      m_downloading{downloading},
      m_progress{progress} {
    m_selectable = false;
}

QHash<int, QByteArray> ModelsListItem::roleNames() const {
    QHash<int, QByteArray> names;
    names[IdRole] = "id";
    names[NameRole] = "name";
    names[LangIdRole] = "lang_id";
    names[AvailableRole] = "available";
    names[ExperimentalRole] = "experimental";
    names[DownloadingRole] = "downloading";
    names[ProgressRole] = "progress";
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
        case AvailableRole:
            return available();
        case ExperimentalRole:
            return experimental();
        case DownloadingRole:
            return downloading();
        case ProgressRole:
            return progress();
        default:
            return {};
    }
}

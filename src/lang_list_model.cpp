/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "lang_list_model.h"

#include <QDebug>
#include <QList>
#include <algorithm>

LangListModel::LangListModel(models_manager& manager, QObject *parent) :
    SelectableItemModel{new LangListItem, parent},
    m_manager{manager}
{
    connect(&m_manager, &models_manager::models_changed, this, [this]{ updateModel(); }, Qt::QueuedConnection);
    connect(this, &ItemModel::busyChanged, this,
        [this] {
            if (!isBusy() && m_changedItem > -1) {
                emit itemChanged(m_changedItem);
                m_changedItem = -1;
            }
        }, Qt::QueuedConnection);
}

LangListModel::~LangListModel()
{
    m_worker.reset();
}

void LangListModel::setShowExperimental(bool value)
{
    if (m_showExperimental != value) {
        m_showExperimental = value;
        emit showExperimentalChanged();
        updateModel();
    }
}

void LangListModel::beforeUpdate(const QList<ListItem*> &oldItems, const QList<ListItem*> &newItems)
{
    const auto& [it, _] = std::mismatch(oldItems.cbegin(), oldItems.cend(), newItems.cbegin(), newItems.cend(),
                              [](const ListItem* a, const ListItem* b) {
                                  const auto aa = static_cast<const LangListItem*>(a);
                                  const auto bb = static_cast<const LangListItem*>(b);
                                  return aa->available() == bb->available() &&
                                         aa->downloading() == bb->downloading() &&
                                         aa->progress() == bb->progress();
                              });
    m_changedItem = std::distance(oldItems.cbegin(), it);
}

QList<ListItem*> LangListModel::makeItems()
{
    QList<ListItem*> items;

    const auto langs = m_manager.langs();

    updateDownloading(langs);

    const auto phase = getFilter();

    const auto make_item = [](const auto& model) {
        return new LangListItem {
            model.id,
            QString{"%1 / %2"}.arg(model.name, model.lang_id),
            model.lang_id,
            model.available,
            model.experimental,
            model.downloading,
            model.download_progress
        };
    };

    if (phase.isEmpty()) {
        std::for_each(langs.cbegin(), langs.cend(), [&](const auto& model) {
            if (m_showExperimental || !model.experimental) items << make_item(model);
        });
    } else {
        std::for_each(langs.cbegin(), langs.cend(), [&](const auto& model) {
            if (m_showExperimental || !model.experimental) {
                if (model.name.contains(phase, Qt::CaseInsensitive) ||
                        model.lang_id.contains(phase, Qt::CaseInsensitive)) {
                    items << make_item(model);
                }
            }
        });
    }

    std::sort(items.begin(), items.end(), [](const ListItem *a, const ListItem *b) {
        const auto aa = static_cast<const LangListItem*>(a);
        const auto bb = static_cast<const LangListItem*>(b);
        if (aa->experimental() == bb->experimental()) {
            return aa->id().compare(bb->id(), Qt::CaseInsensitive) < 0;
        }
        return !aa->experimental() && bb->experimental();
    });

    return items;
}

void LangListModel::updateDownloading(const std::vector<models_manager::lang_t> &models)
{
    const bool new_downloading = !std::none_of(models.cbegin(), models.cend(),
                                               [](const auto& model) { return model.downloading; });
    if (m_downloading != new_downloading) {
        m_downloading = new_downloading;
        emit downloadingChanged();
    }
}

LangListItem::LangListItem(const QString &id,
        const QString &name,
        const QString &langId,
        bool available,
        bool experimental,
        bool downloading,
        double progress,
        QObject *parent) :
    SelectableItem{parent},
    m_id{id},
    m_name{name},
    m_langId{langId},
    m_available{available},
    m_experimental{experimental},
    m_downloading{downloading},
    m_progress{progress}
{
    m_selectable = false;
}

QHash<int, QByteArray> LangListItem::roleNames() const
{
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

QVariant LangListItem::data(int role) const
{
    switch(role) {
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

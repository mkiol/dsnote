/* Copyright (C) 2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "langs_list_model.h"

#include <QDebug>
#include <QList>
#include <algorithm>

LangsListModel::LangsListModel(models_manager &manager, QObject *parent)
    : SelectableItemModel{new LangsListItem, parent}, m_manager{manager} {
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

LangsListModel::~LangsListModel() { m_worker.reset(); }

void LangsListModel::beforeUpdate(const QList<ListItem *> &oldItems,
                                  const QList<ListItem *> &newItems) {
    const auto &[it, _] = std::mismatch(
        oldItems.cbegin(), oldItems.cend(), newItems.cbegin(), newItems.cend(),
        [](const ListItem *a, const ListItem *b) {
            const auto aa = static_cast<const LangsListItem *>(a);
            const auto bb = static_cast<const LangsListItem *>(b);
            return aa->available() == bb->available() &&
                   aa->downloading() == bb->downloading();
        });
    m_changedItem = std::distance(oldItems.cbegin(), it);
}

ListItem *LangsListModel::makeItem(const models_manager::lang_t &lang) {
    return new LangsListItem{lang.id,
                             QStringLiteral("%1 / %2").arg(lang.name, lang.id),
                             lang.available, lang.downloading};
}

QList<ListItem *> LangsListModel::makeItems() {
    QList<ListItem *> items;

    const auto langs = m_manager.langs();

    updateDownloading(langs);

    const auto phase = getFilter();

    if (phase.isEmpty()) {
        std::transform(langs.cbegin(), langs.cend(), std::back_inserter(items),
                       [&](const auto &lang) { return makeItem(lang); });
    } else {
        std::for_each(langs.cbegin(), langs.cend(), [&](const auto &lang) {
            if (lang.name.contains(phase, Qt::CaseInsensitive) ||
                lang.id.contains(phase, Qt::CaseInsensitive)) {
                items.push_back(makeItem(lang));
            }
        });
    }

    return items;
}

void LangsListModel::updateDownloading(
    const std::vector<models_manager::lang_t> &langs) {
    const bool new_downloading =
        !std::none_of(langs.cbegin(), langs.cend(),
                      [](const auto &lang) { return lang.downloading; });
    if (m_downloading != new_downloading) {
        m_downloading = new_downloading;
        emit downloadingChanged();
    }
}

LangsListItem::LangsListItem(const QString &id, const QString &name,
                             bool available, bool downloading, QObject *parent)
    : SelectableItem{parent},
      m_id{id},
      m_name{name},
      m_available{available},
      m_downloading{downloading} {
    m_selectable = false;
}

QHash<int, QByteArray> LangsListItem::roleNames() const {
    QHash<int, QByteArray> names;
    names[IdRole] = "id";
    names[NameRole] = "name";
    names[AvailableRole] = "available";
    names[DownloadingRole] = "downloading";
    return names;
}

QVariant LangsListItem::data(int role) const {
    switch (role) {
        case IdRole:
            return id();
        case NameRole:
            return name();
        case AvailableRole:
            return available();
        case DownloadingRole:
            return downloading();
        default:
            return {};
    }
}

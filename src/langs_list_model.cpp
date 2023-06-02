/* Copyright (C) 2022-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "langs_list_model.h"

#include <QDebug>
#include <QList>
#include <algorithm>

LangsListModel::LangsListModel(QObject *parent)
    : SelectableItemModel{new LangsListItem, parent} {
    connect(
        models_manager::instance(), &models_manager::models_changed, this,
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

size_t LangsListModel::firstChangedItemIdx(const QList<ListItem *> &oldItems,
                                           const QList<ListItem *> &newItems) {
    const auto &[it, _] = std::mismatch(
        oldItems.cbegin(), oldItems.cend(), newItems.cbegin(), newItems.cend(),
        [](const ListItem *a, const ListItem *b) {
            const auto *aa = qobject_cast<const LangsListItem *>(a);
            const auto *bb = qobject_cast<const LangsListItem *>(b);
            return aa->id() == bb->id() && aa->available() == bb->available() &&
                   aa->downloading() == bb->downloading();
        });

    auto idx = std::distance(oldItems.cbegin(), it);

    m_changedItem = static_cast<int>(idx);

    return idx;
}

ListItem *LangsListModel::makeItem(const models_manager::lang_t &lang) {
    return new LangsListItem{
        /*id=*/lang.id,
        /*name=*/QStringLiteral("%1 / %2").arg(lang.name, lang.id),
        /*name_en=*/lang.name_en, /*available=*/lang.available,
        /*downloading=*/lang.downloading};
}

QList<ListItem *> LangsListModel::makeItems() {
    QList<ListItem *> items;

    auto langs = models_manager::instance()->langs();

    updateDownloading(langs);

    auto phase = getFilter();

    if (phase.isEmpty()) {
        std::transform(langs.cbegin(), langs.cend(), std::back_inserter(items),
                       [&](const auto &lang) { return makeItem(lang); });
    } else {
        std::for_each(langs.cbegin(), langs.cend(), [&](const auto &lang) {
            if (lang.name.contains(phase, Qt::CaseInsensitive) ||
                lang.id.contains(phase, Qt::CaseInsensitive) ||
                lang.name_en.contains(phase, Qt::CaseInsensitive)) {
                items.push_back(makeItem(lang));
            }
        });
    }

    return items;
}

void LangsListModel::updateDownloading(
    const std::vector<models_manager::lang_t> &langs) {
    bool new_downloading =
        !std::none_of(langs.cbegin(), langs.cend(),
                      [](const auto &lang) { return lang.downloading; });
    if (m_downloading != new_downloading) {
        m_downloading = new_downloading;
        emit downloadingChanged();
    }
}

LangsListItem::LangsListItem(const QString &id, const QString &name,
                             const QString &name_en, bool available,
                             bool downloading, QObject *parent)
    : SelectableItem{parent},
      m_id{id},
      m_name{name},
      m_name_en{name_en},
      m_available{available},
      m_downloading{downloading} {
    m_selectable = false;
}

QHash<int, QByteArray> LangsListItem::roleNames() const {
    QHash<int, QByteArray> names;
    names[IdRole] = "id";
    names[NameRole] = "name";
    names[NameEnRole] = "name_en";
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
        case NameEnRole:
            return name_en();
        case AvailableRole:
            return available();
        case DownloadingRole:
            return downloading();
        default:
            return {};
    }
}

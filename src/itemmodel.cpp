/* Copyright (C) 2018 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QDebug>

#include "itemmodel.h"

ItemWorker::ItemWorker(ItemModel *model, const QString &data) :
    QThread(model),
    data(data),
    model(model)
{
}

ItemWorker::~ItemWorker()
{
    requestInterruption();
    wait();
}

void ItemWorker::run()
{
    if (!isInterruptionRequested())
        items = model->makeItems();
    if (!isInterruptionRequested())
        model->postMakeItems(items);
}

ItemModel::ItemModel(ListItem *prototype, QObject *parent) :
    ListModel(prototype, parent)
{
}

ItemModel::~ItemModel()
{
    m_worker.reset();
}

void ItemModel::updateModel(const QString &data)
{
    if (m_worker && m_worker->isRunning())
        return;

    setBusy(true);
    m_worker = std::unique_ptr<ItemWorker>(new ItemWorker{this, data});
    connect(m_worker.get(), &QThread::finished, this, &ItemModel::workerDone);
    m_worker->start(QThread::IdlePriority);
}

void ItemModel::postMakeItems(const QList<ListItem*>&)
{
}

void ItemModel::clear()
{
    if (m_list.length() == 0)
        return;

    removeRows(0,rowCount());
    emit countChanged();
}

void ItemModel::workerDone()
{
    auto worker = qobject_cast<ItemWorker*>(sender());
    if (worker) {
        int old_l = m_list.length();

        if (m_list.length() != 0)
            removeRows(0,rowCount());

        if (!worker->items.isEmpty())
            appendRows(worker->items);
        else
            qWarning() << "No items";

        if (old_l != m_list.length())
            emit countChanged();

        m_worker.reset(nullptr);
    }

    setBusy(false);
}

int ItemModel::getCount() const
{
    return m_list.length();
}

bool ItemModel::isBusy() const
{
    return m_busy;
}

void ItemModel::setBusy(bool busy)
{
    if (busy != m_busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void SelectableItem::setSelected(bool value)
{
    if (m_selected != value) {
        m_selected = value;
        emit dataChanged();
    }
}

SelectableItemModel::SelectableItemModel(SelectableItem *prototype, QObject *parent) :
    ItemModel(prototype, parent)
{
}

int SelectableItemModel::selectedCount()
{
    return m_selectedCount;
}

int SelectableItemModel::selectableCount()
{
    return m_selectableCount;
}

void SelectableItemModel::clear()
{
    ItemModel::clear();

    m_selectedCount = 0;
    emit selectedCountChanged();
}

void SelectableItemModel::postMakeItems(const QList<ListItem*> &items)
{
    int count = 0;
    for (const auto item : items) {
        auto sitem = qobject_cast<SelectableItem*>(item);
        if (sitem->selectable())
            count++;
    }

    if (count != m_selectableCount) {
        m_selectableCount = count;
        emit selectableCountChanged();
    }
}

void SelectableItemModel::setFilter(const QString &filter)
{
    if (m_filter != filter) {
        m_filter = filter;
        emit filterChanged();

        updateModel();
    }
}

void SelectableItemModel::setFilterNoUpdate(const QString &filter)
{
    if (m_filter != filter) {
        m_filter = filter;
        emit filterChanged();
    }
}

const QString& SelectableItemModel::getFilter() const
{
    return m_filter;
}

void SelectableItemModel::setSelected(int index, bool value)
{
    if (index >= m_list.length()) {
        qWarning() << "Index is invalid";
        return;
    }

    auto item = qobject_cast<SelectableItem*>(m_list.at(index));

    if (item->selectable()) {
        bool cvalue = item->selected();

        if (cvalue != value) {
            item->setSelected(value);

            if (value)
                m_selectedCount++;
            else
                m_selectedCount--;

            emit selectedCountChanged();
        }
    }
}

void SelectableItemModel::setAllSelected(bool value)
{
    if (m_list.isEmpty())
        return;

    int c = m_selectedCount;

    foreach (auto li, m_list) {
        auto item = qobject_cast<SelectableItem*>(li);
        if (item->selectable()) {
            bool cvalue = item->selected();

            if (cvalue != value) {
                item->setSelected(value);

                if (value)
                    m_selectedCount++;
                else
                    m_selectedCount--;
            }
        }
    }

    if (c != m_selectedCount)
         emit selectedCountChanged();
}

QVariantList SelectableItemModel::selectedItems()
{
    return {};
}

void SelectableItemModel::workerDone()
{
    if (m_worker && m_worker->data != m_filter) {
        updateModel(m_filter);
    } else {
        ItemModel::workerDone();
    }
}

void SelectableItemModel::updateModel(const QString &)
{
    setAllSelected(false);
    ItemModel::updateModel(m_filter);
}

/*
 * Author: Christophe Dumez <dchris@gmail.com>
 * License: Public domain (No attribution required)
 * Website: http://cdumez.blogspot.com/
 * Version: 1.1
 */

#include "listmodel.h"

ListModel::ListModel(ListItem* prototype, QObject *parent) :
    QAbstractListModel(parent), m_prototype(prototype)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#else
    setRoleNames(m_prototype->roleNames());
#endif
}

QHash<int, QByteArray>ListModel::roleNames() const
{
    return m_prototype->roleNames();
}

int ListModel::rowCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return m_list.size();
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= m_list.size())
    return QVariant();
  return m_list.at(index.row())->data(role);
}

ListModel::~ListModel() {
  delete m_prototype;
  clear();
}

int ListModel::indexFromId(const QString& id) const
{
    auto item = find(id);

    if (item == 0) {
        return -1;
    }

    auto indx = indexFromItem(item);
    return indx.row();
}

void ListModel::appendRow(ListItem *item)
{
  appendRows(QList<ListItem*>() << item);
}

void ListModel::appendRows(const QList<ListItem *> &items)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount()+items.size()-1);
  foreach(ListItem *item, items) {
    connect(item, SIGNAL(dataChanged()), SLOT(handleItemChange()));
    m_list.append(item);
  }
  endInsertRows();
}

void ListModel::insertRow(int row, ListItem *item)
{
  beginInsertRows(QModelIndex(), row, row);
  connect(item, SIGNAL(dataChanged()), SLOT(handleItemChange()));
  m_list.insert(row, item);
  endInsertRows();
}

void ListModel::moveRow(int orig, int dest, const QModelIndex &parent)
{
    beginMoveRows(parent, orig, orig, parent, dest);
    m_list.move(orig, dest);
    endMoveRows();
}

void ListModel::handleItemChange()
{
  ListItem* item = static_cast<ListItem*>(sender());
  QModelIndex index = indexFromItem(item);
  if(index.isValid())
    emit dataChanged(index, index);
}

void ListModel::handleItemChangeById(const QString &id)
{
  ListItem* item = find(id);
  if (item) {
      QModelIndex index = indexFromItem(item);
      if(index.isValid())
        emit dataChanged(index, index);
  }
}

ListItem * ListModel::find(const QString &id) const
{
  foreach(ListItem* item, m_list) {
    if(item->id() == id) return item;
  }
  return nullptr;
}

QModelIndex ListModel::indexFromItem(const ListItem *item) const
{
  Q_ASSERT(item);
  for(int row=0; row<m_list.size(); ++row) {
    if(m_list.at(row) == item) return index(row);
  }
  return QModelIndex();
}

void ListModel::clear()
{
  qDeleteAll(m_list);
  m_list.clear();
}

bool ListModel::removeRow(int row, const QModelIndex &parent)
{
  Q_UNUSED(parent);
  if(row < 0 || row >= m_list.size()) return false;
  beginRemoveRows(QModelIndex(), row, row);
  delete m_list.takeAt(row);
  endRemoveRows();
  return true;
}

bool ListModel::removeRows(int row, int count, const QModelIndex &parent)
{
  Q_UNUSED(parent);
  if(row < 0 || (row+count) > m_list.size()) return false;
  beginRemoveRows(QModelIndex(), row, row+count-1);

  for(int i=0; i<count; ++i) {
    delete m_list.takeAt(row);
    //m_list.takeAt(row);
  }
  endRemoveRows();
  return true;
}

bool ListModel::removeRowsNoDeleteItems(int row, int count, const QModelIndex &parent)
{
  Q_UNUSED(parent);
  if(row < 0 || (row+count) > m_list.size()) return false;
  beginRemoveRows(QModelIndex(), row, row+count-1);
  m_list.clear();
  endRemoveRows();
  return true;
}

ListItem * ListModel::takeRow(int row)
{
  beginRemoveRows(QModelIndex(), row, row);
  ListItem* item = m_list.takeAt(row);
  endRemoveRows();
  return item;
}

ListItem * ListModel::readRow(int row)
{
  return m_list.at(row);
}

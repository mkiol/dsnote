/* Copyright (C) 2019 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DIRMODEL_H
#define DIRMODEL_H

#include <QDir>

#include "itemmodel.h"

class DirItem : public ListItem {
    Q_OBJECT
   public:
    enum Roles { NameRole = Qt::DisplayRole, IdRole = Qt::UserRole, PathRole };

    DirItem(QObject *parent = nullptr) : ListItem(parent) {}
    explicit DirItem(const QString &id, const QString &name,
                     const QString &path, QObject *parent = nullptr);
    QVariant data(int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    inline QString id() const override { return m_id; }
    inline QString name() const { return m_name; }
    inline QString path() const { return m_path; }

   private:
    QString m_id;
    QString m_name;
    QString m_path;
};

class DirModel : public ItemModel {
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ getCurrentPath WRITE setCurrentPath
                   NOTIFY currentDirChanged)
    Q_PROPERTY(QString currentName READ getCurrentName NOTIFY currentDirChanged)
   public:
    explicit DirModel(QObject *parent = nullptr);
    void setCurrentPath(const QString &path);
    QString getCurrentPath() const;
    QString getCurrentName() const;
    Q_INVOKABLE bool isCurrentWritable() const;
    Q_INVOKABLE void changeToRemovable();
    Q_INVOKABLE void changeToHome();

   signals:
    void currentDirChanged();

   private:
    QDir m_dir;
    QList<ListItem *> makeItems() override;
};

#endif  // DIRMODEL_H

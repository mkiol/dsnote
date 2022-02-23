/* Copyright (C) 2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LANGSLISTMODEL_H
#define LANGSLISTMODEL_H

#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <optional>

#include "itemmodel.h"
#include "listmodel.h"
#include "models_manager.h"

class LangsListModel : public SelectableItemModel {
    Q_OBJECT
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
   public:
    explicit LangsListModel(models_manager &manager, QObject *parent = nullptr);
    ~LangsListModel();

   signals:
    void itemChanged(int idx);
    void downloadingChanged();

   private:
    models_manager &m_manager;
    int m_changedItem = -1;
    bool m_downloading = false;

    QList<ListItem *> makeItems() override;
    static ListItem *makeItem(const models_manager::lang_t &lang);
    void beforeUpdate(const QList<ListItem *> &oldItems,
                      const QList<ListItem *> &newItems) override;
    inline bool downloading() const { return m_downloading; }
    void updateDownloading(const std::vector<models_manager::lang_t> &langs);
};

class LangsListItem : public SelectableItem {
    Q_OBJECT
   public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        IdRole = Qt::UserRole,
        AvailableRole,
        DownloadingRole
    };

    LangsListItem(QObject *parent = nullptr) : SelectableItem{parent} {}
    LangsListItem(const QString &id, const QString &name,
                  bool available = false, bool downloading = false,
                  QObject *parent = nullptr);
    QVariant data(int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    inline QString id() const override { return m_id; }
    inline QString name() const { return m_name; }
    inline bool available() const { return m_available; }
    inline bool downloading() const { return m_downloading; }

   private:
    QString m_id;
    QString m_name;
    bool m_available = false;
    bool m_downloading = false;
};

#endif  // LANGSLISTMODEL_H

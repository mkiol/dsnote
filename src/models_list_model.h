/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MODELSLISTMODEL_H
#define MODELSLISTMODEL_H

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

class ModelsListModel : public SelectableItemModel {
    Q_OBJECT
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)
    Q_PROPERTY(QString lang READ lang WRITE setLang NOTIFY langChanged)
   public:
    explicit ModelsListModel(models_manager &manager,
                             QObject *parent = nullptr);
    ~ModelsListModel() override;

   signals:
    void itemChanged(int idx);
    void downloadingChanged();
    void langChanged();

   private:
    models_manager &m_manager;
    int m_changedItem = -1;
    bool m_downloading = false;
    QString m_lang;

    QList<ListItem *> makeItems() override;
    static ListItem *makeItem(const models_manager::model_t &model);
    void beforeUpdate(const QList<ListItem *> &oldItems,
                      const QList<ListItem *> &newItems) override;
    inline bool downloading() const { return m_downloading; }
    inline QString lang() const { return m_lang; }
    void setLang(const QString &lang);
    void updateDownloading(const std::vector<models_manager::model_t> &models);
};

class ModelsListItem : public SelectableItem {
    Q_OBJECT
   public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        IdRole = Qt::UserRole,
        LangIdRole,
        AvailableRole,
        ScoreRole,
        DownloadingRole,
        ProgressRole
    };

    ModelsListItem(QObject *parent = nullptr) : SelectableItem{parent} {}
    ModelsListItem(const QString &id, const QString &name,
                   const QString &langId = {}, bool available = true,
                   int score = 2, bool downloading = false,
                   double progress = 0.0, QObject *parent = nullptr);
    QVariant data(int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    inline QString id() const override { return m_id; }
    inline QString name() const { return m_name; }
    inline QString langId() const { return m_langId; }
    inline bool available() const { return m_available; }
    inline int score() const { return m_score; }
    inline bool downloading() const { return m_downloading; }
    inline double progress() const { return m_progress; }

   private:
    QString m_id;
    QString m_name;
    QString m_langId;
    bool m_available = false;
    int m_score = 2;
    bool m_downloading = false;
    double m_progress = 0.0;
};

#endif  // MODELSLISTMODEL_H

/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LANGLISTMODEL_H
#define LANGLISTMODEL_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QDebug>
#include <QByteArray>
#include <QVariant>
#include <QVariantList>
#include <QUrl>
#include <optional>

#include "listmodel.h"
#include "itemmodel.h"
#include "models_manager.h"

class LangListModel : public SelectableItemModel
{
    Q_OBJECT
    Q_PROPERTY (bool showExperimental READ showExperimental WRITE setShowExperimental NOTIFY showExperimentalChanged)
    Q_PROPERTY (bool downloading READ downloading NOTIFY downloadingChanged)
public:
    explicit LangListModel(models_manager& manager, QObject *parent = nullptr);
    ~LangListModel();

signals:
    void itemChanged(int idx);
    void showExperimentalChanged();
    void downloadingChanged();

private:
    models_manager& m_manager;
    int m_changedItem = -1;
    bool m_showExperimental = true;
    bool m_downloading = false;

    QList<ListItem*> makeItems() override;
    void beforeUpdate(const QList<ListItem*> &oldItems, const QList<ListItem*> &newItems) override;
    void setShowExperimental(bool value);
    inline bool showExperimental() const { return m_showExperimental; }
    inline bool downloading() const { return m_downloading; }
    void updateDownloading(const std::vector<models_manager::lang_t> &models);
};

class LangListItem : public SelectableItem
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        IdRole = Qt::UserRole,
        LangIdRole,
        AvailableRole,
        ExperimentalRole,
        DownloadingRole,
        ProgressRole
    };

public:
    LangListItem(QObject *parent = nullptr): SelectableItem{parent} {}
    explicit LangListItem(const QString &id,
                      const QString &name,
                      const QString &langId,
                      bool available,
                      bool experimental,
                      bool downloading,
                      double progress,
                      QObject *parent = nullptr);
    QVariant data(int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    inline QString id() const override { return m_id; }
    inline QString name() const { return m_name; }
    inline QString langId() const { return m_langId; }
    inline bool available() const { return m_available; }
    inline bool experimental() const { return m_experimental; }
    inline bool downloading() const { return m_downloading; }
    inline bool progress() const { return m_progress; }

private:
    QString m_id;
    QString m_name;
    QString m_langId;
    bool m_available = false;
    bool m_experimental = false;
    bool m_downloading = false;
    double m_progress = 0.0;
};

#endif // LANGLISTMODEL_H

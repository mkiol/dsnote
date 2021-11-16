/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef STT_CONFIG_H
#define STT_CONFIG_H

#include <QObject>
#include <QString>
#include <QVariantList>

#include "models_manager.h"

class stt_config : public QObject
{
    Q_OBJECT

    Q_PROPERTY (QVariantList all_models READ all_models NOTIFY models_changed)
    Q_PROPERTY (QVariantList all_experimental_models READ all_experimental_models NOTIFY models_changed)
    Q_PROPERTY (QVariantList available_models READ available_models NOTIFY models_changed)
    Q_PROPERTY (bool busy READ busy NOTIFY busy_changed)

public:
    explicit stt_config(QObject *parent = nullptr);

    Q_INVOKABLE void download_model(const QString &id);
    Q_INVOKABLE void delete_model(const QString &id);

signals:
    void models_changed();
    void model_download_progress(const QString &id, double progress);
    void busy_changed();
    void default_model_changed();

private:
    inline static const QString DBUS_SERVICE_NAME{"org.mkiol.Stt"};
    inline static const QString DBUS_SERVICE_PATH{"/"};
    static const int SUCCESS = 0;
    static const int FAILURE = -1;

    models_manager manager;

    QVariantList all_models() const;
    QVariantList all_experimental_models() const;
    QVariantList available_models() const;
    void handle_models_changed();
    inline bool busy() const { return manager.busy(); }
    void reload();
};

#endif // STT_CONFIG_H

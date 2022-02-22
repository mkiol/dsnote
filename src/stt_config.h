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

#include "langs_list_model.h"
#include "models_list_model.h"
#include "models_manager.h"

class stt_config : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList available_models READ available_models NOTIFY
                   models_changed)
    Q_PROPERTY(LangsListModel *langs_model READ langs_model CONSTANT)
    Q_PROPERTY(ModelsListModel *models_model READ models_model CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)
   public:
    explicit stt_config(QObject *parent = nullptr);

    Q_INVOKABLE void download_model(const QString &id);
    Q_INVOKABLE void cancel_model_download(const QString &id);
    Q_INVOKABLE void delete_model(const QString &id);
    Q_INVOKABLE double model_download_progress(const QString &id) const;

   signals:
    void models_changed();
    void model_download_progress_changed(const QString &id, double progress);
    void busy_changed();
    void default_model_changed();

   private:
    inline static const QString DBUS_SERVICE_NAME{"org.mkiol.Stt"};
    inline static const QString DBUS_SERVICE_PATH{"/"};
    static const int SUCCESS = 0;
    static const int FAILURE = -1;

    models_manager m_manager;
    LangsListModel m_langs_model;
    ModelsListModel m_models_model;

    QVariantList available_models() const;
    LangsListModel *langs_model();
    ModelsListModel *models_model();
    void handle_models_changed();
    inline bool busy() const { return m_manager.busy(); }
    void reload();
};

#endif  // STT_CONFIG_H

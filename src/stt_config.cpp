/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "stt_config.h"

#include <QDBusConnection>
#include <QDebug>
#include <algorithm>

#include "dbus_stt_inf.h"
#include "settings.h"

stt_config::stt_config(QObject *parent)
    : QObject{parent}, m_langs_model{m_manager}, m_models_model{m_manager} {
    connect(settings::instance(), &settings::models_dir_changed, this,
            &stt_config::reload, Qt::QueuedConnection);
    connect(settings::instance(), &settings::default_model_changed, this,
            [this] { emit default_model_changed(); });
    connect(&m_manager, &models_manager::models_changed, this,
            &stt_config::handle_models_changed, Qt::QueuedConnection);
    connect(&m_manager, &models_manager::busy_changed, this,
            [this] { emit busy_changed(); });
    connect(&m_manager, &models_manager::download_progress, this,
            [this](const QString &id, double progress) {
                qDebug() << "download_progress:" << id << progress;
                emit model_download_progress_changed(id, progress);
            });
}

void stt_config::handle_models_changed() {
    reload();
    emit models_changed();
}

ModelsListModel *stt_config::models_model() { return &m_models_model; }

LangsListModel *stt_config::langs_model() { return &m_langs_model; }

QVariantList stt_config::available_models() const {
    const auto available_models_map = m_manager.available_models();

    QVariantList list;
    std::transform(available_models_map.cbegin(), available_models_map.cend(),
                   std::back_inserter(list), [](const auto &model) {
                       return QStringList{model.id,
                                          QStringLiteral("%1 / %2").arg(
                                              model.name, model.lang_id)};
                   });

    return list;
}

void stt_config::download_model(const QString &id) {
    m_manager.download_model(id);
}

void stt_config::cancel_model_download(const QString &id) {
    m_manager.cancel_model_download(id);
}

void stt_config::delete_model(const QString &id) { m_manager.delete_model(id); }

double stt_config::model_download_progress(const QString &id) const {
    return m_manager.model_download_progress(id);
}

void stt_config::set_default_model_for_lang(const QString &model_id) {
    m_manager.set_default_model_for_lang(model_id);
}

void stt_config::reload() const {
    OrgMkiolSttInterface stt{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH,
                             QDBusConnection::sessionBus()};
    if (!stt.isValid()) {
        qWarning() << "cannot reload service because is's not running";
        return;
    }

    auto reply = stt.Reload();
    reply.waitForFinished();

    if (reply.argumentAt<0>() != SUCCESS)
        qWarning() << "cannot reload service, error was returned";
}

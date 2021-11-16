/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "stt_config.h"

#include <QDebug>
#include <QDBusConnection>
#include <algorithm>

#include "settings.h"
#include "dbus_stt_inf.h"

stt_config::stt_config(QObject *parent) :
    QObject{parent}
{
    connect(settings::instance(), &settings::models_dir_changed, this, &stt_config::reload, Qt::QueuedConnection);
    connect(settings::instance(), &settings::default_model_changed, this, [this]{ emit default_model_changed(); });
    connect(&manager, &models_manager::models_changed, this, &stt_config::handle_models_changed, Qt::QueuedConnection);
    connect(&manager, &models_manager::busy_changed, this, [this]{ emit busy_changed(); });
    connect(&manager, &models_manager::download_progress, this, [this](const QString &id, double progress) {
        qDebug() << "download_progress:" << id << progress;
        emit model_download_progress(id, progress);
    });
}

void stt_config::handle_models_changed()
{
    reload();
    emit models_changed();
}

QVariantList stt_config::all_models() const
{
    QVariantList list;

    const auto langs = manager.langs();

    std::for_each(langs.cbegin(), langs.cend(), [&](const auto& model) {
        if (model.experimental) return;
        list.push_back(QVariantList{
            model.id,
            model.lang_id,
            QString{"%1 / %2"}.arg(model.name, model.lang_id),
            model.available,
            model.downloading,
            model.download_progress});
    });

    return list;
}

QVariantList stt_config::all_experimental_models() const
{
    QVariantList list;

    const auto langs = manager.langs();

    std::for_each(langs.cbegin(), langs.cend(), [&](const auto& model) {
        if (!model.experimental) return;
        list.push_back(QVariantList{
            model.id,
            model.lang_id,
            QString{"%1 / %2"}.arg(model.name, model.lang_id),
            model.available,
            model.downloading,
            model.download_progress});
    });

    return list;
}

QVariantList stt_config::available_models() const
{
    const auto available_models_map = manager.available_models();

    QVariantList list;
    std::transform(available_models_map.cbegin(), available_models_map.cend(), std::back_inserter(list),
                   [](const auto &model) {
        return QStringList{model.id, QString{"%1 / %2"}.arg(model.name, model.lang_id)};
    });

    return list;
}

void stt_config::download_model(const QString& id)
{
    manager.download_model(id);
}

void stt_config::delete_model(const QString& id)
{
    manager.delete_model(id);
}

void stt_config::reload()
{
    OrgMkiolSttInterface stt{DBUS_SERVICE_NAME, DBUS_SERVICE_PATH, QDBusConnection::sessionBus()};
    if (!stt.isValid()) {
        qWarning() << "cannot reload service because is's not running";
        return;
    }

    auto reply = stt.Reload();
    reply.waitForFinished();

    if (reply.argumentAt<0>() != SUCCESS) qWarning() << "cannot reload service, error was returned";
}

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

    auto langs = manager.langs();
    std::transform(langs.cbegin(), langs.cend(), std::back_inserter(list), [](const auto& model) {
        return QVariantList{
            model.id,
            model.lang_id,
            QString{"%1 / %2"}.arg(model.name, model.lang_id),
            model.available,
            model.downloading,
            model.download_progress};
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
    //TO-DO: stop service
    manager.delete_model(id);
}

QString stt_config::default_model() const
{
    return test_default_model(settings::instance()->default_model());
}

QString stt_config::test_default_model(const QString &lang) const
{
    if (available_models_map.empty()) return {};

    const auto it = available_models_map.find(lang);

    if (it == available_models_map.cend()) {
        const auto it = std::find_if(available_models_map.cbegin(), available_models_map.cend(),
                                     [&lang](const auto& p){ return p.second.lang_id == lang; });
        if (it != available_models_map.cend()) return it->first;
    } else {
        return it->first;
    }

    return available_models_map.begin()->first;
}

void stt_config::set_default_model(const QString &model_id) const
{
    if (test_default_model(model_id) == model_id) {
        settings::instance()->set_default_model(model_id);
    } else {
        qWarning() << "invalid default model";
    }
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

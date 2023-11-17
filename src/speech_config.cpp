/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "speech_config.h"

#include <QDBusConnection>
#include <QDebug>
#include <algorithm>

#include "dbus_speech_inf.h"
#include "models_manager.h"
#include "settings.h"

speech_config::speech_config(QObject *parent) : QObject{parent} {
    connect(settings::instance(), &settings::default_stt_model_changed, this,
            [this] { emit default_model_changed(); });
    connect(models_manager::instance(), &models_manager::models_changed, this,
            &speech_config::handle_models_changed, Qt::QueuedConnection);
    connect(models_manager::instance(), &models_manager::busy_changed, this,
            [this] { emit busy_changed(); });
    connect(models_manager::instance(), &models_manager::download_progress,
            this, [this](const QString &id, double progress) {
                qDebug() << "download_progress:" << id << progress;
                emit model_download_progress_changed(id, progress);
            });
    connect(
        models_manager::instance(), &models_manager::download_finished, this,
        [this]([[maybe_unused]] const QString &id, bool download_not_needed) {
            if (!download_not_needed) emit model_download_finished();
        });
    connect(models_manager::instance(), &models_manager::download_error, this,
            [this]([[maybe_unused]] const QString &id) {
                emit model_download_error();
            });

    m_langs_model.updateModel();
    m_models_model.updateModel();
}

void speech_config::handle_models_changed() {
    emit models_changed();
}

ModelsListModel *speech_config::models_model() { return &m_models_model; }

LangsListModel *speech_config::langs_model() { return &m_langs_model; }

QVariantList speech_config::available_models() const {
    const auto available_models_list =
        models_manager::instance()->available_models();

    QVariantList list;
    std::transform(available_models_list.cbegin(), available_models_list.cend(),
                   std::back_inserter(list), [](const auto &model) {
                       return QStringList{model.id,
                                          QStringLiteral("%1 / %2").arg(
                                              model.name, model.lang_id)};
                   });

    return list;
}

void speech_config::download_model(const QString &id) {
    models_manager::instance()->download_model(id);
}

void speech_config::cancel_model_download(const QString &id) {
    models_manager::instance()->cancel_model_download(id);
}

void speech_config::delete_model(const QString &id) {
    models_manager::instance()->delete_model(id);
}

double speech_config::model_download_progress(const QString &id) const {
    return models_manager::instance()->model_download_progress(id);
}

void speech_config::set_default_model_for_lang(const QString &model_id) {
    models_manager::instance()->set_default_model_for_lang(model_id);
}

bool speech_config::busy() const { return models_manager::instance()->busy(); }

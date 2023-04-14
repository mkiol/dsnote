/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "models_manager.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrlQuery>
#include <QVariantList>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "checksum_tools.hpp"
#include "comp_tools.hpp"
#include "config.h"
#include "settings.h"

#ifdef ARCH_ARM_32
#include "cpu_tools.hpp"
#endif

static void remove_file_or_dir(const QString& path) {
    if (path.isEmpty()) return;

    if (QFileInfo{path}.isDir())
        QDir{path}.removeRecursively();
    else
        QFile::remove(path);
}

models_manager::models_manager(QObject* parent) : QObject{parent} {
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    m_nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

    connect(settings::instance(), &settings::models_dir_changed, this,
            static_cast<bool (models_manager::*)()>(
                &models_manager::parse_models_file_might_reset));

    parse_models_file_might_reset();
}

models_manager::~models_manager() {
    if (m_thread.joinable()) m_thread.join();
}

bool models_manager::ok() const { return !m_models.empty(); }

void models_manager::set_default_model_for_lang(const QString& model_id) {
    if (m_models.count(model_id) == 0) return;

    auto& model = m_models.at(model_id);

    qDebug() << "set_default_model_for_lang:" << model_id << model.lang_id;

    if (model.default_for_lang) {
        qWarning() << "model is already default for lang";
        return;
    }

    model.default_for_lang = true;

    if (auto old_default_model_id =
            settings::instance()->default_model_for_lang(model.lang_id);
        m_models.count(old_default_model_id) > 0) {
        m_models.at(old_default_model_id).default_for_lang = false;
    }

    settings::instance()->set_default_model_for_lang(model.lang_id, model_id);

    emit models_changed();
}

std::vector<models_manager::lang_t> models_manager::langs() const {
    std::vector<lang_t> list;

    std::transform(m_langs.cbegin(), m_langs.cend(), std::back_inserter(list),
                   [this](const auto& pair) {
                       return lang_t{
                           pair.first,
                           pair.second.first,
                           pair.second.second,
                           lang_available(pair.first),
                           lang_downloading(pair.first),
                       };
                   });

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
    });

    return list;
}

std::vector<models_manager::model_t> models_manager::models(
    const QString& lang_id) const {
    std::vector<model_t> list;

    QDir dir{settings::instance()->models_dir()};

    std::for_each(
        m_models.cbegin(), m_models.cend(),
        [&dir, &list, &lang_id](const auto& pair) {
            if (lang_id.isEmpty() || lang_id == pair.second.lang_id) {
                list.push_back(
                    {pair.first, pair.second.engine, pair.second.lang_id,
                     pair.second.name,
                     pair.second.scorer_file_name.isEmpty()
                         ? ""
                         : dir.filePath(pair.second.scorer_file_name),
                     pair.second.scorer_file_name.isEmpty()
                         ? ""
                         : dir.filePath(pair.second.scorer_file_name),
                     pair.second.score, pair.second.default_for_lang,
                     pair.second.available, pair.second.downloading,
                     pair.second.download_progress});
            }
        });

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        if (a.score == b.score)
            return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
        return a.score > b.score;
    });

    return list;
}

std::vector<models_manager::model_t> models_manager::available_models() const {
    std::vector<model_t> list;

    QDir dir{settings::instance()->models_dir()};

    for (const auto& [id, model] : m_models) {
        auto model_file = dir.filePath(model.file_name);
        if (model.available && QFile::exists(model_file)) {
            list.push_back(
                {id, model.engine, model.lang_id, model.name, model_file,
                 model.scorer_file_name.isEmpty()
                     ? QString{}
                     : dir.filePath(model.scorer_file_name),
                 model.score, model.default_for_lang, model.available,
                 model.downloading, model.download_progress});
        }
    }

    return list;
}

bool models_manager::model_exists(const QString& id) const {
    auto it = m_models.find(id);

    return it != std::cend(m_models) &&
           QFile::exists(model_path(it->second.file_name));
}

bool models_manager::lang_available(const QString& id) const {
    return std::any_of(
        m_models.cbegin(), m_models.cend(), [&id](const auto& model) {
            return model.second.lang_id == id && model.second.available;
        });
}

bool models_manager::lang_downloading(const QString& id) const {
    return std::any_of(
        m_models.cbegin(), m_models.cend(), [&id](const auto& model) {
            return model.second.lang_id == id && model.second.downloading;
        });
}

void models_manager::download_model(const QString& id) {
    download(id, download_type::all);
}

void models_manager::cancel_model_download(const QString& id) {
    models_to_cancel.insert(id);
}

bool models_manager::model_scorer_same_url(const priv_model_t& model) {
    if (model.comp == comp_type::tarxz && model.comp == model.scorer_comp &&
        model.urls.size() == 1 && model.scorer_urls.size() == 1) {
        QUrl model_url{model.urls.front()};
        QUrl scorer_url{model.urls.front()};

        if (model_url.hasQuery() &&
            QUrlQuery{model_url}.hasQueryItem(QStringLiteral("file")) &&
            scorer_url.hasQuery() &&
            QUrlQuery{scorer_url}.hasQueryItem(QStringLiteral("file"))) {
            model_url.setQuery(QUrlQuery{});
            scorer_url.setQuery(QUrlQuery{});
            return model_url == scorer_url;
        }
    }

    return false;
}

void models_manager::download(const QString& id, download_type type, int part) {
    if (type != download_type::all && type != download_type::scorer) {
        qWarning() << "incorrect dl type requested:" << download_type_str(type);
        return;
    }

    auto it = m_models.find(id);
    if (it == std::end(m_models)) {
        qWarning() << "no model with id:" << id;
        return;
    }

    auto& model = it->second;

    if (part < 0) {
        bool m_cs =
            checksum_ok(model.checksum, model.checksum_quick, model.file_name);
        bool s_cs =
            model.scorer_checksum.isEmpty() ||
            model.scorer_file_name.isEmpty() || model.scorer_urls.empty() ||
            checksum_ok(model.scorer_checksum, model.scorer_checksum_quick,
                        model.scorer_file_name);
        if ((type == download_type::all && m_cs && s_cs) ||
            (type == download_type::scorer && s_cs)) {
            qWarning() << "both model and scorer exist, download not needed";

            auto models = settings::instance()->enabled_models();
            models.push_back(id);
            settings::instance()->set_enabled_models(models);

            model.available = true;
            emit download_finished(id);
            model.downloading = false;
            model.download_progress = 0.0;
            model.downloaded_part_data = 0;
            emit models_changed();
            return;
        }
        if (type == download_type::all && m_cs) {
            qDebug() << "model already exists, downloading only scorer";
            model.downloaded_part_data = model.size;
            type = download_type::scorer;
        }
    }

    if (type == download_type::all && part < 0 &&
        model_scorer_same_url(model)) {
        type = download_type::model_scorer;
    }

    const auto& urls =
        type == download_type::scorer ? model.scorer_urls : model.urls;

    if (part < 0) {
        if (type != download_type::scorer) model.downloaded_part_data = 0;
        if (urls.size() > 1) part = 0;
    }

    auto url = urls.at(part < 0 ? 0 : part);

    QString path, path_2, checksum, checksum_2, path_in_archive,
        path_in_archive_2;
    comp_type comp;
    auto next_type = download_type::none;
    const auto next_part =
        part < 0 || part >= static_cast<int>(urls.size()) - 1 ? -1 : part + 1;
    const bool scorer_exists = !model.scorer_file_name.isEmpty() &&
                               !model.scorer_checksum.isEmpty() &&
                               !url.isEmpty();
    const qint64 size = type != download_type::model_scorer && scorer_exists
                            ? model.size + model.scorer_size
                            : model.size;

    QNetworkRequest request{url};
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    if (type == download_type::all || type == download_type::model_scorer) {
        path = model_path(model.file_name);
        checksum = model.checksum;
        comp = model.comp;

        if (type == download_type::all) {
            type = download_type::model;
            if (next_part > 0) {
                next_type = download_type::all;
            } else if (scorer_exists) {
                next_type = download_type::scorer;
            }
        } else {  // model_scorer
            path_2 = model_path(model.scorer_file_name);
            checksum_2 = model.scorer_checksum;
            next_type = download_type::none;
        }
    } else {  // scorer
        path = model_path(model.scorer_file_name);
        checksum = model.scorer_checksum;
        comp = model.scorer_comp;
        next_type = next_part > 0 ? download_type::scorer : download_type::none;
    }

    if ((comp == comp_type::tarxz || comp == comp_type::zip) &&
        url.hasQuery()) {
        if (QUrlQuery query{url}; query.hasQueryItem(QStringLiteral("file"))) {
            path_in_archive = query.queryItemValue(QStringLiteral("file"));
            if (type == download_type::model_scorer) {
                path_in_archive_2 =
                    QUrlQuery{model.scorer_urls.front()}.queryItemValue(
                        QStringLiteral("file"));
            }
            query.removeQueryItem(QStringLiteral("file"));
            url.setQuery(query);
        }
    }

    auto* out_file = new std::ofstream{
        download_filename(path, comp, part).toStdString(), std::ofstream::out};

    QNetworkReply* reply = m_nam.get(request);
    qDebug() << "downloading:" << url << ", type:" << download_type_str(type)
             << ", comp:" << comp_type_str(comp);
    qDebug() << "out path:" << path << path_2;

    reply->setProperty("out_file",
                       QVariant::fromValue(static_cast<void*>(out_file)));
    reply->setProperty("out_path", path);
    reply->setProperty("out_path_2", path_2);
    reply->setProperty("model_id", id);
    reply->setProperty("download_type", static_cast<int>(type));
    reply->setProperty("download_next_type", static_cast<int>(next_type));
    reply->setProperty("checksum", checksum);
    reply->setProperty("checksum_2", checksum_2);
    reply->setProperty("comp", static_cast<int>(comp));
    reply->setProperty("size", size);
    reply->setProperty("part", part);
    reply->setProperty("next_part", next_part);
    reply->setProperty("path_in_archive", path_in_archive);
    reply->setProperty("path_in_archive_2", path_in_archive_2);

    connect(reply, &QNetworkReply::downloadProgress, this,
            &models_manager::handle_download_progress);
    connect(reply, &QNetworkReply::finished, this,
            &models_manager::handle_download_finished);
    connect(reply, &QNetworkReply::readyRead, this,
            &models_manager::handle_download_ready_read);
    connect(reply, &QNetworkReply::sslErrors, this,
            &models_manager::handle_ssl_errors);

    model.downloading = true;
    if (part < 0) emit models_changed();
}

void models_manager::handle_ssl_errors(const QList<QSslError>& errors) {
    qWarning() << "ssl error:" << errors;
}

void models_manager::handle_download_ready_read() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());

    if (check_model_download_cancel(reply)) return;

    if (reply->bytesAvailable() > 0) {
        auto data = reply->readAll();
        auto out_file = static_cast<std::ofstream*>(
            reply->property("out_file").value<void*>());
        out_file->write(data.data(), data.size());
    }
}

bool models_manager::check_checksum(const QString& path,
                                    const QString& checksum) {
    auto real_checksum = checksum_tools::make_checksum(path);
    auto real_quick_checksum = checksum_tools::make_quick_checksum(path);

    bool ok = real_checksum == checksum;

    if (!ok) {
        qWarning() << "checksum 1 is invalid:" << real_checksum << "(expected"
                   << checksum << ")";
        qDebug() << "quick checksum 1:" << real_quick_checksum;
    }

    return ok;
}

bool models_manager::extract_from_archive(
    const QString& archive_path, comp_type comp, const QString& out_path,
    const QString& checksum, const QString& path_in_archive,
    const QString& out_path_2, const QString& checksum_2,
    const QString& path_in_archive_2) {
    bool ok_2 = true;

    auto archive_type = [comp] {
        switch (comp) {
            case comp_type::tar:
                return comp_tools::archive_type::tar;
            case comp_type::zip:
                return comp_tools::archive_type::zip;
            default:
                throw std::runtime_error("unsupported comp type");
        }
    }();

    if (!path_in_archive_2.isEmpty() && !out_path_2.isEmpty()) {
        comp_tools::archive_decode(
            archive_path, archive_type,
            {{},
             {{path_in_archive, out_path}, {path_in_archive_2, out_path_2}}});

        ok_2 = check_checksum(out_path_2, checksum_2);
    } else if (!path_in_archive.isEmpty() && !out_path.isEmpty()) {
        comp_tools::archive_decode(archive_path, archive_type,
                                   {{}, {{path_in_archive, out_path}}});
    } else {
        comp_tools::archive_decode(archive_path, archive_type, {out_path, {}});
    }

    return ok_2 ? check_checksum(out_path, checksum) : false;
}

bool models_manager::handle_download(const QString& path,
                                     const QString& checksum,
                                     const QString& path_in_archive,
                                     const QString& path_2,
                                     const QString& checksum_2,
                                     const QString& path_in_archive_2,
                                     comp_type comp, int parts) {
    QEventLoop loop;

    bool ok = false;

    if (m_thread.joinable()) m_thread.join();

    m_thread = std::thread{[&] {
        if (parts > 1) {
            qDebug() << "joining parts:" << parts;
            join_part_files(download_filename(path, comp), parts);
            for (int i = 0; i < parts; ++i)
                QFile::remove(download_filename(path, comp, i));
        }

        auto comp_file = download_filename(path, comp);
        qDebug() << "total downloaded size:" << QFileInfo{comp_file}.size();

        if (comp == comp_type::none) {
            ok = check_checksum(path, checksum);
        } else {
            if (comp == comp_type::gz) {
                comp_tools::gz_decode(comp_file, path);
                QFile::remove(comp_file);
                ok = check_checksum(path, checksum);
            } else if (comp == comp_type::xz) {
                comp_tools::xz_decode(comp_file, path);
                QFile::remove(comp_file);
                ok = check_checksum(path, checksum);
            } else if (comp == comp_type::tarxz) {
                auto tar_file = download_filename(path, comp_type::tar);
                comp_tools::xz_decode(comp_file, tar_file);
                QFile::remove(comp_file);
                ok = extract_from_archive(tar_file, comp_type::tar, path,
                                          checksum, path_in_archive, path_2,
                                          checksum_2, path_in_archive_2);
                QFile::remove(tar_file);
            } else if (comp == comp_type::zip) {
                ok = extract_from_archive(comp_file, comp_type::zip, path,
                                          checksum, path_in_archive, path_2,
                                          checksum_2, path_in_archive_2);
                QFile::remove(comp_file);
            } else {
                QFile::remove(comp_file);
            }
        }

        if (!ok) {
            remove_file_or_dir(path);
            remove_file_or_dir(path_2);
        }

        loop.quit();
    }};

    loop.exec();
    if (m_thread.joinable()) m_thread.join();

    return ok;
}

bool models_manager::check_model_download_cancel(QNetworkReply* reply) {
    auto it = models_to_cancel.find(reply->property("model_id").toString());
    if (it == models_to_cancel.end()) return false;
    models_to_cancel.erase(it);

    reply->abort();
    return true;
}

void models_manager::handle_download_finished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());

    delete static_cast<std::ofstream*>(
        reply->property("out_file").value<void*>());

    auto id = reply->property("model_id").toString();

    auto& model = m_models.at(id);
    auto type =
        static_cast<download_type>(reply->property("download_type").toInt());
    auto path = reply->property("out_path").toString();
    auto comp = static_cast<comp_type>(reply->property("comp").toInt());
    auto part = reply->property("part").toInt();

    if (auto cancel = check_model_download_cancel(reply);
        cancel || reply->error() != QNetworkReply::NoError) {
        qWarning() << "download error:" << reply->error();
        QFile::remove(download_filename(path, comp, part));
        if (part > 0) {
            for (int i = 0; i < part; ++i) {
                QFile::remove(download_filename(path, comp, i));
            }
        }

        if (!cancel &&
            reply->error() != QNetworkReply::OperationCanceledError) {
            emit download_error(id);
        }
    } else {
        auto next_part = reply->property("next_part").toInt();

        if (next_part < 0) {
            auto parts = type == download_type::scorer
                             ? model.scorer_urls.size()
                             : model.urls.size();
            auto downloaded_part_data =
                QFileInfo{download_filename(path, comp, part)}.size();
            auto path_2 = reply->property("out_path_2").toString();
            auto checksum = reply->property("checksum").toString();
            auto checksum_2 = reply->property("checksum_2").toString();
            auto path_in_archive =
                reply->property("path_in_archive").toString();
            auto path_in_archive_2 =
                reply->property("path_in_archive_2").toString();

            if (handle_download(path, checksum, path_in_archive, path_2,
                                checksum_2, path_in_archive_2, comp, parts)) {
                auto next_type = static_cast<download_type>(
                    reply->property("download_next_type").toInt());
                qDebug() << "successfully downloaded:" << id
                         << ", type:" << download_type_str(type)
                         << ", next_type:" << download_type_str(next_type);

                if (next_type == download_type::scorer) {
                    model.downloaded_part_data += downloaded_part_data;
                    download(id, download_type::scorer);
                    reply->deleteLater();
                    return;
                }

                auto models = settings::instance()->enabled_models();
                models.push_back(id);
                settings::instance()->set_enabled_models(models);

                model.available = true;
                emit download_finished(id);
            } else {
                remove_file_or_dir(path);
                emit download_error(id);
            }
        } else {
            qDebug() << "successfully downloaded:" << id
                     << ", type:" << download_type_str(type)
                     << ", part:" << part;
            model.downloaded_part_data +=
                QFileInfo{download_filename(path, comp, part)}.size();
            download(id,
                     static_cast<download_type>(
                         reply->property("download_next_type").toInt()),
                     next_part);
            reply->deleteLater();
            return;
        }
    }

    reply->deleteLater();

    model.downloading = false;
    model.download_progress = 0.0;
    model.downloaded_part_data = 0;
    emit models_changed();
}

void models_manager::handle_download_progress(qint64 received,
                                              qint64 real_total) {
    auto* reply = qobject_cast<const QNetworkReply*>(sender());

    if (reply->isFinished()) return;

    auto id = reply->property("model_id").toString();

    auto total = reply->property("size").toLongLong();
    if (total <= 0) total = real_total;

    if (total > 0) {
        auto& model = m_models.at(id);
        auto new_download_progress =
            static_cast<double>(received + model.downloaded_part_data) / total;
        if (new_download_progress - model.download_progress >= 0.01) {
            model.download_progress = new_download_progress;
            emit download_progress(id, new_download_progress);
        }
    }
}

void models_manager::delete_model(const QString& id) {
    if (auto it = m_models.find(id); it != std::cend(m_models)) {
        auto& model = it->second;

        if (!std::any_of(
                m_models.cbegin(), m_models.cend(),
                [id = it->first, file_name = model.file_name](const auto& p) {
                    return p.second.available && p.first != id &&
                           p.second.file_name == file_name;
                })) {
            remove_file_or_dir(model_path(model.file_name));
        } else {
            qDebug() << "not removing model file because other model uses it:"
                     << model.file_name;
        }
        if (!std::any_of(m_models.cbegin(), m_models.cend(),
                         [id = it->first,
                          file_name = model.scorer_file_name](const auto& p) {
                             return p.second.available && p.first != id &&
                                    p.second.scorer_file_name == file_name;
                         })) {
            remove_file_or_dir(model_path(model.scorer_file_name));
        } else {
            qDebug() << "not removing scorer file because other model uses it:"
                     << model.scorer_file_name;
        }

        auto models = settings::instance()->enabled_models();
        models.removeAll(id);
        settings::instance()->set_enabled_models(models);

        model.available = false;
        model.download_progress = 0.0;
        model.downloaded_part_data = 0;
        emit models_changed();
    } else {
        qWarning() << "no model with id:" << id;
    }
}

void models_manager::backup_config(const QString& lang_models_file) {
    QString backup_file;
    QFileInfo lang_models_file_i{lang_models_file};
    auto backup_file_stem =
        lang_models_file_i.dir().filePath(lang_models_file_i.baseName());

    int i = 0;
    do {
        backup_file = backup_file_stem + QStringLiteral(".old") +
                      (i == 0 ? QLatin1String{} : QString::number(i)) +
                      QStringLiteral(".json");
        ++i;
    } while (QFile::exists(backup_file) && i < 1000);

    qDebug() << "making lang models file backup to:" << backup_file;

    QFile::remove(backup_file);
    QFile::copy(lang_models_file, backup_file);
}

void models_manager::init_config() {
    QFile default_models_file{":/config/" + models_file};
    if (!default_models_file.exists()) {
        qWarning() << "default models file does not exist";
        return;
    }

    if (!default_models_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "failed to open default models file";
        return;
    }

    QString data_dir{
        QStandardPaths::writableLocation(QStandardPaths::DataLocation)};
    QDir dir{data_dir};
    if (!dir.exists())
        if (!dir.mkpath(data_dir)) qWarning() << "failed to create data dir";

    QFile models{dir.filePath(models_file)};

    if (models.exists()) backup_config(QFileInfo{models}.absoluteFilePath());

    if (!models.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "failed to open models file";
        return;
    }

    auto data = default_models_file.readAll();
    models.write(data.replace("@VERSION@", APP_CONF_VERSION));
}

models_manager::comp_type models_manager::str2comp(const QString& str) {
    if (!str.compare(QStringLiteral("gz"), Qt::CaseInsensitive))
        return comp_type::gz;
    if (!str.compare(QStringLiteral("xz"), Qt::CaseInsensitive))
        return comp_type::xz;
    if (!str.compare(QStringLiteral("tarxz"), Qt::CaseInsensitive))
        return comp_type::tarxz;
    if (!str.compare(QStringLiteral("zip"), Qt::CaseInsensitive))
        return comp_type::zip;
    return comp_type::none;
}

QString models_manager::download_filename(const QString& filename,
                                          comp_type comp, int part) {
    QString ret_name = filename;

    switch (comp) {
        case comp_type::gz:
            ret_name += QStringLiteral(".gz");
            break;
        case comp_type::xz:
            ret_name += QStringLiteral(".xz");
            break;
        case comp_type::tar:
            ret_name += QStringLiteral(".tar");
            break;
        case comp_type::tarxz:
            ret_name += QStringLiteral(".tar.xz");
            break;
        case comp_type::zip:
            ret_name += QStringLiteral(".zip");
            break;
        case comp_type::none:
            break;
    }

    if (part > -1) {
        ret_name +=
            QStringLiteral(".part-%1").arg(part, 2, 10, QLatin1Char{'0'});
    }

    return ret_name;
}

auto models_manager::extract_langs(const QJsonArray& langs_jarray) {
    langs_t langs;

    for (const auto& ele : langs_jarray) {
        auto obj = ele.toObject();

        auto lang_id = obj.value(QLatin1String{"id"}).toString();
        if (lang_id.isEmpty()) {
            qWarning() << "empty model id";
            continue;
        }

        if (langs.find(lang_id) != langs.end()) {
            qWarning() << "duplicate lang id:" << lang_id;
            continue;
        }

        langs.emplace(
            lang_id, std::pair{obj.value(QLatin1String{"name"}).toString(),
                               obj.value(QLatin1String{"name_en"}).toString()});
    }

    return langs;
}

void models_manager::remove_empty_langs(langs_t& langs,
                                        const models_t& models) {
    std::set<QString> existing_langs;
    for (const auto& [_, model] : models) existing_langs.insert(model.lang_id);

    auto it = langs.begin();
    while (it != langs.end()) {
        if (existing_langs.count(it->first) == 0)
            it = langs.erase(it);
        else
            ++it;
    }
}

bool models_manager::checksum_ok(const QString& checksum,
                                 const QString& checksum_quick,
                                 const QString& file_name) {
    QString expected_checksum;
    QString real_checksum;

    if (checksum_quick.isEmpty()) {
        expected_checksum = checksum;
        real_checksum = checksum_tools::make_checksum(model_path(file_name));
    } else {
        expected_checksum = checksum_quick;
        real_checksum =
            checksum_tools::make_quick_checksum(model_path(file_name));
    }

    auto ok = expected_checksum == real_checksum;

    if (ok)
        qDebug() << "checksum ok:" << real_checksum << file_name;
    else
        qWarning() << "checksum mismatch:" << real_checksum
                   << "(expected:" << expected_checksum << ")" << file_name;

    return ok;
}

models_manager::model_engine models_manager::engine_from_name(
    const QString& name) {
    if (name == QStringLiteral("stt_ds")) return model_engine::stt_ds;
    if (name == QStringLiteral("stt_vosk")) return model_engine::stt_vosk;
    if (name == QStringLiteral("stt_whisper")) return model_engine::stt_whisper;
    return model_engine::stt_ds;  // default
}

auto models_manager::extract_models(const QJsonArray& models_jarray) {
    models_t models;

    auto enabled_models = settings::instance()->enabled_models();

    QDir dir{settings::instance()->models_dir()};

#ifdef ARCH_ARM_32
    auto has_neon_fp = cpu_tools::neon_supported();
#endif

    for (const auto& ele : models_jarray) {
        auto obj = ele.toObject();
        auto model_id = obj.value(QLatin1String{"model_id"}).toString();

        if (model_id.isEmpty()) {
            qWarning() << "empty model id in lang models file";
            continue;
        }

        if (models.count(model_id) > 0) {
            qWarning() << "duplicate model id in lang models file:" << model_id;
            continue;
        }

        auto lang_id = obj.value(QLatin1String{"lang_id"}).toString();
        if (lang_id.isEmpty()) {
            qWarning() << "empty lang id in lang models file";
            continue;
        }

        auto checksum = obj.value(QLatin1String{"checksum"}).toString();
        if (checksum.isEmpty()) {
            qWarning() << "checksum cannot be empty:" << model_id;
            continue;
        }

        auto engine =
            engine_from_name(obj.value(QLatin1String{"engine"}).toString());

#ifdef ARCH_ARM_32
        if (!has_neon_fp && engine == model_engine::whisper) {
            qDebug() << "ignoring whisper model because cpu does not support "
                        "neon fd:"
                     << model_id;
            continue;
        }
#endif
#ifdef USE_SFOS
        if (engine == model_engine::vosk && model_id.contains("large")) {
            qDebug() << "ignoring vosk large model:" << model_id;
            continue;
        }
        if (engine == model_engine::whisper &&
            (model_id.contains("small") || model_id.contains("medium") ||
             model_id.contains("large"))) {
            qDebug() << "ignoring whisper small, medium or large model:"
                     << model_id;
            continue;
        }
#endif

        auto file_name = obj.value(QLatin1String{"file_name"}).toString();
        if (file_name.isEmpty())
            file_name = file_name_from_id(model_id, engine);

        auto scorer_file_name =
            obj.value(QLatin1String{"scorer_file_name"}).toString();
        if (scorer_file_name.isEmpty())
            scorer_file_name = scorer_file_name_from_id(model_id);

        bool is_default_model_for_lang =
            settings::instance()->default_model_for_lang(lang_id) == model_id;

        const auto urls = obj.value(QLatin1String{"urls"}).toArray();
        if (urls.isEmpty()) {
            qWarning() << "urls should be non empty array";
            continue;
        }

        priv_model_t model{
            /*engine=*/engine,
            /*lang_id=*/std::move(lang_id),
            /*name=*/obj.value(QLatin1String{"name"}).toString(),
            /*file_name=*/std::move(file_name),
            /*checksum=*/std::move(checksum),
            /*checksum_quick=*/
            obj.value(QLatin1String{"checksum_quick"}).toString(),
            /*comp=*/str2comp(obj.value(QLatin1String{"comp"}).toString()),
            /*urls=*/{},
            /*size=*/obj.value(QLatin1String{"size"}).toString().toLongLong(),
            /*scorer_file_name=*/std::move(scorer_file_name),
            /*scorer_checksum=*/
            obj.value(QLatin1String{"scorer_checksum"}).toString(),
            /*scorer_checksum_quick=*/
            obj.value(QLatin1String{"scorer_checksum_quick"}).toString(),
            /*scorer_comp=*/
            str2comp(obj.value(QLatin1String{"scorer_comp"}).toString()),
            /*scorer_urls=*/{},
            /*scorer_size=*/
            obj.value(QLatin1String{"scorer_size"}).toString().toLongLong(),
            /*score=*/obj.value(QLatin1String{"score"}).toInt(2),
            /*default_for_lang=*/is_default_model_for_lang,
            /*available=*/false,
            /*downloading=*/false};

        for (const auto& url : urls) model.urls.emplace_back(url.toString());

        auto scorer_urls = obj.value(QLatin1String{"scorer_urls"}).toArray();
        for (auto url : scorer_urls)
            model.scorer_urls.emplace_back(url.toString());

        if (dir.exists(model.file_name)) {
            if (checksum_ok(model.checksum, model.checksum_quick,
                            model.file_name)) {
                if (model.scorer_urls.empty()) {
                    model.available = true;
                } else if (dir.exists(model.scorer_file_name)) {
                    if (checksum_ok(model.scorer_checksum,
                                    model.scorer_checksum_quick,
                                    model.scorer_file_name)) {
                        model.available = true;
                    }
                }
            }
        }

        if (model.available) qDebug() << "found model:" << model_id;

        auto model_is_enabled = enabled_models.contains(model_id);
        if (model.available) {
            model.available = model_is_enabled;
        } else {
            if (model_is_enabled) enabled_models.removeAll(model_id);
            if (is_default_model_for_lang)
                settings::instance()->set_default_model_for_lang(model.lang_id,
                                                                 {});
        }

        models.emplace(model_id, std::move(model));
    }

    settings::instance()->set_enabled_models(enabled_models);

    return models;
}

void models_manager::reload() {
    settings::instance()->sync();  // needed to update changes app -> service

    if (!parse_models_file_might_reset()) m_pending_reload = true;
}

bool models_manager::parse_models_file_might_reset() {
    bool expected = false;
    if (!m_busy_value.compare_exchange_strong(expected, true)) return false;

    emit busy_changed();

    if (m_thread.joinable()) m_thread.join();

    m_thread = std::thread{[this] {
        do {
            m_pending_reload = false;
            parse_models_file(false, &m_langs, &m_models);
        } while (m_pending_reload);

        emit models_changed();
        m_busy_value.store(false);
        emit busy_changed();
    }};

    return true;
}

void models_manager::parse_models_file(bool reset, langs_t* langs,
                                       models_t* models) {
    const auto models_file_path =
        QDir{QStandardPaths::writableLocation(QStandardPaths::DataLocation)}
            .filePath(models_file);
    if (!QFile::exists(models_file_path)) init_config();

    if (std::ifstream input{models_file_path.toStdString(),
                            std::ifstream::in | std::ifstream::binary}) {
        std::vector<char> buff{std::istreambuf_iterator<char>{input}, {}};

        QJsonParseError err;
        auto json = QJsonDocument::fromJson(
            QByteArray::fromRawData(&buff[0], buff.size()), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "error parsing json:" << err.errorString();
            input.close();
            if (!reset) {
                init_config();
                parse_models_file(true, langs, models);
            }
        } else {
            auto version = json.object()
                               .value(QStringLiteral("version"))
                               .toString()
                               .toInt();

            auto required_version = std::stoi(APP_CONF_VERSION);

            qDebug() << "config version:" << version << APP_CONF_VERSION;

            if (version != required_version) {
                qWarning("version mismatch, has %d but requires %d", version,
                         required_version);
                input.close();
                if (!reset) {
                    init_config();
                    parse_models_file(true, langs, models);
                }
            } else {
                *langs = extract_langs(
                    json.object().value(QStringLiteral("langs")).toArray());
                *models = extract_models(
                    json.object().value(QStringLiteral("models")).toArray());

                remove_empty_langs(*langs, *models);
            }
        }
    } else {
        qWarning() << "cannot open lang models file";
    }
}

QString models_manager::file_name_from_id(const QString& id,
                                          model_engine engine) {
    switch (engine) {
        case model_engine::stt_ds:
            return id + ".tflite";
        case model_engine::stt_whisper:
            return id + ".ggml";
        case model_engine::stt_vosk:
            return id;
    }

    throw std::runtime_error{"unknown model engine"};
}

QString models_manager::scorer_file_name_from_id(const QString& id) {
    return id + ".scorer";
}

QString models_manager::model_path(const QString& file_name) {
    return QDir{settings::instance()->models_dir()}.filePath(file_name);
}

double models_manager::model_download_progress(const QString& id) const {
    try {
        return m_models.at(id).download_progress;
    } catch (const std::out_of_range&) {
    }
    return 0.0;
}

bool models_manager::join_part_files(const QString& file_out, int parts) {
    qDebug() << "joining files:" << file_out;

    if (std::ofstream output{file_out.toStdString(),
                             std::ios::out | std::ifstream::binary}) {
        for (int i = 0; i < parts; ++i) {
            auto file_in = download_filename(file_out, comp_type::none, i);
            if (std::ifstream input{file_in.toStdString(),
                                    std::ios::in | std::ifstream::binary}) {
                char buff[std::numeric_limits<unsigned short>::max()];
                while (input) {
                    input.read(buff, sizeof buff);
                    output.write(buff, static_cast<int>(input.gcount()));
                }
            } else {
                qWarning() << "error opening in-file:" << file_in;
                return false;
            }
        }
    } else {
        qWarning() << "error opening out-file:" << file_out;
        return false;
    }

    return true;
}

QLatin1String models_manager::download_type_str(download_type type) {
    switch (type) {
        case download_type::all:
            return QLatin1String("all");
        case download_type::model:
            return QLatin1String("model");
        case download_type::model_scorer:
            return QLatin1String("model_scorer");
        case download_type::scorer:
            return QLatin1String("scorer");
        case download_type::none:
            return QLatin1String("none");
    }
    return QLatin1String("unknown");
}

QLatin1String models_manager::comp_type_str(comp_type type) {
    switch (type) {
        case comp_type::gz:
            return QLatin1String("gz");
        case comp_type::tar:
            return QLatin1String("tar");
        case comp_type::tarxz:
            return QLatin1String("tarxz");
        case comp_type::xz:
            return QLatin1String("xz");
        case comp_type::zip:
            return QLatin1String("zip");
        case comp_type::none:
            return QLatin1String("none");
    }
    return QLatin1String("unknown");
}

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
#include <QDirIterator>
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

QDebug operator<<(QDebug d, models_manager::comp_type comp_type) {
    switch (comp_type) {
        case models_manager::comp_type::gz:
            d << "gz";
            break;
        case models_manager::comp_type::tar:
            d << "tar";
            break;
        case models_manager::comp_type::tarxz:
            d << "tarxz";
            break;
        case models_manager::comp_type::targz:
            d << "targz";
            break;
        case models_manager::comp_type::xz:
            d << "xz";
            break;
        case models_manager::comp_type::zip:
            d << "zip";
            break;
        case models_manager::comp_type::zipall:
            d << "zipall";
            break;
        case models_manager::comp_type::dir:
            d << "dir";
            break;
        case models_manager::comp_type::none:
            d << "none";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, models_manager::download_type download_type) {
    switch (download_type) {
        case models_manager::download_type::all:
            d << "all";
            break;
        case models_manager::download_type::model:
            d << "model";
            break;
        case models_manager::download_type::model_scorer:
            d << "model_scorer";
            break;
        case models_manager::download_type::scorer:
            d << "scorer";
            break;
        case models_manager::download_type::none:
            d << "none";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, models_manager::model_role role) {
    switch (role) {
        case models_manager::model_role::stt:
            d << "stt";
            break;
        case models_manager::model_role::ttt:
            d << "ttt";
            break;
        case models_manager::model_role::tts:
            d << "tts";
            break;
    }

    return d;
}

QDebug operator<<(QDebug d, models_manager::model_engine engine) {
    switch (engine) {
        case models_manager::model_engine::stt_ds:
            d << "stt-ds";
            break;
        case models_manager::model_engine::stt_vosk:
            d << "stt-vosk";
            break;
        case models_manager::model_engine::stt_whisper:
            d << "stt-whisper";
            break;
        case models_manager::model_engine::ttt_hftc:
            d << "ttt-hftc";
            break;
        case models_manager::model_engine::tts_coqui:
            d << "tts-coqui";
            break;
        case models_manager::model_engine::tts_piper:
            d << "tts-piper";
            break;
        case models_manager::model_engine::tts_espeak:
            d << "tts-espeak";
            break;
        case models_manager::model_engine::tts_rhvoice:
            d << "tts-rhvoice";
            break;
    }

    return d;
}

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

    if (model.default_for_lang) {
        qWarning() << "model is already default for lang";
        return;
    }

    model.default_for_lang = true;

    switch (role_of_engine(model.engine)) {
        case model_role::stt:
            if (auto old_default_model_id =
                    settings::instance()->default_stt_model_for_lang(
                        model.lang_id);
                m_models.count(old_default_model_id) > 0) {
                m_models.at(old_default_model_id).default_for_lang = false;
            }

            settings::instance()->set_default_stt_model_for_lang(model.lang_id,
                                                                 model_id);
            break;
        case model_role::tts:
            if (auto old_default_model_id =
                    settings::instance()->default_tts_model_for_lang(
                        model.lang_id);
                m_models.count(old_default_model_id) > 0) {
                m_models.at(old_default_model_id).default_for_lang = false;
            }

            settings::instance()->set_default_tts_model_for_lang(model.lang_id,
                                                                 model_id);
            break;
        case model_role::ttt:
            throw std::runtime_error("invalid model role");
    }

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
            if (!pair.second.hidden &&
                (lang_id.isEmpty() || lang_id == pair.second.lang_id)) {
                list.push_back(
                    {pair.first, pair.second.engine, pair.second.lang_id,
                     pair.second.name,
                     pair.second.scorer_file_name.isEmpty()
                         ? ""
                         : dir.filePath(pair.second.scorer_file_name),
                     pair.second.scorer_file_name.isEmpty()
                         ? ""
                         : dir.filePath(pair.second.scorer_file_name),
                     pair.second.speaker, pair.second.score,
                     pair.second.default_for_lang, pair.second.available,
                     pair.second.downloading, pair.second.download_progress});
            }
        });

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        auto ra = static_cast<int>(role_of_engine(a.engine));
        auto rb = static_cast<int>(role_of_engine(b.engine));

        if (ra != rb) return ra < rb;
        if (a.score != b.score) return a.score > b.score;
        return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
    });

    return list;
}

bool models_manager::has_model_of_role(model_role role) const {
    QDir dir{settings::instance()->models_dir()};

    return std::find_if(m_models.cbegin(), m_models.cend(), [&](const auto& p) {
               return !p.second.hidden && p.second.available &&
                      models_manager::role_of_engine(p.second.engine) == role &&
                      QFile::exists(
                          dir.filePath(dir.filePath(p.second.file_name)));
           }) != m_models.cend();
}

std::vector<models_manager::model_t> models_manager::available_models() const {
    std::vector<model_t> list;

    QDir dir{settings::instance()->models_dir()};

    for (const auto& [id, model] : m_models) {
        auto model_file = dir.filePath(model.file_name);
        if (!model.hidden && model.available && QFile::exists(model_file)) {
            list.push_back(
                {id, model.engine, model.lang_id, model.name, model_file,
                 model.scorer_file_name.isEmpty()
                     ? QString{}
                     : dir.filePath(model.scorer_file_name),
                 model.speaker, model.score, model.default_for_lang,
                 model.available, model.downloading, model.download_progress});
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
        qWarning() << "incorrect dl type requested:" << type;
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
            (model.engine == model_engine::tts_espeak &&
             (model.checksum.isEmpty() || model.file_name.isEmpty())) ||
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

    if ((comp == comp_type::tarxz || comp == comp_type::zip ||
         comp == comp_type::zipall) &&
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

    auto out_file_path = [&] {
        auto p = download_filename(path, comp, part, url);
        if (!p.isEmpty())
            QDir::root().mkpath(QFileInfo{p}.dir().absolutePath());
        return p;
    }();

    if (out_file_path.isEmpty()) {
        qWarning() << "out file path is empty";
        return;
    }

    auto* out_file =
        new std::ofstream{out_file_path.toStdString(), std::ofstream::out};

    QNetworkReply* reply = m_nam.get(request);
    qDebug() << "downloading: url=" << url << ", type=" << type
             << ", comp=" << comp << ", out path=" << path
             << ", out path2=" << path_2 << ", out file path=" << out_file_path;

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

    if (!model.downloading) {
        model.downloading = true;
        emit download_started(id);
    }

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
            case comp_type::zipall:
                return comp_tools::archive_type::zip;
            default:
                throw std::runtime_error("unsupported comp type");
        }
    }();

    if (!path_in_archive_2.isEmpty() && !out_path_2.isEmpty()) {
        comp_tools::archive_decode(
            archive_path, archive_type,
            {{},
             {{path_in_archive, out_path}, {path_in_archive_2, out_path_2}}},
            true);

        ok_2 = check_checksum(out_path_2, checksum_2);
    } else if (!path_in_archive.isEmpty() && !out_path.isEmpty()) {
        comp_tools::archive_decode(archive_path, archive_type,
                                   {{}, {{path_in_archive, out_path}}},
                                   comp != comp_type::zipall);
    } else {
        comp_tools::archive_decode(archive_path, archive_type, {out_path, {}},
                                   comp != comp_type::zipall);
    }

    return ok_2 ? check_checksum(out_path, checksum) : false;
}

qint64 models_manager::total_size(const QString& path) {
    QFileInfo fi{path};

    qint64 total = 0;

    if (fi.isDir()) {
        QDirIterator it{path, QDirIterator::NoIteratorFlags};
        while (it.hasNext()) total += QFileInfo{it.next()}.size();
    } else {
        total = fi.size();
    }

    return total;
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
        if (comp != comp_type::dir && parts > 1) {
            qDebug() << "joining parts:" << parts;
            join_part_files(download_filename(path, comp), parts);
            for (int i = 0; i < parts; ++i)
                QFile::remove(download_filename(path, comp, i));
        }

        auto comp_file = download_filename(path, comp);
        qDebug() << "total downloaded size:" << total_size(comp_file);

        if (comp == comp_type::none || comp == comp_type::dir) {
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
            } else if (comp == comp_type::targz) {
                auto tar_file = download_filename(path, comp_type::tar);
                comp_tools::gz_decode(comp_file, tar_file);
                QFile::remove(comp_file);
                ok = extract_from_archive(tar_file, comp_type::tar, path,
                                          checksum, path_in_archive, path_2,
                                          checksum_2, path_in_archive_2);
                QFile::remove(tar_file);
            } else if (comp == comp_type::zip || comp == comp_type::zipall) {
                ok = extract_from_archive(comp_file, comp, path, checksum,
                                          path_in_archive, path_2, checksum_2,
                                          path_in_archive_2);
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

void models_manager::remove_downloaded_files_on_error(
    const QString& path, models_manager::comp_type comp, int part) {
    QFileInfo fi{path};

    if (fi.isDir()) {
        fi.absoluteDir().removeRecursively();
    } else {
        QFile::remove(download_filename(path, comp, part));
        if (part > 0) {
            for (int i = 0; i < part; ++i) {
                QFile::remove(download_filename(path, comp, i));
            }
        }
    }
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

        remove_downloaded_files_on_error(path, comp, part);

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
                         << ", type:" << type << ", next_type:" << next_type;

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
            qDebug() << "successfully downloaded:" << id << ", type:" << type
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
    if (!str.compare(QStringLiteral("targz"), Qt::CaseInsensitive))
        return comp_type::targz;
    if (!str.compare(QStringLiteral("zip"), Qt::CaseInsensitive))
        return comp_type::zip;
    if (!str.compare(QStringLiteral("zipall"), Qt::CaseInsensitive))
        return comp_type::zipall;
    if (!str.compare(QStringLiteral("dir"), Qt::CaseInsensitive))
        return comp_type::dir;
    return comp_type::none;
}

QString models_manager::download_filename(QString filename, comp_type comp,
                                          int part, const QUrl& url) {
    switch (comp) {
        case comp_type::gz:
            filename += QStringLiteral(".gz");
            break;
        case comp_type::xz:
            filename += QStringLiteral(".xz");
            break;
        case comp_type::tar:
            filename += QStringLiteral(".tar");
            break;
        case comp_type::tarxz:
            filename += QStringLiteral(".tar.xz");
            break;
        case comp_type::targz:
            filename += QStringLiteral(".tar.gz");
            break;
        case comp_type::zip:
        case comp_type::zipall:
            filename += QStringLiteral(".zip");
            break;
        case comp_type::dir:
            if (auto f = url.fileName(); !f.isEmpty())
                filename = QDir{filename}.absoluteFilePath(f);
            break;
        case comp_type::none:
            break;
    }

    if (comp != comp_type::dir && part > -1) {
        filename +=
            QStringLiteral(".part-%1").arg(part, 2, 10, QLatin1Char{'0'});
    }

    return filename;
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
    for (const auto& [_, model] : models) {
        if (!model.hidden) existing_langs.insert(model.lang_id);
    }

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

models_manager::model_role models_manager::role_of_engine(model_engine engine) {
    switch (engine) {
        case model_engine::stt_ds:
        case model_engine::stt_vosk:
        case model_engine::stt_whisper:
            return model_role::stt;
        case model_engine::ttt_hftc:
            return model_role::ttt;
        case model_engine::tts_coqui:
        case model_engine::tts_piper:
        case model_engine::tts_espeak:
        case model_engine::tts_rhvoice:
            return model_role::tts;
    }

    throw std::runtime_error("unknown engine");
}

models_manager::model_engine models_manager::engine_from_name(
    const QString& name) {
    if (name == QStringLiteral("stt_ds")) return model_engine::stt_ds;
    if (name == QStringLiteral("stt_vosk")) return model_engine::stt_vosk;
    if (name == QStringLiteral("stt_whisper")) return model_engine::stt_whisper;
    if (name == QStringLiteral("ttt_hftc")) return model_engine::ttt_hftc;
    if (name == QStringLiteral("tts_coqui")) return model_engine::tts_coqui;
    if (name == QStringLiteral("tts_piper")) return model_engine::tts_piper;
    if (name == QStringLiteral("tts_espeak")) return model_engine::tts_espeak;
    if (name == QStringLiteral("tts_rhvoice")) return model_engine::tts_rhvoice;

    throw std::runtime_error("unknown engine: " + name.toStdString());
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

        model_engine engine = model_engine::stt_ds;
        comp_type comp = comp_type::none, scorer_comp = comp_type::none;
        QString file_name, checksum, checksum_quick, scorer_file_name,
            scorer_checksum, scorer_checksum_quick;
        std::vector<QUrl> urls, scorer_urls;
        qint64 size = 0, scorer_size = 0;
        int score = obj.value(QLatin1String{"score"}).toInt(-1);
        bool available = false, exists = false;
        QString speaker = obj.value(QLatin1String{"speaker"}).toString();

        auto model_alias_of =
            obj.value(QLatin1String{"model_alias_of"}).toString();
        if (model_alias_of.isEmpty()) {
            engine =
                engine_from_name(obj.value(QLatin1String{"engine"}).toString());

            if (speaker.isEmpty() && engine == model_engine::tts_espeak)
                speaker = lang_id;

            if (score == -1)
                score = engine == model_engine::tts_espeak
                            ? default_score_espeak
                            : default_score;

            file_name = obj.value(QLatin1String{"file_name"}).toString();
            if (file_name.isEmpty())
                file_name = file_name_from_id(model_id, engine);
            checksum = obj.value(QLatin1String{"checksum"}).toString();
            if (engine != model_engine::tts_espeak && checksum.isEmpty()) {
                qWarning() << "checksum cannot be empty:" << model_id;
                continue;
            }
            checksum_quick =
                obj.value(QLatin1String{"checksum_quick"}).toString();
            size = obj.value(QLatin1String{"size"}).toString().toLongLong();
            comp = str2comp(obj.value(QLatin1String{"comp"}).toString());

            auto aurls = obj.value(QLatin1String{"urls"}).toArray();
            if (aurls.isEmpty()) {
                if (engine == model_engine::tts_espeak) {
                    file_name.clear();
                } else {
                    qWarning() << "urls should be non empty array";
                    continue;
                }
            }
            for (const auto& url : aurls) urls.emplace_back(url.toString());

            scorer_file_name =
                obj.value(QLatin1String{"scorer_file_name"}).toString();
            if (scorer_file_name.isEmpty())
                scorer_file_name = scorer_file_name_from_id(model_id);
            scorer_checksum =
                obj.value(QLatin1String{"scorer_checksum"}).toString();
            scorer_checksum_quick =
                obj.value(QLatin1String{"scorer_checksum_quick"}).toString();
            scorer_size =
                obj.value(QLatin1String{"scorer_size"}).toString().toLongLong();
            scorer_comp =
                str2comp(obj.value(QLatin1String{"scorer_comp"}).toString());

            auto ascorer_urls =
                obj.value(QLatin1String{"scorer_urls"}).toArray();
            for (auto url : ascorer_urls)
                scorer_urls.emplace_back(url.toString());

        } else if (models.count(model_alias_of) > 0) {
            const auto& alias = models.at(model_alias_of);

            engine = alias.engine;
            if (score == -1) score = alias.score;
            file_name = alias.file_name;
            checksum = alias.checksum;
            checksum_quick = alias.checksum_quick;
            size = alias.size;
            comp = alias.comp;
            urls = alias.urls;
            scorer_file_name = alias.scorer_file_name;
            scorer_checksum = alias.scorer_checksum;
            scorer_checksum_quick = alias.scorer_checksum_quick;
            scorer_size = alias.scorer_size;
            scorer_comp = alias.scorer_comp;
            scorer_urls = alias.scorer_urls;
            if (speaker.isEmpty()) speaker = alias.speaker;
            exists = alias.exists;
            available = alias.available;
        } else {
            qWarning() << "invalid model alias:" << model_alias_of;
            continue;
        }

#ifdef ARCH_ARM_32
        if (!has_neon_fp && engine == model_engine::stt_whisper) {
            qDebug() << "ignoring whisper model because cpu does not support "
                        "neon fd:"
                     << model_id;
            continue;
        }
#endif
#ifdef USE_SFOS
        if (engine == model_engine::stt_vosk && model_id.contains("large")) {
            qDebug() << "ignoring vosk large model:" << model_id;
            continue;
        }
        if (engine == model_engine::stt_whisper &&
            (model_id.contains("small") || model_id.contains("medium") ||
             model_id.contains("large"))) {
            qDebug() << "ignoring whisper small, medium or large model:"
                     << model_id;
            continue;
        }
        if (engine == model_engine::stt_vosk && model_id.contains("large")) {
            qDebug() << "ignoring vosk large model:" << model_id;
            continue;
        }
        if (engine == model_engine::tts_coqui) {
            qDebug() << "ignoring coqui model:" << model_id;
            continue;
        }
#endif
#ifndef USE_PY
        if (engine == model_engine::ttt_hftc) {
            qDebug() << "ignoring hftc model:" << model_id;
            continue;
        }
#endif
        bool is_default_model_for_lang = [&] {
            switch (role_of_engine(engine)) {
                case model_role::stt:
                    return settings::instance()->default_stt_model_for_lang(
                               lang_id) == model_id;
                case model_role::tts:
                    return settings::instance()->default_tts_model_for_lang(
                               lang_id) == model_id;
                case model_role::ttt:
                    return false;
            }
            return false;
        }();

        priv_model_t model{
            /*engine=*/engine,
            /*lang_id=*/std::move(lang_id),
            /*name=*/obj.value(QLatin1String{"name"}).toString(),
            /*file_name=*/std::move(file_name),
            /*checksum=*/std::move(checksum),
            /*checksum_quick=*/std::move(checksum_quick),
            /*comp=*/comp,
            /*urls=*/std::move(urls),
            /*size=*/size,
            /*scorer_file_name=*/std::move(scorer_file_name),
            /*scorer_checksum=*/std::move(scorer_checksum),
            /*scorer_checksum_quick=*/std::move(scorer_checksum_quick),
            /*scorer_comp=*/scorer_comp,
            /*scorer_urls=*/std::move(scorer_urls),
            /*scorer_size=*/scorer_size,
            /*speaker=*/speaker,
            /*score=*/score,
            /*hidden=*/obj.value(QLatin1String{"hidden"}).toBool(false),
            /*default_for_lang=*/is_default_model_for_lang,
            /*exists=*/exists,
            /*available=*/available,
            /*downloading=*/false};

        if (!model.exists && dir.exists(model.file_name)) {
            if (checksum_ok(model.checksum, model.checksum_quick,
                            model.file_name)) {
                if (model.scorer_urls.empty()) {
                    model.exists = true;
                } else if (dir.exists(model.scorer_file_name)) {
                    if (checksum_ok(model.scorer_checksum,
                                    model.scorer_checksum_quick,
                                    model.scorer_file_name)) {
                        model.exists = true;
                    }
                }
            }
        }

        if (!model.exists && engine == model_engine::tts_espeak &&
            model.file_name.isEmpty() && model.checksum.isEmpty()) {
            // espeak without model
            model.exists = true;
        }

        if (model.exists) {
            if (model_alias_of.isEmpty())
                qDebug() << "found model:" << model_id;
            else
                qDebug() << "found model:" << model_id << "alias of"
                         << model_alias_of;
        }

        auto model_is_enabled = enabled_models.contains(model_id);
        if (model.exists) {
            model.available = model_is_enabled;
        } else {
            if (model_is_enabled) enabled_models.removeAll(model_id);
            if (is_default_model_for_lang) {
                switch (role_of_engine(engine)) {
                    case model_role::stt:
                        settings::instance()->set_default_stt_model_for_lang(
                            model.lang_id, {});
                        break;
                    case model_role::tts:
                        settings::instance()->set_default_tts_model_for_lang(
                            model.lang_id, {});
                        break;
                    case model_role::ttt:
                        break;
                }
            }
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

        qDebug() << "models changed";
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
        case model_engine::ttt_hftc:
        case model_engine::tts_coqui:
        case model_engine::tts_piper:
        case model_engine::tts_espeak:
        case model_engine::tts_rhvoice:
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

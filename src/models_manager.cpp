/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "models_manager.h"

#include <archive.h>
#include <archive_entry.h>
#include <lzma.h>
#include <zlib.h>

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
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "info.h"
#include "settings.h"

models_manager::models_manager(QObject* parent) : QObject{parent} {
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    m_nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

    connect(settings::instance(), &settings::models_dir_changed, this,
            static_cast<bool (models_manager::*)()>(
                &models_manager::parse_models_file_might_reset));

    parse_models_file_might_reset();
}

models_manager::~models_manager() { m_thread.join(); }

bool models_manager::ok() const { return !m_models.empty(); }

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
                    {pair.first, pair.second.lang_id, pair.second.name,
                     pair.second.scorer_file_name.isEmpty()
                         ? ""
                         : dir.filePath(pair.second.scorer_file_name),
                     pair.second.scorer_file_name.isEmpty()
                         ? ""
                         : dir.filePath(pair.second.scorer_file_name),
                     pair.second.experimental, pair.second.available,
                     pair.second.downloading, pair.second.download_progress});
            }
        });

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        if (a.experimental && !b.experimental) return false;
        if (!a.experimental && b.experimental) return true;
        return QString::compare(a.id, b.id, Qt::CaseInsensitive) < 0;
    });

    return list;
}

std::vector<models_manager::model_t> models_manager::available_models() const {
    std::vector<model_t> list;

    QDir dir{settings::instance()->models_dir()};

    for (const auto& [id, model] : m_models) {
        auto model_file = dir.filePath(model.file_name);
        if (model.available && QFile::exists(model_file)) {
            list.push_back({id, model.lang_id, model.name, model_file,
                            model.scorer_file_name.isEmpty()
                                ? QString{}
                                : dir.filePath(model.scorer_file_name),
                            model.experimental, model.available,
                            model.downloading, model.download_progress});
        }
    }

    return list;
}

bool models_manager::model_exists(const QString& id) const {
    const auto it = m_models.find(id);

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
            checksum_ok(model.scorer_checksum, model.scorer_checksum_quick,
                        model.scorer_file_name);
        if ((type == download_type::all && m_cs && s_cs) ||
            (type == download_type::scorer && s_cs)) {
            qWarning() << "both model and scorer exist, download not needed";
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

    if (comp == comp_type::tarxz && url.hasQuery()) {
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
    auto reply = qobject_cast<QNetworkReply*>(sender());

    if (check_model_download_cancel(reply)) return;

    if (reply->bytesAvailable() > 0) {
        auto data = reply->readAll();
        auto out_file = static_cast<std::ofstream*>(
            reply->property("out_file").value<void*>());
        out_file->write(data.data(), data.size());
    }
}

bool models_manager::xz_decode(const QString& file_in,
                               const QString& file_out) {
    qDebug() << "extracting xz archive:" << file_in;

    if (std::ifstream input{file_in.toStdString(),
                            std::ios::in | std::ifstream::binary}) {
        if (std::ofstream output{file_out.toStdString(),
                                 std::ios::out | std::ifstream::binary}) {
            lzma_stream strm = LZMA_STREAM_INIT;
            if (lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, 0);
                ret != LZMA_OK) {
                qWarning() << "error initializing the xz decoder:" << ret;
                return false;
            }

            lzma_action action = LZMA_RUN;

            char buff_out[std::numeric_limits<unsigned short>::max()];
            char buff_in[std::numeric_limits<unsigned short>::max()];

            strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
            strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
            strm.avail_out = sizeof buff_out;
            strm.avail_in = 0;

            while (true) {
                if (strm.avail_in == 0 && input) {
                    strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
                    input.read(buff_in, sizeof buff_in);
                    strm.avail_in = input.gcount();
                }

                if (!input) action = LZMA_FINISH;

                auto ret = lzma_code(&strm, action);

                if (strm.avail_out == 0 || ret == LZMA_STREAM_END) {
                    output.write(buff_out, sizeof buff_out - strm.avail_out);

                    strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
                    strm.avail_out = sizeof buff_out;
                }

                if (ret == LZMA_STREAM_END) break;

                if (ret != LZMA_OK) {
                    qWarning() << "xz decoder error:" << ret;
                    lzma_end(&strm);
                    return false;
                }
            }

            lzma_end(&strm);

            return true;
        }
        qWarning() << "error opening out-file:" << file_out;
    } else {
        qWarning() << "error opening in-file:" << file_in;
    }

    return false;
}

bool models_manager::join_part_files(const QString& file_out, int parts) {
    qDebug() << "joining files:" << file_out;

    if (std::ofstream output{file_out.toStdString(),
                             std::ios::out | std::ifstream::binary}) {
        for (int i = 0; i < parts; ++i) {
            const auto file_in =
                download_filename(file_out, comp_type::none, i);
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

bool models_manager::gz_decode(const QString& file_in,
                               const QString& file_out) {
    qDebug() << "extracting gz archive:" << file_in;

    if (std::ifstream input{file_in.toStdString(),
                            std::ios::in | std::ifstream::binary}) {
        if (std::ofstream output{file_out.toStdString(),
                                 std::ios::out | std::ifstream::binary}) {
            z_stream strm;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;

            if (int ret = inflateInit2(&strm, MAX_WBITS | 16); ret != Z_OK) {
                qWarning() << "error initializing the gzip decoder:" << ret;
                return false;
            }

            char buff_out[std::numeric_limits<unsigned short>::max()];
            char buff_in[std::numeric_limits<unsigned short>::max()];

            strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
            strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
            strm.avail_out = sizeof buff_out;
            strm.avail_out = sizeof buff_out;
            strm.avail_in = 0;

            while (true) {
                if (strm.avail_in == 0 && input) {
                    strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
                    input.read(buff_in, sizeof buff_in);
                    strm.avail_in = input.gcount();
                }

                if (input.bad()) {
                    inflateEnd(&strm);
                    return false;
                }

                auto ret = inflate(&strm, Z_NO_FLUSH);

                if (strm.avail_out == 0 || ret == Z_STREAM_END) {
                    output.write(buff_out, sizeof buff_out - strm.avail_out);

                    strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
                    strm.avail_out = sizeof buff_out;
                }

                if (ret == Z_STREAM_END) break;

                if (ret != Z_OK) {
                    qWarning() << "gzip decoder error:" << ret;
                    inflateEnd(&strm);
                    return false;
                }
            }

            inflateEnd(&strm);

            return true;
        } else {
            qWarning() << "error opening out-file:" << file_out;
        }
    } else {
        qWarning() << "error opening in-file:" << file_in;
    }

    return false;
}

// source: https://github.com/libarchive/libarchive/blob/master/examples/untar.c
static int copy_data(struct archive* ar, struct archive* aw) {
    int r;
    const void* buff;
    size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
    int64_t offset;
#else
    off_t offset;
#endif

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF) return (ARCHIVE_OK);
        if (r != ARCHIVE_OK) return (r);
        r = archive_write_data_block(aw, buff, size, offset);
        if (r != ARCHIVE_OK) {
            return (r);
        }
    }
}

bool models_manager::tar_decode(const QString& file_in,
                                std::map<QString, QString>&& files_out) {
    qDebug() << "extracting tar archive:" << file_in;

    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();
    archive_read_support_format_tar(a);

    bool ok = true;

    if (archive_read_open_filename(a, file_in.toStdString().c_str(), 10240)) {
        qWarning() << "error opening in-file:" << file_in
                   << archive_error_string(a);
        ok = false;
    } else {
        struct archive_entry* entry;
        while (true) {
            int ret = archive_read_next_header(a, &entry);
            if (ret == ARCHIVE_EOF) break;
            if (ret != ARCHIVE_OK) {
                qWarning() << "error archive_read_next_header:" << file_in
                           << archive_error_string(a);
                ok = false;
                break;
            }

            const QString entry_path{archive_entry_pathname_utf8(entry)};

            qDebug() << "found file in tar archive:" << entry_path;

            if (const auto it = files_out.find(entry_path);
                it != files_out.cend()) {
                qDebug() << "extracting file:" << entry_path << "to"
                         << it->second;

                const auto std_file_out = it->second.toStdString();
                archive_entry_set_pathname(entry, std_file_out.c_str());

                ret = archive_write_header(ext, entry);
                if (ret != ARCHIVE_OK) {
                    qWarning() << "error archive_write_header:" << file_in
                               << archive_error_string(ext);
                    ok = false;
                    break;
                }

                copy_data(a, ext);
                ret = archive_write_finish_entry(ext);
                if (ret != ARCHIVE_OK) {
                    qWarning() << "error archive_write_finish_entry:" << file_in
                               << archive_error_string(ext);
                    ok = false;
                    break;
                }

                files_out.erase(it);
                if (files_out.empty()) break;
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return ok;
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

    std::thread thread{[&] {
        if (parts > 1) {
            qDebug() << "joining parts:" << parts;
            join_part_files(download_filename(path, comp), parts);
            for (int i = 0; i < parts; ++i)
                QFile::remove(download_filename(path, comp, i));
        }

        auto comp_file = download_filename(path, comp);
        qDebug() << "total downloaded size:" << QFileInfo{comp_file}.size();

        bool ok_2 = true;

        if (comp != comp_type::none) {
            if (comp == comp_type::gz) {
                gz_decode(comp_file, path);
                QFile::remove(comp_file);
            } else if (comp == comp_type::xz) {
                xz_decode(comp_file, path);
                QFile::remove(comp_file);
            } else if (comp == comp_type::tarxz) {
                auto tar_file = download_filename(path, comp_type::tar);
                xz_decode(comp_file, tar_file);
                QFile::remove(comp_file);
                if (!path_in_archive_2.isEmpty() && !path_2.isEmpty()) {
                    tar_decode(tar_file, {{path_in_archive, path},
                                          {path_in_archive_2, path_2}});
                    ok_2 = make_checksum(path_2) == checksum_2;
                    make_quick_checksum(path_2);  // log quick checksum
                    if (!ok_2) qWarning() << "checksum 2 is invalid";
                } else {
                    tar_decode(tar_file, {{path_in_archive, path}});
                }
                QFile::remove(tar_file);
            } else {
                QFile::remove(comp_file);
            }
        }

        if (ok_2) {
            ok = make_checksum(path) == checksum;
            make_quick_checksum(path);  // log quick checksum
            if (!ok) qWarning() << "checksum is invalid";
        }

        loop.quit();
    }};

    loop.exec();
    thread.join();

    return ok;
}

bool models_manager::check_model_download_cancel(QNetworkReply* reply) {
    const auto id = reply->property("model_id").toString();

    auto it = models_to_cancel.find(id);
    if (it == models_to_cancel.end()) return false;
    models_to_cancel.erase(it);

    reply->abort();
    return true;
}

void models_manager::handle_download_finished() {
    auto reply = qobject_cast<QNetworkReply*>(sender());

    delete static_cast<std::ofstream*>(
        reply->property("out_file").value<void*>());

    const auto id = reply->property("model_id").toString();

    auto& model = m_models.at(id);
    const auto type =
        static_cast<download_type>(reply->property("download_type").toInt());
    const auto path = reply->property("out_path").toString();
    const auto comp = static_cast<comp_type>(reply->property("comp").toInt());
    const auto part = reply->property("part").toInt();

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
        const auto next_part = reply->property("next_part").toInt();

        if (next_part < 0) {
            const auto parts = type == download_type::scorer
                                   ? model.scorer_urls.size()
                                   : model.urls.size();
            const auto downloaded_part_data =
                QFileInfo{download_filename(path, comp, part)}.size();
            const auto path_2 = reply->property("out_path_2").toString();
            const auto checksum = reply->property("checksum").toString();
            const auto checksum_2 = reply->property("checksum_2").toString();
            const auto path_in_archive =
                reply->property("path_in_archive").toString();
            const auto path_in_archive_2 =
                reply->property("path_in_archive_2").toString();

            if (handle_download(path, checksum, path_in_archive, path_2,
                                checksum_2, path_in_archive_2, comp, parts)) {
                const auto next_type = static_cast<download_type>(
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

                model.available = true;
                emit download_finished(id);
            } else {
                QFile::remove(path);
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

QString models_manager::make_quick_checksum(const QString& file) {
    if (std::ifstream input{file.toStdString(), std::ios::in |
                                                    std::ifstream::binary |
                                                    std::ios::ate}) {
        const auto chunk = static_cast<std::ifstream::pos_type>(
            std::numeric_limits<unsigned short>::max());
        const auto end_pos = input.tellg();
        if (end_pos < 2 * chunk) {
            qWarning() << "file size too short for quick checksum";
            return {};
        }

        auto checksum = crc32(0L, Z_NULL, 0);
        char buff[std::numeric_limits<unsigned short>::max()];

        input.seekg(end_pos - chunk);
        input.read(buff, sizeof buff);
        checksum = crc32(checksum, reinterpret_cast<unsigned char*>(buff),
                         static_cast<unsigned int>(input.gcount()));
        input.seekg(0);
        input.read(buff, sizeof buff);
        checksum = crc32(checksum, reinterpret_cast<unsigned char*>(buff),
                         static_cast<unsigned int>(input.gcount()));

        std::stringstream ss;
        ss << std::hex << checksum;
        auto hex = QString::fromStdString(ss.str());
        qDebug() << "crc quick checksum:" << hex << file;
        return hex;
    }

    qWarning() << "cannot open file:" << file;

    return {};
}

QString models_manager::make_checksum(const QString& file) {
    if (std::ifstream input{file.toStdString(),
                            std::ios::in | std::ifstream::binary}) {
        auto checksum = crc32(0L, Z_NULL, 0);
        char buff[std::numeric_limits<unsigned short>::max()];
        while (input) {
            input.read(buff, sizeof buff);
            checksum = crc32(checksum, reinterpret_cast<unsigned char*>(buff),
                             static_cast<unsigned int>(input.gcount()));
        }
        std::stringstream ss;
        ss << std::hex << checksum;
        auto hex = QString::fromStdString(ss.str());
        qDebug() << "crc checksum:" << hex << file;
        return hex;
    }
    qWarning() << "cannot open file:" << file;

    return {};
}

void models_manager::handle_download_progress(qint64 received,
                                              qint64 real_total) {
    const auto reply = qobject_cast<const QNetworkReply*>(sender());

    if (reply->isFinished()) return;

    const auto id = reply->property("model_id").toString();

    auto total = reply->property("size").toLongLong();
    if (total <= 0) total = real_total;

    if (total > 0) {
        auto& model = m_models.at(id);
        const auto new_download_progress =
            static_cast<double>(received + model.downloaded_part_data) / total;
        if (new_download_progress - model.download_progress >= 0.01) {
            model.download_progress = new_download_progress;
            emit download_progress(id, new_download_progress);
        }
    }
}

void models_manager::delete_model(const QString& id) {
    if (const auto it = m_models.find(id); it != std::cend(m_models)) {
        auto& model = it->second;

        if (!std::any_of(
                m_models.cbegin(), m_models.cend(),
                [id = it->first, file_name = model.file_name](const auto& p) {
                    return p.second.available && p.first != id &&
                           p.second.file_name == file_name;
                })) {
            QFile::remove(model_path(model.file_name));
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
            QFile::remove(model_path(model.scorer_file_name));
        } else {
            qDebug() << "not removing scorer file because other model uses it:"
                     << model.scorer_file_name;
        }
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
        qWarning() << "cannot open default models file";
        return;
    }

    const QString data_dir{
        QStandardPaths::writableLocation(QStandardPaths::DataLocation)};
    QDir dir{data_dir};
    if (!dir.exists())
        if (!dir.mkpath(data_dir)) qWarning() << "unable to create data dir";

    QFile models{dir.filePath(models_file)};

    if (models.exists()) backup_config(QFileInfo{models}.absoluteFilePath());

    if (!models.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "cannot open models file";
        return;
    }

    auto data = default_models_file.readAll();
    models.write(data.replace("%VERSION%",
                              std::to_string(dsnote::CONF_VERSION).c_str()));
}

models_manager::comp_type models_manager::str2comp(const QString& str) {
    if (!str.compare(QStringLiteral("gz"), Qt::CaseInsensitive))
        return comp_type::gz;
    if (!str.compare(QStringLiteral("xz"), Qt::CaseInsensitive))
        return comp_type::xz;
    if (!str.compare(QStringLiteral("tarxz"), Qt::CaseInsensitive))
        return comp_type::tarxz;
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
        default:;
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

        const auto lang_id = obj.value(QLatin1String{"id"}).toString();
        if (lang_id.isEmpty()) {
            qWarning() << "empty model id";
            continue;
        }

        if (langs.find(lang_id) != langs.end()) {
            qWarning() << "duplicate lang id:" << lang_id;
            continue;
        }

        langs.insert({lang_id,
                      {obj.value(QLatin1String{"name"}).toString(),
                       obj.value(QLatin1String{"name_en"}).toString()}});
    }

    return langs;
}

bool models_manager::checksum_ok(const QString& checksum,
                                 const QString& checksum_quick,
                                 const QString& file_name) {
    if (checksum_quick.isEmpty()) {
        return checksum == make_checksum(model_path(file_name));
    }
    return checksum_quick == make_quick_checksum(model_path(file_name));
};

auto models_manager::extract_models(const QJsonArray& models_jarray) {
    models_t models;

    QDir dir{settings::instance()->models_dir()};

    for (const auto& ele : models_jarray) {
        auto obj = ele.toObject();
        const auto model_id = obj.value(QLatin1String{"model_id"}).toString();

        if (model_id.isEmpty()) {
            qWarning() << "empty model id in lang models file";
            continue;
        }

        if (models.find(model_id) != models.end()) {
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

        auto file_name = obj.value(QLatin1String{"file_name"}).toString();
        if (file_name.isEmpty()) file_name = file_name_from_id(model_id);

        auto scorer_file_name =
            obj.value(QLatin1String{"scorer_file_name"}).toString();
        if (scorer_file_name.isEmpty())
            scorer_file_name = scorer_file_name_from_id(model_id);

        priv_model_t model{
            std::move(lang_id),
            obj.value(QLatin1String{"name"}).toString(),
            std::move(file_name),
            std::move(checksum),
            obj.value(QLatin1String{"checksum_quick"}).toString(),
            str2comp(obj.value(QLatin1String{"comp"}).toString()),
            {},
            obj.value(QLatin1String{"size"}).toString().toLongLong(),
            std::move(scorer_file_name),
            obj.value(QLatin1String{"scorer_checksum"}).toString(),
            obj.value(QLatin1String{"scorer_checksum_quick"}).toString(),
            str2comp(obj.value(QLatin1String{"scorer_comp"}).toString()),
            {},
            obj.value(QLatin1String{"scorer_size"}).toString().toLongLong(),
            obj.value(QLatin1String{"experimental"}).toBool(),
            false,
            false};

        const auto urls = obj.value(QLatin1String{"urls"}).toArray();
        if (urls.isEmpty()) {
            qWarning() << "urls should be non empty array";
            continue;
        }
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

        models.insert({model_id, std::move(model)});
    }

    return models;
}

void models_manager::reload() {
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
        const std::vector<char> buff{std::istreambuf_iterator<char>{input}, {}};

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

            qDebug() << "config version:" << version << dsnote::CONF_VERSION;

            if (version != dsnote::CONF_VERSION) {
                qWarning("version mismatch, has %d but requires %d", version,
                         dsnote::CONF_VERSION);
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
            }
        }
    } else {
        qWarning() << "cannot open lang models file";
    }
}

inline QString models_manager::file_name_from_id(const QString& id) {
    return id + ".tflite";
}

inline QString models_manager::scorer_file_name_from_id(const QString& id) {
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
        case comp_type::none:
            return QLatin1String("none");
    }
    return QLatin1String("unknown");
}

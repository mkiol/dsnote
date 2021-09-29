/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "models_manager.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QByteArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCryptographicHash>
#include <QVariantList>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QEventLoop>

#include <fstream>
#include <utility>
#include <vector>
#include <iostream>
#include <string>
#include <functional>
#include <thread>

#include <lzma.h>
#include <zlib.h>

#include "settings.h"
#include "info.h"

const QString models_manager::lang_models_file{"lang_models.json"};

models_manager::models_manager(QObject *parent)
    : QObject(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

    connect(settings::instance(), &settings::lang_models_dir_changed, this,
            static_cast<void (models_manager::*)()>(&models_manager::parse_models_file));

    parse_models_file();
}

models_manager::~models_manager()
{
    thread.join();
}

bool models_manager::ok() const
{
    return !models.empty();
}

std::vector<models_manager::lang_t> models_manager::langs() const
{
    std::vector<lang_t> list;

    QDir dir{settings::instance()->lang_models_dir()};

    std::transform(models.cbegin(), models.cend(), std::back_inserter(list),
                [&dir](decltype(models)::value_type const& pair) {
        return lang_t{
            pair.first,
            pair.second.lang_id,
            pair.second.name,
            pair.second.scorer_file_name.isEmpty() ? "" : dir.filePath(pair.second.scorer_file_name),
            pair.second.scorer_file_name.isEmpty() ? "" : dir.filePath(pair.second.scorer_file_name),
            pair.second.available,
            pair.second.current_dl != download_type::none,
            pair.second.download_progress
        };
    });

    return list;
}

std::vector<models_manager::lang_t> models_manager::available_langs() const
{
    std::vector<lang_t> list;

    QDir dir{settings::instance()->lang_models_dir()};

    for (const auto& [id, model] : models) {
        const auto model_file = dir.filePath(model.file_name);
        if (model.available && QFile::exists(model_file))
            list.push_back({id,
                            model.lang_id,
                            model.name,
                            model_file,
                            model.scorer_file_name.isEmpty() ? "" : dir.filePath(model.scorer_file_name),
                            model.available,
                            model.current_dl != download_type::none,
                            model.download_progress});
    }

    return list;
}

bool models_manager::model_exists(const QString& id) const
{
    const auto it = models.find(id);

    return it != std::cend(models) &&
            QFile::exists(model_path(it->second.file_name));
}

void models_manager::download_model(const QString& id)
{
    download(id, download_type::all);
}

void models_manager::download(const QString& id, download_type type, int part)
{
    if (type != download_type::all && type != download_type::scorer) {
        qWarning() << "incorrect dl type requested:" << static_cast<int>(type);
        return;
    }

    if (auto it = models.find(id); it != std::end(models)) {
        auto& model = it->second;

        const auto& urls = type == download_type::all ? model.urls : model.scorer_urls;

        if (part < 0) {
            if (type != download_type::scorer) model.downloaded_part_data = 0;
            if (urls.size() > 1) part = 0;
        }

        const auto& url = urls.at(part < 0 ? 0 : part);

        qDebug() << "downloading:" << url;

        QString path;
        QString md5;
        comp_type comp;
        auto next_type = download_type::none;
        const auto next_part = part < 0 || part >= static_cast<int>(urls.size()) - 1 ? -1 : part + 1;
        const bool scorer_exists = !model.scorer_file_name.isEmpty() && !model.scorer_md5.isEmpty() && !url.isEmpty();
        const qint64 size = scorer_exists ? model.size + model.scorer_size : model.size;

        QNetworkRequest request{url};
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        QNetworkReply* reply = nam.get(request);

        if (type == download_type::all) {
            path = model_path(model.file_name);
            md5 = model.md5;
            comp = model.comp;
            type = download_type::model;

            if (next_part > 0) {
                next_type = download_type::all;
            } else if (scorer_exists) {
                next_type = download_type::scorer;
            }

            model.current_dl = next_type != download_type::none ? download_type::all : download_type::model;
        } else { // type == download_type::scorer
            path = model_path(model.scorer_file_name);
            md5 = model.scorer_md5;
            comp = model.scorer_comp;
            next_type = next_part > 0 ? download_type::scorer : download_type::none;
            model.current_dl = download_type::all;
        }

        auto out_file = new std::ofstream{download_filename(path, comp, part).toStdString(), std::ofstream::out};

        reply->setProperty("out_file", QVariant::fromValue(static_cast<void*>(out_file)));
        reply->setProperty("out_path", path);
        reply->setProperty("model_id", id);
        reply->setProperty("download_type", static_cast<int>(type));
        reply->setProperty("download_next_type", static_cast<int>(next_type));
        reply->setProperty("checksum", md5);
        reply->setProperty("comp", static_cast<int>(comp));
        reply->setProperty("size", size);
        reply->setProperty("part", part);
        reply->setProperty("next_part", next_part);

        connect(reply, &QNetworkReply::downloadProgress, this, &models_manager::handle_download_progress);
        connect(reply, &QNetworkReply::finished, this, &models_manager::handle_download_finished);
        connect(reply, &QNetworkReply::readyRead, this, &models_manager::handle_download_ready_read);
        connect(reply, &QNetworkReply::sslErrors, this, &models_manager::handle_ssl_errors);

        if (part < 0) emit models_changed();
    } else {
        qWarning() << "no model with id:" << id;
    }
}

void models_manager::handle_ssl_errors(const QList<QSslError> &errors)
{
    qWarning() << "ssl error:" << errors;

    // workaround for outdated cert db on Jolla 1
    static_cast<QNetworkReply*>(sender())->ignoreSslErrors();
}

void models_manager::handle_download_ready_read()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->bytesAvailable() > 0) {
        auto data = reply->readAll();
        auto out_file = static_cast<std::ofstream*>(reply->property("out_file").value<void*>());
        out_file->write(data.data(), data.size());
    }
}

bool models_manager::xz_decode(const QString& file_in, const QString& file_out)
{
    if (std::ifstream input{file_in.toStdString(), std::ios::in | std::ifstream::binary}) {
        if (std::ofstream output{file_out.toStdString(), std::ios::out | std::ifstream::binary}) {
            lzma_stream strm = LZMA_STREAM_INIT;
            if (lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, 0); ret != LZMA_OK) {
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
        } else {
            qWarning() << "error opening out-file:" << file_out;
        }
    } else {
        qWarning() << "error opening in-file:" << file_in;
    }

    return false;
}

bool models_manager::join_part_files(const QString& file_out, int parts)
{
    if (std::ofstream output{file_out.toStdString(), std::ios::out | std::ifstream::binary}) {
        for (int i = 0; i < parts; ++i) {
            const auto file_in = download_filename(file_out, comp_type::none, i);
            if (std::ifstream input{file_in.toStdString(), std::ios::in | std::ifstream::binary}) {
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

bool models_manager::gz_decode(const QString& file_in, const QString& file_out)
{
    if (std::ifstream input{file_in.toStdString(), std::ios::in | std::ifstream::binary}) {
        if (std::ofstream output{file_out.toStdString(), std::ios::out | std::ifstream::binary}) {
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

bool models_manager::check_download(const QString& file, const QString& checksum, comp_type comp, int parts)
{
    QEventLoop loop;

    bool ok = false;

    std::thread thread{[&file, &checksum, comp, &ok, &loop, parts] {
        if (parts > 1) {
            qDebug() << "joining parts:" << parts;
            join_part_files(download_filename(file, comp), parts);
            for (int i = 0; i < parts; ++i) QFile::remove(download_filename(file, comp, i));
        }

        auto comp_file = download_filename(file, comp);
        qDebug() << "total size after join:" << comp_file << QFileInfo{comp_file}.size();

        if (comp != comp_type::none) {
            if (comp == comp_type::gz) gz_decode(comp_file, file);
            else if (comp == comp_type::xz) xz_decode(comp_file, file);

            QFile::remove(comp_file);
        }
        ok = make_checksum(file) == checksum;
        loop.quit();
    }};

    loop.exec();
    thread.join();

    return ok;
}

void models_manager::handle_download_finished()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());

    delete static_cast<std::ofstream*>(reply->property("out_file").value<void*>());

    const auto id = reply->property("model_id").toString();

    if (models.find(id) == models.cend()) throw std::runtime_error("invalid id");

    auto& model = models[id];
    const auto type = static_cast<download_type>(reply->property("download_type").toInt());
    const auto path = reply->property("out_path").toString();
    const auto comp = static_cast<comp_type>(reply->property("comp").toInt());
    const auto part = reply->property("part").toInt();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "error in download:" << (type == download_type::scorer ? "scorer" : "model") << id
                   << "error type:" << reply->error();
        QFile::remove(download_filename(path, comp, part));
        if (part > 0) {
           for (int i = 0; i < part; ++i) QFile::remove(download_filename(path, comp, i));
        }

        emit download_error(id);
    } else {
        const auto next_part = reply->property("next_part").toInt();
        if (next_part < 0) {
            const auto parts = type == download_type::scorer ? model.scorer_urls.size() : model.urls.size();
            const auto downloaded_part_data = QFileInfo(download_filename(path, comp, part)).size();
            if (check_download(path, reply->property("checksum").toString(), comp, parts)) {
                qDebug() << "successfully downloaded:" << (type == download_type::scorer ? "scorer" : "model") << id;

                if (static_cast<download_type>(reply->property("download_next_type").toInt()) == download_type::scorer) {
                    model.downloaded_part_data += downloaded_part_data;
                    download(id, download_type::scorer);
                    reply->deleteLater();
                    return;
                } else {
                    model.available = true;
                    emit download_finished(id);
                }
            } else {
                qWarning() << "checksum is invalid:" << (type == download_type::scorer ? "scorer" : "model") << id;
                QFile::remove(path);
                emit download_error(id);
            }
        } else {
            qDebug() << "successfully downloaded:" << (type == download_type::scorer ? "scorer" : "model") << id << "part =" << part;
            model.downloaded_part_data += QFileInfo(download_filename(path, comp, part)).size();
            download(id, static_cast<download_type>(reply->property("download_next_type").toInt()), next_part);
            reply->deleteLater();
            return;
        }
    }

    reply->deleteLater();

    model.current_dl = download_type::none;
    model.download_progress = 0.0;
    model.downloaded_part_data = 0;
    emit models_changed();
}

QString models_manager::make_checksum(const QString& file)
{
    if (std::ifstream input{file.toStdString(), std::ios::in | std::ifstream::binary}) {
        QCryptographicHash hash{QCryptographicHash::Md5};
        char buff[std::numeric_limits<unsigned short>::max()];

        while (input) {
            input.read(buff, sizeof buff);
            hash.addData(buff, static_cast<int>(input.gcount()));
        }

        qDebug() << "md5 checksum:" << hash.result().toHex() << file;
        return hash.result().toHex();
    } else {
        qWarning() << "cannot open file:" << file;
    }

    return {};
}

void models_manager::handle_download_progress(qint64 received, qint64 real_total)
{
    const auto reply = qobject_cast<const QNetworkReply*>(sender());
    const auto id = reply->property("model_id").toString();

    auto total = reply->property("size").toLongLong();
    if (total <= 0) total = real_total;

    if (total > 0) {
        auto& model = models.at(id);
        received += model.downloaded_part_data;
        model.download_progress = static_cast<double>(received)/total;

        //qDebug("download progress for %s: %lld / %lld", id.toLatin1().data(), received, total);
        emit download_progress(id, model.download_progress);
    }
}

void models_manager::delete_model(const QString &id)
{
    if (const auto it = models.find(id); it != std::cend(models)) {
        auto& model = it->second;
        QFile::remove(model_path(model.file_name));
        QFile::remove(model_path(model.scorer_file_name));
        model.available = false;
        model.download_progress = 0.0;
        model.downloaded_part_data = 0;
        emit models_changed();
    } else {
        qWarning() << "no model with id:" << id;
    }
}

void models_manager::backup_config(const QString& lang_models_file)
{
    QString backup_file;
    QFileInfo lang_models_file_i{lang_models_file};
    auto backup_file_stem = lang_models_file_i.dir().filePath(lang_models_file_i.baseName());

    int i = 0;
    do {
        backup_file = backup_file_stem + ".old" + (i == 0 ? "" : QString::number(i)) + ".json";
        ++i;
    } while (QFile::exists(backup_file) && i < 1000);

    qDebug() << "making lang models file backup to:" << backup_file;

    QFile::remove(backup_file);
    QFile::copy(lang_models_file, backup_file);
}

void models_manager::init_config()
{
    const QString data_dir{QStandardPaths::writableLocation(QStandardPaths::DataLocation)};
    const auto lang_models_file_path = QDir{data_dir}.filePath(lang_models_file);

    if (QFile::exists(lang_models_file_path))
        backup_config(lang_models_file_path);

    QDir{}.mkpath(data_dir);

    /*
     M - mandatory
     O - optional

     { "version": <config_version>,
       "langs: [
       {
         "name": "<native language name (M)>",
         "model_id": "unique model id (M)>",
         "id": "<ISO 639-1 language code (M)>",
         "urls": "<array of download URL(s) of model file, might be compressd file(s) (M)>",
         "md5": "<md5 hash of (not compressed) model file (M)>",
         "file_name": "<file name of deep-speech model (O)>",
         "comp": <type of compression for model file provided in 'url', following are supported: 'xz', 'gz' (O)>
         "size": "<size in bytes of file provided in 'url' (O)>",
         "scorer_urls": "<array download URL(s) of scorer file, might be compressd file(s) (O)>",
         "scorer_md5": "<md5 hash of (not compressed) scorer file (M if scorer is provided)>",
         "scorer_file_name": "<file name of deep-speech scorer (O)>",
         "scorer_comp": <type of compression for scorer file provided in 'scorer_url', following are supported: 'xz', 'gz' (O)>
         "scorer_size": "<size in bytes of file provided in 'scorer_url' (O)>"
       } ]
    }
    */

    std::ofstream outfile{lang_models_file_path.toStdString(), std::ofstream::out | std::ofstream::trunc};
    outfile << "{\n\"version\": " << dsnote::CONF_VERSION << ",\n\"langs\": [\n"

#ifdef TF_LITE
            << "{ \"name\": \"Czech\", \"model_id\": \"cs\", \"id\": \"cs\", "
            << "\"md5\": \"10cbdafa216b498445034c9e861bfba4\", "
            << "\"urls\": [\"https://github.com/comodoro/deepspeech-cs/releases/download/2021-07-21/output_graph.tflite\"], "
            << "\"size\": \"47360928\", "
            << "\"scorer_md5\": \"a5e7e891276b1f539b7d9a9cb11ce966\", "
            << "\"scorer_urls\": [\"https://github.com/comodoro/deepspeech-cs/releases/download/2021-07-21/o4-500k-wnc-2011.scorer\"], "
            << "\"scorer_size\": \"539766416\"},\n"

            << "{ \"name\": \"English\", \"model_id\": \"en\", \"id\": \"en\", "
            << "\"md5\": \"afcc08e56f024655c30187a41c4e8c9c\", "
            << "\"urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.tflite\"], "
            << "\"size\": \"47331784\", "
            << "\"scorer_md5\": \"08a02b383a9bc93c8a8ad188dbf79bc9\", "
            << "\"scorer_urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.scorer\"], "
            << "\"scorer_size\": \"953363776\"},\n"

            << "{ \"name\": \"Deutsch\", \"model_id\": \"de\", \"id\": \"de\", "
            << "\"md5\": \"bc31379c31052392b2ea881eefede747\", \"comp\": \"gz\", "
            << "\"urls\": [\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.tflite.gz\"], "
            << "\"size\": \"20296313\", "
            << "\"scorer_md5\": \"e1fbc58d92c0872f7a1502d33416a23c\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-06\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-07\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-08\"], "
            << "\"scorer_size\": \"229904847\"},\n"

            << "{ \"name\": \"Español\", \"model_id\": \"es\", \"id\": \"es\", "
            << "\"md5\": \"cc618b45dd01b8a6cc6b1d781653f89a\", \"comp\": \"gz\", "
            << "\"urls\": [\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.tflite.gz\"], "
            << "\"size\": \"20430228\", "
            << "\"scorer_md5\": \"650e2325ebf70d08a69ae5bf238ad5bd\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-06\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-07\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-08\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-09\"], "
            << "\"scorer_size\": \"247688532\"},\n"

            << "{ \"name\": \"Français\", \"model_id\": \"fr\", \"id\": \"fr\", "
            << "\"md5\": \"fcf644611a833f4f8e9767b2ab6b16ea\", \"comp\": \"gz\", "
            << "\"urls\": [\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.tflite.gz\"], "
            << "\"size\": \"20519594\", "
            << "\"scorer_md5\": \"35224069b08e801c84051d65e810bdd1\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-06\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-07\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-08\"], "
            << "\"scorer_size\": \"225945743\"},\n"

            << "{ \"name\": \"Italiano\", \"model_id\": \"it\", \"id\": \"it\", "
            << "\"md5\": \"11c9980d444f04e28bff007fedbac882\", \"comp\": \"gz\", "
            << "\"urls\": [\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.tflite.gz\"], "
            << "\"size\": \"20542809\", "
            << "\"scorer_md5\": \"9b2df256185e442246159b33cd05fc2d\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/base.scorer.gz\"], "
            << "\"scorer_size\": \"5350776\"},\n"

            << "{ \"name\": \"Italiano (MozillaItalia)\", \"model_id\": \"it_mozz\", \"id\": \"it\", "
            << "\"md5\": \"48fec5a7ac237ce345e3b1e3f7b14e93\", \"comp\": \"gz\", "
            << "\"urls\": [\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.tflite.gz\"], "
            << "\"size\": \"20690678\", "
            << "\"scorer_md5\": \"7abbe30b5ee8591360c73a0a0cb47813\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-06\"], "
            << "\"scorer_size\": \"160714948\"},\n"

            << "{ \"name\": \"Polski\", \"model_id\": \"pl\", \"id\": \"pl\", "
            << "\"md5\": \"a56c693bb0d644af5dc53f0e59f0da76\", \"comp\": \"gz\", "
            << "\"urls\": [\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.tflite.gz\"], "
            << "\"size\": \"20752162\", "
            << "\"scorer_md5\": \"0984ebda29d9c51a87e5823bd301d980\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/base.scorer.gz\"], "
            << "\"scorer_size\": \"3000583\"},\n"

            << "{ \"name\": \"简体中文\", \"model_id\": \"zh-CN\", \"id\": \"zh-CN\", "
            << "\"md5\": \"5664793cafe796d0821a3e49d56eb797\", "
            << "\"urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models-zh-CN.tflite\"], "
            << "\"size\": \"47798728\", "
            << "\"scorer_md5\": \"628e68fd8e0dd82c4a840d56c4cdc661\", "
            << "\"scorer_urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models-zh-CN.scorer\"], "
            << "\"scorer_size\": \"67141744\"}\n"
#else
            << "{ \"name\": \"Czech\", \"model_id\": \"cs\", \"id\": \"cs\", "
            << "\"md5\": \"071c0cefc8484908770028752f04c692\", "
            << "\"urls\": [\"https://github.com/comodoro/deepspeech-cs/releases/download/2021-07-21/output_graph.pbmm\"], "
            << "\"size\": \"189031154\", "
            << "\"scorer_md5\": \"a5e7e891276b1f539b7d9a9cb11ce966\", "
            << "\"scorer_urls\": [\"https://github.com/comodoro/deepspeech-cs/releases/download/2021-07-21/o4-500k-wnc-2011.scorer\"], "
            << "\"scorer_size\": \"539766416\"},\n"

            << "{ \"name\": \"English\", \"model_id\": \"en\", \"id\": \"en\", "
            << "\"md5\": \"8b15ccb86d0214657e48371287b7a49a\", "
            << "\"urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.pbmm\"], "
            << "\"size\": \"188915987\", "
            << "\"scorer_md5\": \"08a02b383a9bc93c8a8ad188dbf79bc9\", "
            << "\"scorer_urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.scorer\"], "
            << "\"scorer_size\": \"953363776\"},\n"

            << "{ \"name\": \"Deutsch\", \"model_id\": \"de\", \"id\": \"de\", "
            << "\"md5\": \"ccb15318053a245487a15b90bf052cca\", \"comp\": \"gz\", "
            << "\"urls\": [ "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-00\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-01\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-02\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-03\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-04\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-05\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-06\"], "
            << "\"size\": \"175382475\", "
            << "\"scorer_md5\": \"e1fbc58d92c0872f7a1502d33416a23c\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-06\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-07\", "
            << "\"https://github.com/rhasspy/de_deepspeech-jaco/raw/master/model/base.scorer.gz.part-08\"], "
            << "\"scorer_size\": \"229904847\"},\n"

            << "{ \"name\": \"Español\", \"model_id\": \"es\", \"id\": \"es\", "
            << "\"md5\": \"8b0739839abd0f98f2638be166fb3b74\", \"comp\": \"gz\", "
            << "\"urls\": [ "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-00\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-01\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-02\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-03\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-04\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-05\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-06\"], "
            << "\"size\": \"175381280\", "
            << "\"scorer_md5\": \"650e2325ebf70d08a69ae5bf238ad5bd\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-06\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-07\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-08\", "
            << "\"https://github.com/rhasspy/es_deepspeech-jaco/raw/master/model/base.scorer.gz.part-09\"], "
            << "\"scorer_size\": \"247688532\"},\n"

            << "{ \"name\": \"Français\", \"model_id\": \"fr\", \"id\": \"fr\", "
            << "\"md5\": \"079fa68c49feff6aa2bd3cc22aab6226\", \"comp\": \"gz\", "
            << "\"urls\": [ "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-00\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-01\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-02\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-03\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-04\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-05\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-06\"], "
            << "\"size\": \"175494759\", "
            << "\"scorer_md5\": \"35224069b08e801c84051d65e810bdd1\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-06\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-07\", "
            << "\"https://github.com/rhasspy/fr_deepspeech-jaco/raw/master/model/base.scorer.gz.part-08\"], "
            << "\"scorer_size\": \"225945743\"},\n"

            << "{ \"name\": \"Italiano\", \"model_id\": \"it\", \"id\": \"it\", "
            << "\"md5\": \"ec10ea9d01cc9ab3135e4e5b0341821e\", \"comp\": \"gz\", "
            << "\"urls\": [ "
            << "\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-00\", "
            << "\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-01\", "
            << "\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-02\", "
            << "\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-03\", "
            << "\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-04\", "
            << "\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-05\", "
            << "\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-06\"], "
            << "\"size\": \"175358136\", "
            << "\"scorer_md5\": \"9b2df256185e442246159b33cd05fc2d\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [\"https://github.com/rhasspy/it_deepspeech-jaco/raw/master/model/base.scorer.gz\"], "
            << "\"scorer_size\": \"5350776\"},\n"

            << "{ \"name\": \"Italiano (MozillaItalia)\", \"model_id\": \"it_mozz\", \"id\": \"it\", "
            << "\"md5\": \"a36764f7f97c357a5bea5b92dbfa5abc\", \"comp\": \"gz\", "
            << "\"urls\": [ "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.pbmm.gz.part-00\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.pbmm.gz.part-01\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.pbmm.gz.part-02\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.pbmm.gz.part-03\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.pbmm.gz.part-04\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.pbmm.gz.part-05\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/output_graph.pbmm.gz.part-06\"], "
            << "\"size\": \"175423291\", "
            << "\"scorer_md5\": \"7abbe30b5ee8591360c73a0a0cb47813\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [ "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-00\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-01\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-02\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-03\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-04\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-05\", "
            << "\"https://github.com/rhasspy/it_deepspeech-mozillaitalia/raw/master/model/base.scorer.gz.part-06\"], "
            << "\"scorer_size\": \"160714948\"},\n"

            << "{ \"name\": \"Polski\", \"model_id\": \"pl\", \"id\": \"pl\", "
            << "\"md5\": \"69d0069a0d68f33f6634e8b2c0e06af6\", \"comp\": \"gz\", "
            << "\"urls\": [ "
            << "\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-00\", "
            << "\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-01\", "
            << "\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-02\", "
            << "\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-03\", "
            << "\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-04\", "
            << "\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-05\", "
            << "\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/output_graph.pbmm.gz.part-06\"], "
            << "\"size\": \"175414854\", "
            << "\"scorer_md5\": \"0984ebda29d9c51a87e5823bd301d980\", \"scorer_comp\": \"gz\", "
            << "\"scorer_urls\": [\"https://github.com/rhasspy/pl_deepspeech-jaco/raw/master/model/base.scorer.gz\"], "
            << "\"scorer_size\": \"3000583\"},\n"

            << "{ \"name\": \"简体中文\", \"model_id\": \"zh-CN\", \"id\": \"zh-CN\", "
            << "\"md5\": \"57b99451aaabbada2708e3b6a28e55c8\", "
            << "\"urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models-zh-CN.pbmm\"], "
            << "\"size\": \"190777619\", "
            << "\"scorer_md5\": \"628e68fd8e0dd82c4a840d56c4cdc661\", "
            << "\"scorer_urls\": [\"https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models-zh-CN.scorer\"], "
            << "\"scorer_size\": \"67141744\"}\n"
#endif
            << "]\n}\n";
    outfile.close();
}

models_manager::comp_type models_manager::str2comp(const QString& str)
{
    if (!str.compare("gz", Qt::CaseInsensitive)) return comp_type::gz;
    if (!str.compare("xz", Qt::CaseInsensitive)) return comp_type::xz;
    return comp_type::none;
}

QString models_manager::download_filename(const QString& filename, comp_type comp, int part)
{
    QString ret_name = filename;

    switch (comp) {
    case comp_type::gz: ret_name += ".gz"; break;
    case comp_type::xz: ret_name += ".xz"; break;
    default: ;
    }

    if (part > -1) {
        ret_name += QString(".part-%1").arg(part, 2, 10, QLatin1Char('0'));
    }

    return ret_name;
}

auto models_manager::check_lang_file(const QJsonArray& langs)
{
    std::map<QString, models_manager::model_t> models;

    QDir dir{settings::instance()->lang_models_dir()};

    for (const auto& ele : langs) {
        auto obj = ele.toObject();
        auto model_id = obj.value("model_id").toString();

        if (model_id.isEmpty()) {
            qWarning() << "empty model id in lang models file";
            continue;
        }

        if (models.find(model_id) != models.end()) {
            qWarning() << "duplicate model id in lang models file:" << model_id;
            continue;
        }

        auto lang_id = obj.value("id").toString();
        if (lang_id.isEmpty()) {
            qWarning() << "empty lang id in lang models file";
            continue;
        }

        auto md5 = obj.value("md5").toString();
        if (md5.isEmpty()) {
            qWarning() << "md5 checksum cannot be empty:" << model_id;
            continue;
        }

        auto file_name = obj.value("file_name").toString();
        if (file_name.isEmpty()) file_name = file_name_from_id(model_id);

        auto scorer_file_name = obj.value("scorer_file_name").toString();
        if (scorer_file_name.isEmpty()) scorer_file_name = scorer_file_name_from_id(model_id);

        model_t model {
            lang_id,
            obj.value("name").toString(),
            file_name,
            md5,
            str2comp(obj.value("comp").toString()),
            {},
            obj.value("size").toString().toLongLong(),
            scorer_file_name,
            obj.value("scorer_md5").toString(),
            str2comp(obj.value("scorer_comp").toString()),
            {},
            obj.value("scorer_size").toString().toLongLong(),
            false,
            download_type::none
        };

        auto urls = obj.value("urls").toArray();
        if (urls.isEmpty()) {
            qWarning() << "urls should be non empty array";
            continue;
        }
        for (auto url : urls) model.urls.emplace_back(url.toString());

        auto scorer_urls = obj.value("scorer_urls").toArray();
        for (auto url : scorer_urls) model.scorer_urls.emplace_back(url.toString());

        model.available = dir.exists(file_name) && md5 == make_checksum(dir.filePath(file_name)) &&
                ((dir.exists(scorer_file_name) && model.scorer_md5 == make_checksum(dir.filePath(scorer_file_name))) ||
                 model.scorer_urls.empty());

        models.insert({model_id, std::move(model)});
    }

    return models;
}

void models_manager::parse_models_file()
{
    bool expected = false;
    if (!busy_value.compare_exchange_strong(expected, true))
        return;

    emit busy_changed();

    if (thread.joinable())
        thread.join();

    thread = std::thread{[this]{
        models = parse_models_file(false);
        emit models_changed();

        busy_value.store(false);
        emit busy_changed();
    }};
}

std::map<QString, models_manager::model_t> models_manager::parse_models_file(bool reset)
{
    std::map<QString, models_manager::model_t> models;

    const auto lang_models_file_path = QDir{QStandardPaths::writableLocation(QStandardPaths::DataLocation)}
                                           .filePath(lang_models_file);
    if (!QFile::exists(lang_models_file_path))
        init_config();

    if (std::ifstream input{lang_models_file_path.toStdString(), std::ifstream::in | std::ifstream::binary}) {
        const std::vector<char> buff{std::istreambuf_iterator<char>{input}, {}};

        QJsonParseError err;
        auto json = QJsonDocument::fromJson(QByteArray::fromRawData(&buff[0], buff.size()), &err);
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "error parsing json:" << err.errorString();
            input.close();
            if (!reset) {
                init_config();
                models = parse_models_file(true);
            }
        } else {
            auto version = json.object().value("version").toInt();

            if (version < dsnote::CONF_VERSION) {
                qWarning("version mismatch, has %d but requires %d", version, dsnote::CONF_VERSION);
                input.close();
                if (!reset) {
                    init_config();
                    models = parse_models_file(true);
                }
            } else {
                models = check_lang_file(json.object().value("langs").toArray());
            }
        }
    } else {
        qWarning() << "cannot open lang models file";
    }

    return models;
}

QString models_manager::file_name_from_id(const QString& id)
{
#ifdef TF_LITE
    return id + ".tflite";
#else
    return id + ".pbmm";
#endif
}

QString models_manager::scorer_file_name_from_id(const QString& id)
{
    return id + ".scorer";
}

QString models_manager::model_path(const QString& file_name)
{
    return QDir{settings::instance()->lang_models_dir()}.filePath(file_name);
}

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "downloader.hpp"

#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTime>
#include <QTimer>

static const QByteArray user_agent = QByteArrayLiteral(
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_6) AppleWebKit/605.1.15 "
    "(KHTML, like Gecko) Version/12.1.2 Safari/605.1.15");

static void set_request_props(QNetworkRequest &request) {
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader(QByteArrayLiteral("Range"),
                         QByteArrayLiteral("bytes=0-"));
    request.setRawHeader(QByteArrayLiteral("Connection"),
                         QByteArrayLiteral("close"));
    request.setRawHeader(QByteArrayLiteral("User-Agent"), user_agent);
}

downloader::downloader(QObject *parent) : QObject{parent}, m_progress{0, 0} {}

bool downloader::download_to_file(const QUrl &url, const QString &output_path) {
    if (busy()) {
        qWarning() << "downloader is busy";
        return false;
    }

    m_cancel_requested = false;

    QFile output{output_path};
    if (!output.open(QIODevice::WriteOnly)) {
        qWarning() << "cannot open output file for writing:" << output_path;
        return false;
    }

    QNetworkRequest request;
    request.setUrl(url);
    set_request_props(request);

    m_reply = m_nam.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(start_timeout);

    auto con1 = connect(
        &timer, &QTimer::timeout, this,
        [this, &output] {
            if (m_reply && output.size() == 0) {
                qWarning() << "downloader start timeout => aborting";
                m_reply->abort();
            }
        },
        Qt::QueuedConnection);
    auto con2 = connect(
        m_reply, &QNetworkReply::readyRead, this,
        [this, &output] {
            auto *reply = qobject_cast<QNetworkReply *>(sender());
            if (!reply) return;
            if (m_cancel_requested ||
                QThread::currentThread()->isInterruptionRequested()) {
                qWarning() << "downloader cancel was requested";
                reply->abort();
                return;
            }
            output.write(reply->readAll());
        },
        Qt::QueuedConnection);
    auto con3 = connect(
        m_reply, &QNetworkReply::finished, this,
        [&loop, &timer] {
            timer.stop();
            loop.quit();
        },
        Qt::QueuedConnection);
    auto con4 = connect(
        m_reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 bytes_received, qint64 bytes_total) {
            auto *reply = qobject_cast<QNetworkReply *>(sender());
            if (!reply) return;

            m_progress = {bytes_received, bytes_total};
            emit progress_changed();
        },
        Qt::QueuedConnection);
    timer.start();
    loop.exec();
    timer.stop();
    disconnect(con1);
    disconnect(con2);
    disconnect(con3);
    disconnect(con4);

    if (auto err = m_reply->error(); err != QNetworkReply::NoError) {
        qWarning() << "downloader error:" << err << m_reply->url();
        m_reply->deleteLater();
        m_reply = nullptr;
        output.close();
        QFile::remove(output_path);
        return false;
    }

    output.write(m_reply->readAll());
    output.close();
    m_reply->deleteLater();
    m_reply = nullptr;

    m_progress.first = m_progress.second;
    emit progress_changed();

    if (output.size() == 0) {
        qWarning() << "downloaded file is empty";
        QFile::remove(output_path);
        return false;
    }

    return true;
}

static QString mime_from_reply(const QNetworkReply *reply) {
    auto mime =
        reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();

    auto list = mime.split(',');
    if (!list.isEmpty()) mime = list.last().trimmed();

    return mime;
}

downloader::data_t downloader::download_data(const QUrl &url) {
    if (busy()) {
        qWarning() << "downloaded is busy";
        return {};
    }

#ifdef QT_DEBUG
    qDebug() << "download data:" << url;
#endif

    m_cancel_requested = false;
    QNetworkRequest request;
    request.setUrl(url);
    set_request_props(request);

    m_reply = m_nam.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(http_timeout);

    auto con1 = connect(
        &timer, &QTimer::timeout, this,
        [this] {
            if (m_reply) {
                qWarning() << "downloader timeout => aborting";
                m_reply->abort();
            }
        },
        Qt::QueuedConnection);
    auto con2 = connect(
        m_reply, &QNetworkReply::readyRead, this,
        [this] {
            auto *reply = qobject_cast<QNetworkReply *>(sender());
            if (!reply) return;
            if (m_cancel_requested ||
                QThread::currentThread()->isInterruptionRequested()) {
                qWarning() << "downloader cancel was requested";
                reply->abort();
            }
        },
        Qt::QueuedConnection);
    auto con3 = connect(
        m_reply, &QNetworkReply::finished, this,
        [&loop, &timer] {
            timer.stop();
            loop.quit();
        },
        Qt::QueuedConnection);
    auto con4 = connect(
        m_reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 bytes_received, qint64 bytes_total) {
            m_progress = {bytes_received, bytes_total};
            emit progress_changed();
        },
        Qt::QueuedConnection);

    timer.start();
    loop.exec();
    timer.stop();
    disconnect(con1);
    disconnect(con2);
    disconnect(con3);
    disconnect(con4);

    QByteArray data;
    QString mime;

    if (auto err = m_reply->error(); err != QNetworkReply::NoError) {
        qWarning() << "downloader error:" << err << m_reply->url();
    } else {
        data = m_reply->readAll();
        mime = mime_from_reply(m_reply);
    }

    m_progress.first = m_progress.second;
    emit progress_changed();

    m_reply->deleteLater();
    m_reply = nullptr;

    return {std::move(mime), std::move(data)};
}

void downloader::cancel() {
    m_cancel_requested = true;
    if (m_reply) {
        qWarning() << "downloader cancel requested";
        m_reply->abort();
    }
}

void downloader::reset() {
    m_cancel_requested = false;
    m_progress = {0, 0};
}

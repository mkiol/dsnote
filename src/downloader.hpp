/* Copyright (C) 2023-2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <utility>

class downloader final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busy_changed)

   public:
    enum class error_t { no_error, aborted, other };

    struct data_t {
        QString mime;
        QByteArray bytes;
        error_t error = error_t::no_error;
    };

    explicit downloader(QObject *parent = nullptr);
    data_t download_data(const QUrl &url,
                         const QString &expected_content_type_pattern = {});
    bool download_to_file(const QUrl &url, const QString &output_path);
    void cancel();
    void reset();
    auto canceled() const { return m_cancel_requested; }
    bool busy() const { return m_reply && m_reply->isRunning(); }
    auto progress() const { return m_progress; }

   signals:
    void busy_changed();
    void progress_changed();

   private:
    static const int http_timeout = 10000;  // 10s
    static const int start_timeout = 5000;  // 5s
    bool m_cancel_requested = false;
    QNetworkReply *m_reply = nullptr;
    std::pair<int64_t, int64_t> m_progress;
    QNetworkAccessManager m_nam;
};

#endif  // DOWNLOADER_H

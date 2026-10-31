/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "icon_provider.hpp"

#include <sailfishapp.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QStandardPaths>
#include <mlite5/MGConfItem>

namespace {
static QString pathTo(const QString &dir) {
    return SailfishApp::pathTo(dir).toString(QUrl::RemoveScheme);
}
}  // namespace

icon_provider::icon_provider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap),
      m_theme_dir{theme_dir()} {}

QString icon_provider::theme_dir() {
    QString path;
    auto ratio =
        MGConfItem(QStringLiteral("/desktop/sailfish/silica/theme_pixel_ratio"))
            .value()
            .toDouble();
    qDebug() << "device pixel ratio:" << ratio;
    if (ratio == 1.0) {
        path = pathTo(QStringLiteral("images/z1.0"));
    } else if (ratio == 1.25) {
        path = pathTo(QStringLiteral("images/z1.25"));
    } else if (ratio == 1.5) {
        path = pathTo(QStringLiteral("images/z1.5"));
    } else if (ratio == 1.65 || ratio == 1.75 || ratio == 1.8) {
        path = pathTo(QStringLiteral("images/z1.75"));
    } else if (ratio == 2.0) {
        path = pathTo(QStringLiteral("images/z2.0"));
    } else {
        qWarning() << "unknown pixel ratio so, defaulting to 1.0";
        path = pathTo(QStringLiteral("images/z1.0"));
    }
    return path;
}

QString icon_provider::path_to_id(const QString &id) {
    QDir dir{theme_dir()};
    auto filepath = dir.absoluteFilePath(id.split('?').at(0) + ".png");
    if (!QFile::exists(filepath)) {
        // icon file does not exist => fallback to default icon
        filepath = dir.absoluteFilePath(QStringLiteral("icon-m-item.png"));
    }
    return filepath;
}

QPixmap icon_provider::requestPixmap(const QString &id, QSize *size,
                                     const QSize &requested_size) {
    auto parts = id.split('?');
    auto filepath = QStringLiteral("%1/%2.png").arg(m_theme_dir, parts.at(0));
    if (!QFile::exists(filepath)) {
        // icon file does not exist => fallback to default icon
        filepath = m_theme_dir + "/icon-m-item.png";
    }

    QPixmap pixmap{filepath};

    if (parts.size() > 1 && QColor::isValidColor(parts.at(1))) {
        QPainter painter{&pixmap};
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), parts.at(1));
        painter.end();
    }

    if (size) {
        *size = pixmap.size();
    }

    if (requested_size.width() > 0 && requested_size.height() > 0) {
        return pixmap.scaled(requested_size.width(), requested_size.height());
    }

    return pixmap;
}

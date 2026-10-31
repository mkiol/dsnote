/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ICON_PROVIDER_H
#define ICON_PROVIDER_H

#include <QPixmap>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>
#include <QUrl>

class icon_provider : public QQuickImageProvider {
   public:
    icon_provider();
    QPixmap requestPixmap(const QString &id, QSize *size,
                          const QSize &requested_size) override;

   private:
    QString m_theme_dir;
    QString cacheDir;

    static QString theme_dir();
    static QString path_to_id(const QString &id);
};

#endif  // ICON_PROVIDER_H

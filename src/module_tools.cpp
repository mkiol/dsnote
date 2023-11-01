/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "module_tools.hpp"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <cstdlib>

#include "checksum_tools.hpp"
#include "comp_tools.hpp"
#include "config.h"
#include "settings.h"

namespace module_tools {
QString unpacked_dir(const QString& name) {
    return QStringLiteral("%1/%2").arg(
        QStandardPaths::writableLocation(QStandardPaths::DataLocation), name);
}

bool init_module(const QString& name) {
    if (!module_unpacked(name)) {
        unpack_module(name);
        if (!module_unpacked(name)) {
            unpack_module(name);
            if (!module_unpacked(name)) {
                qWarning() << "failed to unpack module:" << name;
                return false;
            }
        }
    }

    return true;
}

QString path_to_dir_for_path(const QString& dir, const QString& path) {
    auto path_full = QStringLiteral(INSTALL_PREFIX "/share/%1/%2/%3")
                         .arg(APP_BINARY_ID, dir, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral(INSTALL_PREFIX "/share/%1/%2")
            .arg(APP_BINARY_ID, dir);

    path_full = QStringLiteral(INSTALL_PREFIX "/%1/%2").arg(dir, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral(INSTALL_PREFIX "/%1").arg(dir);

    path_full =
        QStringLiteral(INSTALL_PREFIX "/share/%1/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral(INSTALL_PREFIX "/share/%1").arg(APP_BINARY_ID);

    path_full = QStringLiteral("/usr/%1/%2").arg(dir, path);
    if (QFileInfo::exists(path_full)) return QStringLiteral("/usr/%1").arg(dir);

    path_full =
        QStringLiteral("/usr/share/%1/%2/%3").arg(APP_BINARY_ID, dir, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral("/usr/share/%1/%2").arg(APP_BINARY_ID, dir);

    path_full = QStringLiteral("/usr/share/%1/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral("/usr/share/%1").arg(APP_BINARY_ID);

    path_full = QStringLiteral("/usr/local/%1/%2").arg(dir, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral("/usr/local/%1").arg(dir);

    path_full = QStringLiteral("/usr/local/share/%1/%2/%3")
                    .arg(APP_BINARY_ID, dir, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral("/usr/local/share/%1/%2").arg(APP_BINARY_ID, dir);

    path_full =
        QStringLiteral("/usr/local/share/%1/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full))
        return QStringLiteral("/usr/local/share/%1").arg(APP_BINARY_ID);

    qWarning() << "can't find dir for path:" << dir << path;

    return QString{};
}

QString path_to_share_dir_for_path(const QString& path) {
    return path_to_dir_for_path(QStringLiteral("share"), path);
}

QString path_to_bin_dir_for_path(const QString& path) {
    return path_to_dir_for_path(QStringLiteral("bin"), path);
}

QString path_in_share_dir(const QString& path) {
    auto path_full = QStringLiteral(INSTALL_PREFIX "/share/%1").arg(path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full = QStringLiteral(INSTALL_PREFIX "/share/%1/share/%2")
                    .arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full =
        QStringLiteral(INSTALL_PREFIX "/share/%1/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full = QStringLiteral("/usr/share/%1").arg(path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full =
        QStringLiteral("/usr/share/%1/share/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full = QStringLiteral("/usr/share/%1/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full = QStringLiteral("/usr/local/share/%1").arg(path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full =
        QStringLiteral("/usr/local/share/%1/share/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full)) return path_full;

    path_full =
        QStringLiteral("/usr/local/share/%1/%2").arg(APP_BINARY_ID, path);
    if (QFileInfo::exists(path_full)) return path_full;

    qWarning() << "can't find path in share dir:" << path;

    return QString{};
}

QString module_file(const QString& name) {
    return path_in_share_dir(QStringLiteral("%1/%2.tar.xz")
                                 .arg(QStringLiteral(APP_BINARY_ID), name));
}

bool unpack_module(const QString& name) {
    qDebug() << "unpacking module:" << name;

    auto m_file = module_file(name);
    if (m_file.isEmpty()) {
        qWarning() << "module does not exist:" << name;
        return false;
    }

    auto unpack_dir =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    auto unpack_file = QStringLiteral("%1/%2.tar").arg(unpack_dir, name);

    QDir{QStringLiteral("%1/%2").arg(unpack_dir, name)}.removeRecursively();
    QFile::remove(unpack_file);

    if (!comp_tools::xz_decode(m_file, unpack_file)) {
        qWarning() << "failed to extract archive:" << m_file;
        QFile::remove(unpack_file);
        settings::instance()->set_module_checksum(name, {});
        return false;
    }

    if (!comp_tools::archive_decode(unpack_file, comp_tools::archive_type::tar,
                                    {unpack_dir, {}}, false)) {
        qWarning() << "failed to extract tar archive:" << unpack_file;
        QFile::remove(unpack_file);
        settings::instance()->set_module_checksum(name, {});
        return false;
    }

    settings::instance()->set_module_checksum(
        name, checksum_tools::make_checksum(m_file));

    QFile::remove(unpack_file);

    qDebug() << "module successfully unpacked:" << name;

    return true;
}

bool module_unpacked(const QString& name) {
    auto old_checksum = settings::instance()->module_checksum(name);
    if (old_checksum.isEmpty()) {
        qDebug() << "module checksum missing, need to unpack:" << name;
        return false;
    }

    auto m_file = module_file(name);
    if (m_file.isEmpty()) {
        qWarning() << "module does not exist:" << name;
        return false;
    }

    if (old_checksum != checksum_tools::make_checksum(m_file)) {
        qDebug() << "module checksum is invalid, need to unpack";
        return false;
    }

    if (!QFile::exists(unpacked_dir(name))) {
        qDebug() << "no unpacked dir:" << unpacked_dir(name);
        return false;
    }

    qDebug() << "module already unpacked:" << name;

    return true;
}
}  // namespace module_tools

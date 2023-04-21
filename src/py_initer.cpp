/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "py_initer.hpp"

#ifdef USE_SFOS
#include <QDebug>
#include <QFileInfo>
#include <cstdlib>

#include "checksum_tools.hpp"
#include "comp_tools.hpp"
#include "settings.h"
#endif

#ifdef USE_SFOS
bool py_initer::init_modules() {
    if (!modules_installed()) {
        unpack_modules();
        if (!modules_installed()) {
            qWarning() << "failed to init py modules";
            return false;
        }
    }

    auto py_path =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" +
        python_site_path;

    qDebug() << "setting env PYTHONPATH=" << py_path;

    setenv("PYTHONPATH", py_path.toStdString().c_str(), true);

    return true;
}

bool py_initer::unpack_modules() {
    qDebug() << "unpacking py modules";

    if (!QFileInfo::exists(python_archive_path)) {
        qWarning() << "py archive does not exist";
        return false;
    }

    auto python_unpack_path =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    auto unpack_path = python_unpack_path + "/python.tar";
    if (QFileInfo::exists(unpack_path)) QFile::remove(unpack_path);

    if (!comp_tools::xz_decode(python_archive_path, unpack_path)) {
        qWarning() << "failed to extract py archive";
        settings::instance()->set_python_modules_checksum({});
        return false;
    }

    if (!comp_tools::archive_decode(unpack_path, comp_tools::archive_type::tar,
                                    {python_unpack_path, {}}, false)) {
        qWarning() << "failed to extract py tar archive";
        QFile::remove(unpack_path);
        settings::instance()->set_python_modules_checksum({});
        return false;
    }

    settings::instance()->set_python_modules_checksum(
        checksum_tools::make_checksum(python_archive_path));

    QFile::remove(unpack_path);

    qDebug() << "py modules successfully unpacked";

    return true;
}

bool py_initer::modules_installed() {
    auto old_checksum = settings::instance()->python_modules_checksum();
    if (old_checksum.isEmpty()) {
        qDebug() << "py modules checksum missing, need to unpack";
        return false;
    }

    if (old_checksum != checksum_tools::make_checksum(python_archive_path)) {
        qDebug() << "py modules checksum is invalid, need to unpack";
        return false;
    }

    auto py_path =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" +
        python_site_path;

    if (!QFile::exists(py_path)) {
        qDebug() << "no py site dir";
        return false;
    }

    qDebug() << "py modules installed";

    return true;
}
#endif

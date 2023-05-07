/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PY_TOOLS_HPP
#define PY_TOOLS_HPP

#include <QString>

#include "config.h"

namespace py_tools {
inline static const auto python_archive_path =
    QStringLiteral("/usr/share/%1/lib/python.tar.xz").arg(APP_BINARY_ID);
inline static const auto python_site_path = "python3.8/site-packages";

bool unpack_modules();
bool modules_installed();
bool init_modules();
};      // namespace py_tools

#endif  // PY_TOOLS_HPP

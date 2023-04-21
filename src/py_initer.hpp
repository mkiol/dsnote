/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PY_INITER_HPP
#define PY_INITER_HPP

#ifdef USE_SFOS
#include <QStandardPaths>
#include <QString>
#endif

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include "config.h"

namespace py = pybind11;

class py_initer {
   public:
    py_initer() = default;
#ifdef USE_SFOS
    static bool init_modules();
#endif

   private:
#ifdef USE_SFOS
    inline static const auto python_archive_path =
        QStringLiteral("/usr/share/%1/lib/python.tar.xz").arg(APP_BINARY_ID);
    inline static const auto python_site_path = "python3.8/site-packages";
#endif
    py::scoped_interpreter m_py_interpreter{};
    py::gil_scoped_release m_py_gil_release{};

#ifdef USE_SFOS
    static bool unpack_modules();
    static bool modules_installed();
#endif
};

#endif  // PY_INITER_HPP

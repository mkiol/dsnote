/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PYTHON_MODULES_INITER_HPP
#define PYTHON_MODULES_INITER_HPP

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

class python_modules_initer {
   public:
    python_modules_initer();

   private:
#ifdef USE_SFOS
    inline static const auto python_archive_path =
        QStringLiteral("/usr/share/%1/lib/python.tar.xz").arg(APP_BINARY_ID);
    inline static const auto python_site_path = "python3.8/site-packages";
#endif
    std::optional<py::scoped_interpreter> m_py_interpreter;
    std::optional<py::gil_scoped_release> m_py_gil_release{};

#ifdef USE_SFOS
    static bool init();
    static bool unpack_modules();
    static bool modules_installed();
#endif
};

#endif  // PYTHON_MODULES_INITER_HPP

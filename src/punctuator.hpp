/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PUNCTUATOR_H
#define PUNCTUATOR_H

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <string>

namespace py = pybind11;

class punctuator {
   public:
    punctuator(const std::string& model_path, int device = -1);
    ~punctuator();
    std::string process(std::string text);

   private:
    std::optional<py::object> m_pipeline;
};

#endif  // PUNCTUATOR_H

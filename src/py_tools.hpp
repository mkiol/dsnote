/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PY_TOOLS_HPP
#define PY_TOOLS_HPP

#include <iostream>

namespace py_tools {
inline static const auto python_site_path = "python/site-packages";

struct libs_availability_t {
    bool coqui_tts = false;
    bool torch_cuda = false;
    bool faster_whisper = false;
    bool mimic3_tts = false;
    bool transformers = false;
    bool unikud = false;
    bool gruut_de = false;
    bool gruut_es = false;
    bool gruut_fa = false;
    bool gruut_fr = false;
    bool gruut_it = false;
    bool gruut_nl = false;
    bool gruut_ru = false;
    bool gruut_sw = false;
    bool mecab = false;
};

libs_availability_t libs_availability();
bool init_module();
}  // namespace py_tools

std::ostream& operator<<(std::ostream& os,
                         const py_tools::libs_availability_t& availability);

#endif  // PY_TOOLS_HPP

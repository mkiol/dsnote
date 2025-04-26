/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PY_TOOLS_HPP
#define PY_TOOLS_HPP

#include <iostream>

namespace py_tools {
enum class libs_scan_type_t { on, off_all_disabled, off_all_enabled };
inline static const auto python_site_path = "python/site-packages";

struct py_version_t {
    int major = 0;
    int minor = 0;
    int micro = 0;
};

struct libs_availability_t {
    py_version_t py_version;
    bool coqui_tts = false;
    bool torch_cuda = false;
    bool torch_hip = false;
    bool faster_whisper = false;
    bool ctranslate2_cuda = false;
    bool mimic3_tts = false;
    bool whisperspeech_tts = false;
    bool parler_tts = false;
    bool f5_tts = false;
    bool kokoro_tts = false;
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
    bool kokoro_ja = false;
    bool kokoro_zh = false;
    bool mecab = false;
};

libs_availability_t libs_availability(libs_scan_type_t scan_type);
bool init_module();
}  // namespace py_tools

std::ostream& operator<<(std::ostream& os,
                         const py_tools::libs_availability_t& availability);
std::ostream& operator<<(std::ostream& os, py_tools::py_version_t version);

#endif  // PY_TOOLS_HPP

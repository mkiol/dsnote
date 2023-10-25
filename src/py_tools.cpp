/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "py_tools.hpp"

#ifdef USE_PY
#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS
#endif

#include <QDebug>
#include <QStandardPaths>
#include <QString>

#include "logger.hpp"

#ifdef USE_PYTHON_MODULE
#include "module_tools.hpp"
#endif

std::ostream& operator<<(std::ostream& os,
                         const py_tools::libs_availability_t& availability) {
    os << "coqui-tts=" << availability.coqui_tts
       << ", faster-whisper=" << availability.faster_whisper
       << ", mimic3-tts=" << availability.mimic3_tts
       << ", transformers=" << availability.transformers
       << ", unikud=" << availability.unikud
       << ", gruut_de=" << availability.gruut_de
       << ", gruut_es=" << availability.gruut_es
       << ", gruut_fa=" << availability.gruut_fa
       << ", gruut_fr=" << availability.gruut_fr
       << ", gruut_nl=" << availability.gruut_nl
       << ", gruut_it=" << availability.gruut_it
       << ", gruut_ru=" << availability.gruut_ru
       << ", gruut_sw=" << availability.gruut_sw
       << ", mecab=" << availability.mecab
       << ", torch-cuda=" << availability.torch_cuda;

    return os;
}

namespace py_tools {
libs_availability_t libs_availability() {
    // run only in py thread

    libs_availability_t availability{};

#ifdef USE_PY
    namespace py = pybind11;
    using namespace pybind11::literals;

    try {
        py::module_::import("TTS");
        availability.coqui_tts = true;
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    try {
        auto torch = py::module_::import("torch.cuda");
        availability.torch_cuda = torch.attr("is_available")().cast<bool>();
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    try {
        py::module_::import("faster_whisper");
        availability.faster_whisper = true;
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    try {
        py::module_::import("mimic3_tts");
        availability.mimic3_tts = true;
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    try {
        py::module_::import("transformers");
        py::module_::import("accelerate");
        availability.transformers = true;
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    try {
        py::module_::import("unikud");
        availability.unikud = true;
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    try {
        py::module_::import("gruut");

        try {
            py::module_::import("gruut_lang_de");
            availability.gruut_de = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            py::module_::import("gruut_lang_es");
            availability.gruut_es = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            py::module_::import("gruut_lang_fr");
            availability.gruut_fr = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            py::module_::import("gruut_lang_it");
            availability.gruut_it = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            py::module_::import("gruut_lang_ru");
            availability.gruut_ru = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            py::module_::import("gruut_lang_fa");
            availability.gruut_fa = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            py::module_::import("gruut_lang_sw");
            availability.gruut_sw = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            py::module_::import("gruut_lang_nl");
            availability.gruut_nl = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    try {
        py::module_::import("MeCab");
        py::module_::import("unidic_lite");
        availability.mecab = true;
    } catch (const std::exception& err) {
        LOGD("py error: " << err.what());
    }

    LOGD("py libs availability: [" << availability << "]");
#endif

    return availability;
}

bool init_module() {
#ifdef USE_PYTHON_MODULE
    if (!module_tools::init_module(QStringLiteral("python"))) return false;

    auto py_path =
        QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" +
        python_site_path;

    qDebug() << "setting env PYTHONPATH=" << py_path;

    setenv("PYTHONPATH", py_path.toStdString().c_str(), true);

    return true;
#else
    return true;
#endif
}
}  // namespace py_tools

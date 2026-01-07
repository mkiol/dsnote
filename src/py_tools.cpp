/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
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

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "settings.h"

#ifdef USE_PYTHON_MODULE
#include "module_tools.hpp"
#endif

std::ostream& operator<<(std::ostream& os, py_tools::py_version_t version) {
    os << version.major << '.' << version.minor << '.' << version.micro;
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const py_tools::libs_availability_t& availability) {
    os << "py-version=" << availability.py_version
       << ", coqui-tts=" << availability.coqui_tts
       << ", faster-whisper=" << availability.faster_whisper
       << ", nemo-asr=" << availability.nemo_asr
       << ", ctranslate2-cuda=" << availability.ctranslate2_cuda
       << ", mimic3-tts=" << availability.mimic3_tts
       << ", whisperspeech-tts=" << availability.whisperspeech_tts
       << ", parler-tts=" << availability.parler_tts
       << ", f5-tts=" << availability.f5_tts
       << ", kokoro-tts=" << availability.f5_tts
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
       << ", kokoro_ja=" << availability.kokoro_ja
       << ", kokoro_zh=" << availability.kokoro_zh
       << ", mecab=" << availability.mecab << ", uroman=" << availability.uroman
       << ", torch-cuda=" << availability.torch_cuda
       << ", torch-hip=" << availability.torch_hip;

    return os;
}

namespace py_tools {
libs_availability_t libs_availability(libs_scan_type_t scan_type,
                                      unsigned int scan_flags) {
    // run only in py thread

    libs_availability_t availability{};

    switch (scan_type) {
        case libs_scan_type_t::on:
            break;
        case libs_scan_type_t::off_all_enabled:
            if (cpu_tools::cpuinfo().feature_flags &
                cpu_tools::feature_flags_t::avx) {
                availability.coqui_tts = true;
                availability.whisperspeech_tts = true;
                availability.parler_tts = true;
                availability.f5_tts = true;
                availability.kokoro_tts = true;
                availability.kokoro_ja = true;
                availability.kokoro_zh = true;
            }
            availability.faster_whisper = true;
            availability.mimic3_tts = true;
            availability.transformers = true;
            availability.unikud = true;
            availability.gruut_de = true;
            availability.gruut_es = true;
            availability.gruut_fa = true;
            availability.gruut_fr = true;
            availability.gruut_it = true;
            availability.gruut_nl = true;
            availability.gruut_ru = true;
            availability.gruut_sw = true;
            availability.mecab = true;
            availability.uroman = true;
            break;
        case libs_scan_type_t::off_all_disabled:
            return availability;
    }

#ifdef USE_PY
    namespace py = pybind11;
    using namespace pybind11::literals;

    try {
        LOGD("checking: python version");
        auto version_info = py::module_::import("sys").attr("version_info");
        availability.py_version.major = version_info.attr("major").cast<int>();
        availability.py_version.minor = version_info.attr("minor").cast<int>();
        availability.py_version.micro = version_info.attr("micro").cast<int>();
        LOGD("python version: " << availability.py_version);
    } catch (const std::exception& err) {
        LOGD("python version check py error: " << err.what());
    }

    if ((scan_flags & settings::ScanFlagNoTorchCuda) > 0) {
        LOGD("checking: torch cuda (skipped)");
    } else {
        if (cpu_tools::cpuinfo().feature_flags &
            cpu_tools::feature_flags_t::avx) {
            try {
                LOGD("checking: torch cuda");
                auto torch_cuda = py::module_::import("torch.cuda");
                auto torch_ver = py::module_::import("torch.version");
                if (torch_cuda.attr("is_available")().cast<bool>()) {
                    try {
                        auto cuda_ver =
                            torch_ver.attr("cuda").cast<std::string>();
                        LOGD("torch cuda version: " << cuda_ver);
                        availability.torch_cuda = !cuda_ver.empty();
                    } catch ([[maybe_unused]] const py::cast_error& err) {
                    }
                    try {
                        auto hip_ver =
                            torch_ver.attr("hip").cast<std::string>();
                        LOGD("torch hip version: " << hip_ver);
                        availability.torch_hip = !hip_ver.empty();
                    } catch ([[maybe_unused]] const py::cast_error& err) {
                    }
                }
            } catch (const std::exception& err) {
                LOGD("torch cuda check py error: " << err.what());
            }
        }
    }

    if ((scan_flags & settings::ScanFlagNoCt2Cuda) > 0) {
        LOGD("checking: ctranslate2-cuda (skipped)");
    } else {
        try {
            LOGD("checking: ctranslate2-cuda");
            auto ct2 = py::module_::import("ctranslate2");
            LOGD("ctranslate2 version: "
                 << ct2.attr("__version__").cast<std::string>());
            availability.ctranslate2_cuda =
                py::len(ct2.attr("get_supported_compute_types")("cuda")) > 0;
        } catch (const std::exception& err) {
            LOGD("ctranslate2-cuda check py error: " << err.what());
        }
    }

    if (scan_type == libs_scan_type_t::off_all_enabled ||
        scan_type == libs_scan_type_t::off_all_disabled) {
        return availability;
    }

    if (cpu_tools::cpuinfo().feature_flags & cpu_tools::feature_flags_t::avx) {
        try {
            LOGD("checking: coqui tts");
            py::module_::import("TTS");
            availability.coqui_tts = true;
        } catch (const std::exception& err) {
            LOGD("coqui tts check py error: " << err.what());
        }

        try {
            LOGD("checking: whisperspeech tts");
            py::module_::import("whisperspeech");
            availability.whisperspeech_tts = true;
        } catch (const std::exception& err) {
            LOGD("whisperspeech tts check py error: " << err.what());
        }

        try {
            LOGD("checking: parler tts");
            py::module_::import("parler_tts");
            LOGD("checking: transformers");
            py::module_::import("transformers");
            availability.parler_tts = true;
        } catch (const std::exception& err) {
            LOGD("parler tts check py error: " << err.what());
        }

        try {
            LOGD("checking: f5 tts");
            py::module_::import("f5_tts");
            availability.f5_tts = true;
        } catch (const std::exception& err) {
            LOGD("f5 tts check py error: " << err.what());
        }

        try {
            LOGD("checking: kokoro tts");
            py::module_::import("kokoro");
            LOGD("checking: misaki");
            py::module_::import("misaki");
            availability.kokoro_tts = true;
            try {
                LOGD("checking: misaki.ja");
                py::module_::import("misaki.ja");
                availability.kokoro_ja = true;
            } catch (const std::exception& err) {
                LOGD("kokoro kokoro.ja check py error: " << err.what());
            }
            try {
                LOGD("checking: misaki.zh");
                py::module_::import("misaki.zh");
                availability.kokoro_zh = true;
            } catch (const std::exception& err) {
                LOGD("kokoro kokoro.zh check py error: " << err.what());
            }
        } catch (const std::exception& err) {
            LOGD("kokoro tts check py error: " << err.what());
        }

        try {
            LOGD("checking: faster-whisper");
            py::module_::import("faster_whisper");
            availability.faster_whisper = true;
        } catch (const std::exception& err) {
            LOGD("faster-whisper check py error: " << err.what());
        }

        try {
            LOGD("checking: nemo-asr");
            py::module_::import("nemo.collections.asr");
            availability.nemo_asr = true;
        } catch (const std::exception& err) {
            LOGD("nemo-asr check py error: " << err.what());
        }

        try {
            LOGD("checking: transformers");
            py::module_::import("transformers");
            LOGD("checking: accelerate");
            py::module_::import("accelerate");
            availability.transformers = true;
        } catch (const std::exception& err) {
            LOGD("transformers check py error: " << err.what());
        }

        try {
            LOGD("checking: unikud");
            py::module_::import("unikud");
            availability.unikud = true;
        } catch (const std::exception& err) {
            LOGD("unikud check py error: " << err.what());
        }
    } else {
        LOGW("disabling torch dependent libraries as avx is not supported");
    }

    try {
        LOGD("checking: mimic3 tts");
        py::module_::import("mimic3_tts");
        availability.mimic3_tts = true;
    } catch (const std::exception& err) {
        LOGD("mimic3 tts check py error: " << err.what());
    }

    try {
        LOGD("checking: gruut");
        py::module_::import("gruut");

        try {
            LOGD("checking: gruut-de");
            py::module_::import("gruut_lang_de");
            availability.gruut_de = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            LOGD("checking: gruut-es");
            py::module_::import("gruut_lang_es");
            availability.gruut_es = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            LOGD("checking: gruut-fr");
            py::module_::import("gruut_lang_fr");
            availability.gruut_fr = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            LOGD("checking: gruut-it");
            py::module_::import("gruut_lang_it");
            availability.gruut_it = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            LOGD("checking: gruut-ru");
            py::module_::import("gruut_lang_ru");
            availability.gruut_ru = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            LOGD("checking: gruut-fa");
            py::module_::import("gruut_lang_fa");
            availability.gruut_fa = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            LOGD("checking: gruut-sw");
            py::module_::import("gruut_lang_sw");
            availability.gruut_sw = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }

        try {
            LOGD("checking: gruut-nl");
            py::module_::import("gruut_lang_nl");
            availability.gruut_nl = true;
        } catch (const std::exception& err) {
            LOGD("py error: " << err.what());
        }
    } catch (const std::exception& err) {
        LOGD("gruut check py error: " << err.what());
    }

    try {
        LOGD("checking: mecab");
        py::module_::import("MeCab");
        LOGD("checking: unidic-lite");
        py::module_::import("unidic_lite");
        availability.mecab = true;
    } catch (const std::exception& err) {
        LOGD("mecab check py error: " << err.what());
    }

    try {
        LOGD("checking: uroman");
        py::module_::import("uroman");
        availability.uroman = true;
    } catch (const std::exception& err) {
        LOGD("uroman check py error: " << err.what());
    }

    LOGD("py libs availability: [" << availability << "]");

    try {
        // release mem
        py::module_::import("gc").attr("collect")();
        py::module_::import("torch").attr("cuda").attr("empty_cache")();
    } catch (const std::exception& err) {
        LOGE("py error: " << err.what());
    }
#endif

    return availability;
}

bool init_module() {
#ifdef USE_PYTHON_MODULE
    if (!module_tools::init_module(QStringLiteral("python"))) return false;

    auto py_path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" +
        python_site_path;

    qDebug() << "setting env PYTHONPATH=" << py_path;

    setenv("PYTHONPATH", py_path.toStdString().c_str(), true);

    return true;
#else
    return true;
#endif
}
}  // namespace py_tools

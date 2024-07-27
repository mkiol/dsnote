/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cpu_tools.hpp"

#include <fstream>
#include <regex>
#include <string>

#include "logger.hpp"

std::ostream& operator<<(std::ostream& os, cpu_tools::arch_t arch) {
    switch (arch) {
        case cpu_tools::arch_t::x86_64:
            os << "x86_64";
            break;
        case cpu_tools::arch_t::arm32:
            os << "arm32";
            break;
        case cpu_tools::arch_t::arm64:
            os << "arm64";
            break;
        case cpu_tools::arch_t::unknown:
            os << "unknown";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, cpu_tools::cpuinfo_t cpuinfo) {
    os << "processor-count=" << cpuinfo.number_of_processors << ", flags=[";

    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::avx) os << "avx, ";
    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::avx2)
        os << "avx2, ";
    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::avx512)
        os << "avx512, ";
    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::fma) os << "fma, ";
    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::f16c)
        os << "f16c, ";
    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::asimd)
        os << "asimd, ";
    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::sse4_1)
        os << "sse4.1, ";

    os << "]";

    return os;
}

namespace cpu_tools {

arch_t arch() {
#ifdef ARCH_X86_64
    return arch_t::x86_64;
#elif ARCH_ARM_32
    return arch_t::arm32;
#elif ARCH_ARM_64
    return arch_t::arm64;
#endif
    return arch_t::unknown;
}

cpuinfo_t cpuinfo() {
    static auto cpuinfo = []() {
        std::ifstream cpuinfo_file{"/proc/cpuinfo"};
        if (!cpuinfo_file) {
            LOGE("can't open cpuinfo");
            return cpuinfo_t{};
        }

        return parse_cpuinfo(cpuinfo_file);
    }();

    return cpuinfo;
}

cpuinfo_t parse_cpuinfo(std::istream& stream) {
    cpuinfo_t cpuinfo;

    try {
        std::regex processor_rx{"processor\\s*:\\s+\\d+"};
        std::regex flags_rx{"(Features|flags)\\s*:\\s+(.*)"};

        bool flags_done = false;

        for (std::string line; std::getline(stream, line);) {
            if (std::smatch pieces_match;
                std::regex_match(line, pieces_match, processor_rx))
                ++cpuinfo.number_of_processors;

            if (flags_done) continue;

            if (std::smatch pieces_match;
                std::regex_match(line, pieces_match, flags_rx) &&
                pieces_match.size() > 2) {
                if (pieces_match[2].str().find("avx") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::avx;
                if (pieces_match[2].str().find("avx2") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::avx2;
                if (pieces_match[2].str().find("avx512") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::avx512;
                if (pieces_match[2].str().find("fma") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::fma;
                if (pieces_match[2].str().find("f16c") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::f16c;
                if (pieces_match[2].str().find("asimd") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::asimd;
                if (pieces_match[2].str().find("sse4_1") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::sse4_1;

                LOGD("cpu flags: " << pieces_match[2].str());
                flags_done = true;
            }
        }
    } catch (const std::exception& e) {
        LOGE("can't parse cpuinfo: " << e.what());
    }

    LOGD("cpuinfo: " << cpuinfo);

    return cpuinfo;
}
}  // namespace cpu_tools

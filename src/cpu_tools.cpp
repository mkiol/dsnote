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
#if defined(ARCH_ARM_64) || defined(ARCH_ARM_32)
#include <sys/auxv.h>
#endif

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

std::ostream& operator<<(std::ostream& os, cpu_tools::arm_cpu_arch_t arch) {
    switch (arch) {
        case cpu_tools::arm_cpu_arch_t::arm7:
            os << "arm7";
            break;
        case cpu_tools::arm_cpu_arch_t::arm8:
            os << "arm8";
            break;
        case cpu_tools::arm_cpu_arch_t::arm9:
            os << "arm9";
            break;
        case cpu_tools::arm_cpu_arch_t::unknown:
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
    if (cpuinfo.feature_flags & cpu_tools::feature_flags_t::bmi2)
        os << "bmi2, ";
    os << "]";
#if defined(ARCH_ARM_64) || defined(ARCH_ARM_32)
    os << ", arm-cpu-arch=" << cpuinfo.arm_cpu_arch;
#endif

    return os;
}

std::ostream& operator<<(
    std::ostream& os, [[maybe_unused]] cpu_tools::hw_cap_flags_t hw_cap_info) {
#define X(enum_id, name_str, ...) \
    if (hw_cap_info & cpu_tools::hw_cap_##enum_id) os << name_str ", ";
    CPUTOOLS_HW_CAP_TABLE
#undef X
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
        bool flags_done = false;
        bool arm_arch_done = true;
#if defined(ARCH_ARM_64) || defined(ARCH_ARM_32)
        arm_arch_done = false;
        static const std::regex arm_arch_rx{"CPU\\sarchitecture\\s*:\\s+(.*)"};
#endif
        static const std::regex processor_rx{"processor\\s*:\\s+\\d+"};
        static const std::regex flags_rx{"(Features|flags)\\s*:\\s+(.*)"};
        
        for (std::string line; std::getline(stream, line);) {
            if (std::smatch pieces_match;
                std::regex_match(line, pieces_match, processor_rx)) {
                ++cpuinfo.number_of_processors;
            }

            if (flags_done && arm_arch_done) {
                continue;
            }

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
                if (pieces_match[2].str().find("neon") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::asimd;
                if (pieces_match[2].str().find("sse4_1") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::sse4_1;
                if (pieces_match[2].str().find("bmi2") != std::string::npos)
                    cpuinfo.feature_flags |= feature_flags_t::bmi2;

                LOGD("cpu flags: " << pieces_match[2].str());
                flags_done = true;
            }
#if defined(ARCH_ARM_64) || defined(ARCH_ARM_32)
            if (arm_arch_done) {
                continue;
            }

            if (std::smatch pieces_match;
                std::regex_match(line, pieces_match, arm_arch_rx)) {
                if (pieces_match[1].str().find('7') != std::string::npos)
                    cpuinfo.arm_cpu_arch = arm_cpu_arch_t::arm7;
                else if (pieces_match[1].str().find('8') != std::string::npos)
                    cpuinfo.arm_cpu_arch = arm_cpu_arch_t::arm8;
                else if (pieces_match[1].str().find('9') != std::string::npos)
                    cpuinfo.arm_cpu_arch = arm_cpu_arch_t::arm9;

                // LOGD("arm arch: " << pieces_match[1].str());
                arm_arch_done = true;
            }
#endif
        }
    } catch (const std::exception& e) {
        LOGE("can't parse cpuinfo: " << e.what());
    }

    LOGD("cpuinfo: " << cpuinfo);

    return cpuinfo;
}

void log_hw_caps() {
    std::underlying_type_t<hw_cap_flags_t> flags = 0;
#ifdef ARCH_ARM_32
    uint32_t hwcap = getauxval(AT_HWCAP);
#endif
#ifdef ARCH_ARM_64
    uint32_t hwcap = getauxval(AT_HWCAP);
    uint32_t hwcap2 = getauxval(AT_HWCAP2);
#endif

#define X(enum_id, name_str, hw_cap_flag, hwcap_var, hwcap_macro, ...) \
    if (hwcap_var & hwcap_macro) flags |= hw_cap_flags_t::hw_cap_##enum_id;
    CPUTOOLS_HW_CAP_TABLE
#undef X

    // sfos x13: hw-caps for arm64: asimd, asimdhp, asimdrdm, asimddp, fphp, sve
    // sfos x10: hw-caps for arm32: arm-fastmult, arm-vfp, arm-neon, arm-vfpv3, arm-vfpv4
    // sfos jc1: hw-caps for arm32: arm-fastmult, arm-vfp, arm-neon, arm-vfpv3, arm-vfpv4, arm-vfpd32
    LOGD("hw-caps for " << arch() << ": "
                        << static_cast<hw_cap_flags_t>(flags));
}
}  // namespace cpu_tools

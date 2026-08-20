/* Copyright (C) 2023-2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CPU_TOOLS_CPP
#define CPU_TOOLS_CPP

#include <iostream>
#include <istream>



// enum_id, name_str, hw_cap_flag, hwcpa_var, hwcap_macro
#ifdef ARCH_ARM_32
#if !defined(HWCAP_NEON)
#define HWCAP_NEON (1 << 12)
#endif
#if !defined(HWCAP_FPHP)
#define HWCAP_FPHP (1 << 22)
#endif
#if !defined(HWCAP_ASIMDHP)
#define HWCAP_ASIMDHP (1 << 23)
#endif
#if !defined(HWCAP_ASIMDDP)
#define HWCAP_ASIMDDP (1 << 24)
#endif
#if !defined(HWCAP_ASIMDFHM)
#define HWCAP_ASIMDFHM (1 << 25)
#endif
#if !defined(HWCAP_I8MM)
#define HWCAP_I8MM (1 << 27)
#endif
#define CPUTOOLS_HW_CAP_TABLE_ARM_32                                         \
    X(arm_fastmult, "arm-fastmult", 1U << 0U, hwcap, HWCAP_ARM_FAST_MULT, 0) \
    X(arm_fpa, "arm-fpa", 1U << 1U, hwcap, HWCAP_ARM_FPA, 0)                 \
    X(arm_vfp, "arm-vfp", 1U << 2U, hwcap, HWCAP_ARM_VFP, 0)                 \
    X(arm_neon, "arm-neon", 1U << 3U, hwcap, HWCAP_ARM_NEON, 0)              \
    X(arm_vfpv3, "arm-vfpv3", 1U << 4U, hwcap, HWCAP_ARM_VFPv3, 0)           \
    X(arm_vfpv3d16, "arm-vfpv3d16", 1U << 5U, hwcap, HWCAP_ARM_VFPv3D16, 0)  \
    X(arm_vfpv4, "arm-vfpv4", 1U << 6U, hwcap, HWCAP_ARM_VFPv4, 0)           \
    X(arm_vfpd32, "arm-vfpd32", 1U << 7U, hwcap, HWCAP_ARM_VFPD32, 0)        \
    X(asimdhp, "asimdhp", 1U << 8U, hwcap, HWCAP_ASIMDHP, 0)                 \
    X(asimddp, "asimddp", 1U << 9U, hwcap, HWCAP_ASIMDDP, 0)                 \
    X(asimdfhm, "asimdfhm", 1U << 10U, hwcap, HWCAP_ASIMDFHM, 0)             \
    X(i8mm, "i8mm", 1U << 11U, hwcap, HWCAP_I8MM, 0)
#else
#define CPUTOOLS_HW_CAP_TABLE_ARM_32
#endif
#ifdef ARCH_ARM_64
#if !defined(HWCAP_ASIMD)
#define HWCAP_ASIMD (1 << 1)
#endif
#if !defined(HWCAP_ASIMDHP)
#define HWCAP_ASIMDHP (1 << 10)
#endif
#if !defined(HWCAP_ASIMDRDM)
#define HWCAP_ASIMDRDM (1 << 12)
#endif
#if !defined(HWCAP_ASIMDDP)
#define HWCAP_ASIMDDP (1 << 20)
#endif
#if !defined(HWCAP_ASIMDFHM)
#define HWCAP_ASIMDFHM (1 << 23)
#endif
#if !defined(HWCAP2_SVE2)
#define HWCAP2_SVE2 (1 << 1)
#endif
#if !defined(HWCAP2_I8MM)
#define HWCAP2_I8MM (1 << 13)
#endif
#if !defined(HWCAP2_SME)
#define HWCAP2_SME (1 << 23)
#endif
#define CPUTOOLS_HW_CAP_TABLE_ARM_64                            \
    X(asimd, "asimd", 1U << 0U, hwcap, HWCAP_ASIMD, 0)          \
    X(asimdhp, "asimdhp", 1U << 1U, hwcap, HWCAP_ASIMDHP, 0)    \
    X(asimdrdm, "asimdrdm", 1U << 2U, hwcap, HWCAP_ASIMDRDM, 0) \
    X(asimddp, "asimddp", 1U << 3U, hwcap, HWCAP_ASIMDDP, 0)    \
    X(asimdfhm, "asimdfhm", 1U << 4U, hwcap, HWCAP_ASIMDFHM, 0) \
    X(fphp, "fphp", 1U << 5U, hwcap, HWCAP_FPHP, 0)             \
    X(sve, "sve", 1U << 6U, hwcap, HWCAP_SVE, 0)                \
    X(sve2, "sve2", 1U << 7U, hwcap2, HWCAP2_SVE2, 0)           \
    X(i8mm, "i8mm", 1U << 8U, hwcap2, HWCAP2_I8MM, 0)           \
    X(sme, "sme", 1U << 9U, hwcap2, HWCAP2_SME, 0)
#else
#define CPUTOOLS_HW_CAP_TABLE_ARM_64
#endif
#define CPUTOOLS_HW_CAP_TABLE    \
    CPUTOOLS_HW_CAP_TABLE_ARM_32 \
    CPUTOOLS_HW_CAP_TABLE_ARM_64

namespace cpu_tools {
enum class arch_t { unknown, x86_64, arm32, arm64 };
enum class arm_cpu_arch_t { unknown, arm7, arm8, arm9 };

enum feature_flags_t : unsigned int {
    none = 0U,
    avx = 1U << 0U,
    avx2 = 1U << 1U,
    avx512 = 1U << 2U,
    fma = 1U << 3U,
    f16c = 1U << 4U,
    asimd = 1U << 5U,
    sse4_1 = 1U << 6U,
    bmi2 = 1U << 7U
};

enum hw_cap_flags_t : unsigned int {
    hw_cap_none = 0U,
#define X(enum_id, name_str, enum_value, ...) hw_cap_##enum_id = enum_value,
    CPUTOOLS_HW_CAP_TABLE
#undef X
};

struct cpuinfo_t {
    unsigned int number_of_processors = 0;
    unsigned int feature_flags = feature_flags_t::none;
    arm_cpu_arch_t arm_cpu_arch = arm_cpu_arch_t::unknown;

    bool operator==(const cpuinfo_t& rhs) const {
        return number_of_processors == rhs.number_of_processors &&
               feature_flags == rhs.feature_flags &&
               arm_cpu_arch == rhs.arm_cpu_arch;
    }
};

cpuinfo_t cpuinfo();
cpuinfo_t parse_cpuinfo(std::istream& stream);
arch_t arch();
void log_hw_caps();
}  // namespace cpu_tools

std::ostream& operator<<(std::ostream& os, cpu_tools::arch_t arch);
std::ostream& operator<<(std::ostream& os, cpu_tools::arm_cpu_arch_t arch);
std::ostream& operator<<(std::ostream& os, cpu_tools::cpuinfo_t cpuinfo);
std::ostream& operator<<(std::ostream& os, cpu_tools::hw_cap_flags_t hw_cap_info);

#endif // CPU_TOOLS_CPP

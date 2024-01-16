/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CPU_TOOLS_CPP
#define CPU_TOOLS_CPP

#include <iostream>
#include <istream>
#include <string>

namespace cpu_tools {
enum class arch_t { unknown, x86_64, arm32, arm64 };

enum feature_flags_t : unsigned int {
    none = 0,
    avx = 1 << 0,
    avx2 = 1 << 1,
    avx512 = 1 << 2,
    fma = 1 << 3,
    f16c = 1 << 4,
    asimd = 1 << 5
};

struct cpuinfo_t {
    unsigned int number_of_processors = 0;
    unsigned int feature_flags = feature_flags_t::none;

    inline bool operator==(const cpuinfo_t& rhs) {
        return number_of_processors == rhs.number_of_processors &&
               feature_flags == rhs.feature_flags;
    }
};

cpuinfo_t cpuinfo();
cpuinfo_t parse_cpuinfo(std::istream& stream);
arch_t arch();
}  // namespace cpu_tools

std::ostream& operator<<(std::ostream& os, cpu_tools::arch_t arch);
std::ostream& operator<<(std::ostream& os, cpu_tools::cpuinfo_t cpuinfo);

#endif // CPU_TOOLS_CPP

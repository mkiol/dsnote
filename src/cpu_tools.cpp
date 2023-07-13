/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cpu_tools.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>

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

cpu_tools::arch_t cpu_tools::arch() {
#ifdef ARCH_X86_64
    return arch_t::x86_64;
#elif ARCH_ARM_32
    return arch_t::arm32;
#elif ARCH_ARM_64
    return arch_t::arm64;
#endif
    return arch_t::unknown;
}

int cpu_tools::number_of_cores() {
    static const auto count = [] {
        std::ifstream cpuinfo{"/proc/cpuinfo"};
        auto count =
            std::count(std::istream_iterator<std::string>{cpuinfo},
                       std::istream_iterator<std::string>{}, "processor");
        if (count == 0)
            return static_cast<int>(std::thread::hardware_concurrency());
        return static_cast<int>(count);
    }();

    return count;
}

bool cpu_tools::neon_supported() {
    static const bool supported = [] {
        std::array flags = {"asimd"};  // neon-fp-armv8

        std::ifstream cpuinfo("/proc/cpuinfo");
        return std::find_first_of(std::istream_iterator<std::string>{cpuinfo},
                                  std::istream_iterator<std::string>{},
                                  flags.cbegin(), flags.cend()) !=
               std::istream_iterator<std::string>{};
    }();

    return supported;
}

bool cpu_tools::avx_avx2_fma_f16c_supported() {
    static const bool supported = [] {
        std::array flags = {"avx", "avx2", "fma", "f16c"};

        std::ifstream cpuinfo("/proc/cpuinfo");

        for (const auto& flag : flags) {
            if (std::find(std::istream_iterator<std::string>{cpuinfo},
                          std::istream_iterator<std::string>{},
                          flag) == std::istream_iterator<std::string>{}) {
                return false;
            }
        }

        return true;
    }();

    return supported;
}

bool cpu_tools::avx_supported() {
    static const bool supported = [] {
        std::array flags = {"avx"};

        std::ifstream cpuinfo("/proc/cpuinfo");
        return std::find_first_of(std::istream_iterator<std::string>{cpuinfo},
                                  std::istream_iterator<std::string>{},
                                  flags.cbegin(), flags.cend()) !=
               std::istream_iterator<std::string>{};
    }();

    return supported;
}

bool cpu_tools::avx_avx2_supported() {
    static const bool supported = [] {
        std::array flags = {"avx", "avx2"};

        std::ifstream cpuinfo("/proc/cpuinfo");
        return std::find_first_of(std::istream_iterator<std::string>{cpuinfo},
                                  std::istream_iterator<std::string>{},
                                  flags.cbegin(), flags.cend()) !=
               std::istream_iterator<std::string>{};
    }();

    return supported;
}

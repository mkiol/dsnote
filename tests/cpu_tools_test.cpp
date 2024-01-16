/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cpu_tools.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>

TEST_CASE("cpu_tools", "[parse_cpuinfo]") {
    SECTION("parse_new_cpuinfo") {
        std::string cpuinfo_data =
            "processor       : 0\n"
            "flags           : fpu vme avx avx2\n\n"
            "processor       : 1\n"
            "flags           : fpu vme avx avx2";
        std::istringstream is{cpuinfo_data};

        auto cpuinfo = cpu_tools::parse_cpuinfo(is);
        cpu_tools::cpuinfo_t expected_cpuinfo = {
            2u,
            cpu_tools::feature_flags_t::avx | cpu_tools::feature_flags_t::avx2};

        REQUIRE(cpuinfo == expected_cpuinfo);
    }

    SECTION("parse_old_cpuinfo") {
        std::string cpuinfo_data =
            "processor       : 0\n"
            "Features        : fpu vme de pse\n\n"
            "processor       : 1\n"
            "Features        : fpu vme de pse";
        std::istringstream is{cpuinfo_data};

        auto cpuinfo = cpu_tools::parse_cpuinfo(is);
        cpu_tools::cpuinfo_t expected_cpuinfo = {
            2u, cpu_tools::feature_flags_t::none};

        REQUIRE(cpuinfo == expected_cpuinfo);
    }
}

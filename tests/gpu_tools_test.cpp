/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gpu_tools.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

TEST_CASE("gpu_tools", "[rocm_overrided_gfx_version]") {
    SECTION("RDNA1") {
        auto value = gpu_tools::rocm_overrided_gfx_version("gfx90c");
        REQUIRE(value == "9.0.0");
    }

    SECTION("RDNA2") {
        auto value = gpu_tools::rocm_overrided_gfx_version("gfx1031");
        REQUIRE(value == "10.3.0");
    }

    SECTION("RDNA3") {
        auto value = gpu_tools::rocm_overrided_gfx_version("gfx1101");
        REQUIRE(value == "11.0.0");
    }
}

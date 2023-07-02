/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define private public

#include "cpu_tools.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("cpu_tools", "[avx_avx2_fma_f16c_supported]") {
    SECTION("avx_avx2_fma_f16c_supported") {
        auto supported = cpu_tools::avx_avx2_fma_f16c_supported();
        REQUIRE(supported);
    }
}

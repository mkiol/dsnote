/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define private public

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "vad.hpp"

TEST_CASE("vad", "[shift_left]") {
    std::vector<int16_t> buf{1, 2, 3, 4, 5};

    SECTION("shift 0") {
        vad::shift_left(buf, 0);

        REQUIRE(buf == std::vector<int16_t>{1, 2, 3, 4, 5});
    }

    SECTION("shift 3") {
        vad::shift_left(buf, 3);

        REQUIRE(buf == std::vector<int16_t>{4, 5});
    }

    SECTION("shift more than buf size") {
        vad::shift_left(buf, buf.size() + 1);

        REQUIRE(buf.empty());
    }
}

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define protected public

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "stt_engine.hpp"

TEST_CASE("stt_engine", "[merge_texts]") {
    std::string text1{"Hello, How are you"};
    std::string text2{"you today?"};
    std::string text3{"today?"};

    SECTION("merge not overlaped texts") {
        auto result = stt_engine::merge_texts(text1, std::move(text3));

        REQUIRE(result == "Hello, How are you today?");
    }

    SECTION("merge overlaped texts") {
        auto result = stt_engine::merge_texts(text1, std::move(text2));

        REQUIRE(result == "Hello, How are you today?");
    }

    SECTION("merge fully overlaped texts") {
        auto result = stt_engine::merge_texts(text1, "you");

        REQUIRE(result == "Hello, How are you");
    }

    SECTION("merge repeated text") {
        auto result = stt_engine::merge_texts(text1, "How are you");

        REQUIRE(result == "Hello, How are you");
    }
}

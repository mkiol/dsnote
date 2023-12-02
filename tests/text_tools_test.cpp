/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "text_tools.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("text_tools", "[restore_caps]") {
    SECTION("latin text") {
        std::string text =
            "hello. how are you today? everything is fine! we are happy.";

        text_tools::restore_caps(text);

        REQUIRE(text ==
                "Hello. How are you today? Everything is fine! We are happy.");
    }

    SECTION("double space") {
        std::string text = "hello.  how are you today?";

        text_tools::restore_caps(text);

        REQUIRE(text == "Hello.  How are you today?");
    }
}

TEST_CASE("text_tools", "[to_lower_case]") {
    SECTION("latin text") {
        std::string text = "HELLO. HOW ARE YOU TODAY?";

        text_tools::to_lower_case(text);

        REQUIRE(text == "hello. how are you today?");
    }

    //    SECTION("non-latin text") {
    //        std::string text = "ΓΕΙΑ ΣΑΣ. ΠΩΣ ΕΙΣΑΙ ΣΗΜΕΡΑ?";

    //        text_tools::to_lower_case(text);

    //        REQUIRE(text == "γειά σου. πώς είσαι σήμερα?");
    //    }
}

TEST_CASE("text_tools", "[clean_white_characters]") {
    SECTION("latin text") {
        std::string text = "Hello.   \n\n \t How\tare\t  you\rtoday?";

        text_tools::clean_white_characters(text);

        REQUIRE(text == "Hello. How are you today?");
    }
}

TEST_CASE("text_tools", "[remove_hyphen_word_break]") {
    SECTION("simple") {
        std::string text = "Hel-\nlo";

        text_tools::remove_hyphen_word_break(text);

        REQUIRE(text == "Hello");
    }

    SECTION("double line-break") {
        std::string text = "Hel-\n\nlo";

        text_tools::remove_hyphen_word_break(text);

        REQUIRE(text == "Hel-\n\nlo");
    }

    SECTION("non-alpha left") {
        std::string text = "1-\nst";

        text_tools::remove_hyphen_word_break(text);

        REQUIRE(text == "1-\nst");
    }

    SECTION("non-alpha right") {
        std::string text = "A-\n1";

        text_tools::remove_hyphen_word_break(text);

        REQUIRE(text == "A-\n1");
    }
}

TEST_CASE("text_tools", "[trim_lines]") {
    SECTION("latin text") {
        std::string text = "\nHello.  \n How are you?\t";

        text_tools::trim_lines(text);

        REQUIRE(text == "Hello.\nHow are you?");
    }
}

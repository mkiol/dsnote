/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "text_tools.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

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
}

TEST_CASE("text_tools", "[clean_white_characters]") {
    SECTION("remove duplicated") {
        std::string text = "Hello.   \t How\tare\t  you\rtoday?";

        text_tools::clean_white_characters(text);

        REQUIRE(text == "Hello. How are you today?");
    }

    SECTION("remove single newline") {
        std::string text = "Hello. \nHow are you today?";

        text_tools::clean_white_characters(text);

        REQUIRE(text == "Hello. How are you today?");
    }

    SECTION("preserve double newline") {
        std::string text = "Hello. \n\n\n How are you today?";

        text_tools::clean_white_characters(text);

        REQUIRE(text == "Hello.\n\nHow are you today?");
    }

    SECTION("mixed newline control chars") {
        std::string text = "Hello.\r\n\r\nHow are\nyou\rtoday?";

        text_tools::clean_white_characters(text);

        REQUIRE(text == "Hello.\n\nHow are you today?");
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
    SECTION("generic") {
        std::string text = "Hello.  \n How are you?\t";

        text_tools::trim_lines(text);

        INFO(text);

        REQUIRE(text == "Hello.\nHow are you?");
    }

    SECTION("preserve double line ends") {
        std::string text = "Hello. \n\n\n How are you?";

        text_tools::trim_lines(text);

        REQUIRE(text == "Hello.\n\nHow are you?");
    }

    SECTION("trim first empty lines") {
        std::string text = "\n\nHello. How are you?";

        text_tools::trim_lines(text);

        INFO(text);

        REQUIRE(text == "Hello. How are you?");
    }
}

TEST_CASE("text_tools", "[restore_punctuation_in_segments]") {
    SECTION("empty_text_with_punctuation") {
        std::string text_with_punctuation = "";
        std::vector<text_tools::segment_t> segments = {
            {1, 0, 0, "abc dfgh ijklm"}, {2, 0, 0, "abc"}, {3, 0, 0, "dfgh"}};

        std::vector<text_tools::segment_t> expected_segments = segments;

        text_tools::restore_punctuation_in_segments(text_with_punctuation,
                                                    segments);

        REQUIRE(segments == expected_segments);
    }

    SECTION("text_with_punctuation_for_one_segment") {
        std::string text_with_punctuation = "Abc dfgh, ijklm.";
        std::vector<text_tools::segment_t> segments = {
            {1, 0, 0, "abc dfgh ijklm"}, {2, 0, 0, "abc"}, {3, 0, 0, "dfgh"}};

        std::vector<text_tools::segment_t> expected_segments = {
            {1, 0, 0, "Abc dfgh, ijklm."}, {2, 0, 0, "abc"}, {3, 0, 0, "dfgh"}};

        text_tools::restore_punctuation_in_segments(text_with_punctuation,
                                                    segments);

        REQUIRE(segments == expected_segments);
    }

    SECTION("text_with_punctuation_shorter_than_segment") {
        std::string text_with_punctuation = "Abc dfgh,";
        std::vector<text_tools::segment_t> segments = {
            {1, 0, 0, "abc dfgh ijklm"}, {2, 0, 0, "abc"}, {3, 0, 0, "dfgh"}};

        std::vector<text_tools::segment_t> expected_segments = segments;

        text_tools::restore_punctuation_in_segments(text_with_punctuation,
                                                    segments);

        REQUIRE(segments == expected_segments);
    }

    SECTION("one_line_segments") {
        std::string text_with_punctuation = "Abc dfgh, ijklm. Abc dfgh.";
        std::vector<text_tools::segment_t> segments = {
            {1, 0, 0, "abc dfgh ijklm"}, {2, 0, 0, "abc"}, {3, 0, 0, "dfgh"}};

        std::vector<text_tools::segment_t> expected_segments = {
            {1, 0, 0, "Abc dfgh, ijklm."},
            {2, 0, 0, "Abc"},
            {3, 0, 0, "dfgh."}};

        text_tools::restore_punctuation_in_segments(text_with_punctuation,
                                                    segments);

        REQUIRE(segments == expected_segments);
    }
}

TEST_CASE("text_tools", "[subrip_text_to_segments]") {
    SECTION("line_breaks") {
        std::string subrip_text =
            "1\n00:00:1,100 --> 00:00:2,000\nHello,\nhow are "
            "you?\n\n2\n00:01:0,000 --> 000:02:2,000\nI'm fine.\nThank you\n";
        std::vector<text_tools::segment_t> expected_segments = {
            {1, 1100, 2000, "Hello, how are you?"},
            {2, 60000, 122000, "I'm fine. Thank you"}};

        auto segments = text_tools::subrip_text_to_segments(subrip_text);

        REQUIRE(segments == expected_segments);
    }

    SECTION("valid_srt_with_dot") {
        std::string subrip_text = "1\n00:00:1.100 --> 00:00:2.000\nHello";
        std::vector<text_tools::segment_t> expected_segments = {
            {1, 1100, 2000, "Hello"}};

        auto segments = text_tools::subrip_text_to_segments(subrip_text);

        REQUIRE(segments == expected_segments);
    }

    SECTION("long_timestamp") {
        std::string subrip_text = "1\n00:00:0,99999999 --> 00:00:1,0\nHello";
        std::vector<text_tools::segment_t> expected_segments = {
            {1, 999, 1000, "Hello"}};

        auto segments = text_tools::subrip_text_to_segments(subrip_text);

        REQUIRE(segments == expected_segments);
    }

    SECTION("invalid_timestamp") {
        std::string subrip_text =
            "1\n00:00:0 --> 00:00:1.0\nHello\n\n"
            "2\n00:00:0,1 --> 00:00:1,0\nHello again";
        std::vector<text_tools::segment_t> expected_segments = {
            {1, 1, 1000, "Hello again"}};

        auto segments = text_tools::subrip_text_to_segments(subrip_text);

        REQUIRE(segments == expected_segments);
    }
}

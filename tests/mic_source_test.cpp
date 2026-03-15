/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define private public

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QTimer>
#include <memory>

#include "../src/mic_source.h"

TEST_CASE("mic_source", "[stop]") {
    int argc = 0;
    char* argv[] = {nullptr};
    QCoreApplication app(argc, argv);

    SECTION("immediate stop after creation") {
        bool has_audio = false;
        try {
            auto mic = std::make_unique<mic_source>("");
            has_audio = mic->ok();

            if (has_audio) {
                // Stop immediately
                mic->stop();

                // Verify internal state
                REQUIRE(mic->m_stopped == true);
                REQUIRE(mic->m_eof == true);
                REQUIRE(mic->m_audio_device == nullptr);
            }
        } catch (const std::exception& e) {
            // No audio device available in test environment
            WARN("No audio device available: " << e.what());
        }
    }

    SECTION("stop prevents double-stop") {
        bool has_audio = false;
        try {
            auto mic = std::make_unique<mic_source>("");
            has_audio = mic->ok();

            if (has_audio) {
                // Call stop twice
                mic->stop();
                mic->stop();  // Should return early

                // Verify state is still correct
                REQUIRE(mic->m_stopped == true);
                REQUIRE(mic->m_eof == true);
                REQUIRE(mic->m_audio_device == nullptr);
            }
        } catch (const std::exception& e) {
            // No audio device available in test environment
            WARN("No audio device available: " << e.what());
        }
    }

    SECTION("read_audio after stop returns EOF") {
        bool has_audio = false;
        try {
            auto mic = std::make_unique<mic_source>("");
            has_audio = mic->ok();

            if (has_audio) {
                mic->stop();

                // Try to read audio
                char buf[1024];
                auto data = mic->read_audio(buf, sizeof(buf));

                // Should return EOF
                REQUIRE(data.eof == true);
                REQUIRE(mic->m_ended == true);
            }
        } catch (const std::exception& e) {
            // No audio device available in test environment
            WARN("No audio device available: " << e.what());
        }
    }

    SECTION("clear after stop doesn't crash") {
        bool has_audio = false;
        try {
            auto mic = std::make_unique<mic_source>("");
            has_audio = mic->ok();

            if (has_audio) {
                mic->stop();

                // Should not crash
                REQUIRE_NOTHROW(mic->clear());
            }
        } catch (const std::exception& e) {
            // No audio device available in test environment
            WARN("No audio device available: " << e.what());
        }
    }

    SECTION("destructor after stop doesn't crash") {
        bool has_audio = false;
        try {
            auto mic = std::make_unique<mic_source>("");
            has_audio = mic->ok();

            if (has_audio) {
                mic->stop();
                // Destructor will be called when mic goes out of scope
                REQUIRE_NOTHROW(mic.reset());
            }
        } catch (const std::exception& e) {
            // No audio device available in test environment
            WARN("No audio device available: " << e.what());
        }
    }
}

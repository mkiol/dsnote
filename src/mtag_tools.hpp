/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <optional>
#include <string>

namespace mtag_tools {
struct mtag_t {
    std::string title;
    std::string artist;
    std::string album;
    std::string comment;
    int track = 0;
};

bool write(const std::string &path, const mtag_t &mtag);
std::optional<mtag_t> read(const std::string &path);
}

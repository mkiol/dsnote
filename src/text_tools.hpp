/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef TEXT_TOOLS_H
#define TEXT_TOOLS_H

#include <string>
#include <vector>

namespace text_tools {
struct break_line_info {
    bool break_line = false;
    size_t count = 0;
};

std::pair<std::vector<std::string>, std::vector<break_line_info>> split(
    const std::string& text, const std::string& nb_data = {});
}  // namespace text_tools

#endif  // TEXT_TOOLS_H

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "text_tools.hpp"

#include <ssplit.h>

#include <string_view>

#include "logger.hpp"

namespace text_tools {
std::pair<std::vector<std::string>, std::vector<break_line_info>> split(
    const std::string& text, const std::string& nb_data) {
    std::pair<std::vector<std::string>, std::vector<break_line_info>> parts;

    ug::ssplit::SentenceSplitter ssplit{};

    if (!nb_data.empty()) ssplit.loadFromSerialized(nb_data);

    ug::ssplit::SentenceStream sentence_stream{
        text, ssplit,
        ug::ssplit::SentenceStream::splitmode::one_paragraph_per_line};

    std::string_view snt;

    size_t last_pos = 0;

    while (sentence_stream >> snt) {
        if (!snt.empty()) {
            std::string part{snt};

            auto pos = text.find(part, last_pos);

            if (pos == std::string::npos) {
                parts.second.push_back({});
                LOGW("cannot find part in orig text");
            } else {
                last_pos = pos + part.size();

                break_line_info bl_info{};
                for (; last_pos < text.size(); ++last_pos) {
                    if (text.at(last_pos) == '\n') {
                        bl_info.break_line = true;
                        bl_info.count++;
                    } else if (text.at(last_pos) != ' ') {
                        --last_pos;
                        break;
                    }
                }

                parts.second.push_back(bl_info);
            }

            parts.first.push_back(std::move(part));
        }
    }

    return parts;
}
}  // namespace text_tools

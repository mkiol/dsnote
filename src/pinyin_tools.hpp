/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PINYIN_TOOLS_H
#define PINYIN_TOOLS_H

#include <string>

namespace pinyin_tools {
bool is_hanzi(const std::string& text);
bool is_pinyin(const std::string& text);
void pinyin_to_hanzi(std::string& text, const std::string& model_path);
void hanzi_to_pinyin(std::string& text, const std::string& model_path);
std::string punc_ch_to_latin(const std::string& text);
std::string punc_latin_to_ch(const std::string& text);
}  // namespace text_tools

#endif  // PINYIN_TOOLS_H

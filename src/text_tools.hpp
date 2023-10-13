/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef TEXT_TOOLS_H
#define TEXT_TOOLS_H

#undef slots
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <optional>
#include <piper-phonemize/tashkeel.hpp>
#include <string>
#include <vector>

namespace text_tools {
enum class engine_t { ssplit, astrunc };

struct break_line_info {
    bool break_line = false;
    size_t count = 0;
};

class processor {
   public:
    processor() = default;
    ~processor();
    std::string preprocess(const std::string& text, const std::string& options,
                           const std::string& lang,
                           const std::string& lang_code,
                           const std::string& prefix_path,
                           const std::string& diacritizer_path);
    void hebrew_diacritize(std::string& text, const std::string& model_path);
    void arabic_diacritize(std::string& text, const std::string& model_path);

   private:
    std::optional<pybind11::object> m_unikud;
    std::optional<tashkeel::State> m_tashkeel_state;
};

std::pair<std::vector<std::string>, std::vector<break_line_info>> split(
    const std::string& text, engine_t engine, const std::string& lang,
    const std::string& nb_data = {});
void to_lower_case(std::string& text);
void uroman(std::string& text, const std::string& lang_code,
            const std::string& prefix_path);
void numbers_to_words(std::string& text, const std::string& lang,
                      const std::string& prefix_path);
}  // namespace text_tools

#endif  // TEXT_TOOLS_H

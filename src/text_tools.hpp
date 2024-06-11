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

#include <iostream>
#include <optional>
#include <piper-phonemize/tashkeel.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace text_tools {
enum class split_engine_t { ssplit, astrunc };
enum class text_format_t { markdown, subrip };
enum class tag_t { none, silence, speech_change };

struct segment_t {
    size_t n = 0;
    size_t t0 = 0;
    size_t t1 = 0;
    std::string text;

    bool operator==(const text_tools::segment_t& rhs) const;
    friend std::ostream& operator<<(std::ostream& os,
                                    const text_tools::segment_t& segment);
};

struct taged_segment_t {
    std::string text;
    tag_t type = tag_t::none;
    unsigned int value = 0;
};

struct break_line_info {
    bool break_line = false;
    size_t count = 0;
};

class processor {
   public:
    explicit processor(int device);
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
    int m_device = -1;  // cuda device
};

std::pair<std::vector<std::string>, std::vector<break_line_info>> split(
    const std::string& text, split_engine_t engine, const std::string& lang,
    const std::string& nb_data = {});
std::vector<taged_segment_t> split_by_tags(const std::string& text);
std::string remove_tags(const std::string& text);
void restore_caps(std::string& text);
void to_lower_case(std::string& text);
void trim_lines(std::string& text);
void trim_line(std::string& text);
void remove_hyphen_word_break(std::string& text);
void clean_white_characters(std::string& text);
bool has_uroman();
void uroman(std::string& text, const std::string& lang_code,
            const std::string& prefix_path);

void numbers_to_words(std::string& text, const std::string& lang,
                      const std::string& prefix_path);
void convert_text_format_to_html(std::string& text, text_format_t input_format);
void convert_text_format_from_html(std::string& text,
                                   text_format_t output_format);
std::string to_timestamp(size_t t);
std::optional<size_t> subrip_text_start(const std::string& text,
                                        size_t max_lines);
void segment_to_subrip_text(const segment_t& segment, std::ostringstream& os);
std::string segment_to_subrip_text(const segment_t& segment);
std::string segments_to_subrip_text(const std::vector<segment_t>& segments);
std::vector<segment_t> subrip_text_to_segments(const std::string& text,
                                               size_t offset);
void restore_punctuation_in_segment(const std::string& text_with_punctuation,
                                    segment_t& segment);
void restore_punctuation_in_segments(const std::string& text_with_punctuation,
                                     std::vector<segment_t>& segments);
void break_segment_to_multiline(unsigned int min_line_size,
                                unsigned int max_line_size, segment_t& segment);
void break_segments_to_multiline(unsigned int min_line_size,
                                 unsigned int max_line_size,
                                 std::vector<segment_t>& segments);
}  // namespace text_tools

#endif  // TEXT_TOOLS_H

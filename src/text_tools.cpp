/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "text_tools.hpp"

#include <ctype.h>
#include <fmt/format.h>
#include <html2md/html2md.h>
#include <maddy/parser.h>
#include <ssplit.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cwctype>
#include <libnumbertext/Numbertext.hxx>
#include <memory>
#include <regex>
#include <sstream>
#include <string_view>
#include <tuple>
#include <utility>

#include "astrunc/astrunc.h"
#include "logger.hpp"
#include "py_executor.hpp"

namespace text_tools {
std::ostream& operator<<(std::ostream& os,
                         const text_tools::segment_t& segment) {
    os << "n=" << segment.n << ", t0=" << segment.t0 << ", t1=" << segment.t1
       << ", text=" << segment.text;
    return os;
}

bool segment_t::operator==(const text_tools::segment_t& rhs) const {
    return n == rhs.n && t0 == rhs.t0 && t1 == rhs.t1 && text == rhs.text;
}

static astrunc::access::lang_t lang_str_to_astrunc_lang(
    const std::string& lang) {
    if (lang.size() < 2) return astrunc::access::lang_t::NONE;

    switch (lang[0]) {
        case 'a':
            if (lang[1] == 'f') return astrunc::access::lang_t::AF;
            if (lang[1] == 'r') return astrunc::access::lang_t::AR;
            if (lang[1] == 'z') return astrunc::access::lang_t::AZ;
            break;
        case 'b':
            if (lang[1] == 'e') return astrunc::access::lang_t::BE;
            if (lang[1] == 'g') return astrunc::access::lang_t::BG;
            if (lang[1] == 'o') return astrunc::access::lang_t::BO;
            if (lang[1] == 'n') return astrunc::access::lang_t::BN;
            break;
        case 'c':
            if (lang[1] == 'a') return astrunc::access::lang_t::CA;
            if (lang[1] == 's') return astrunc::access::lang_t::CS;
            break;
        case 'd':
            if (lang[1] == 'a') return astrunc::access::lang_t::DA;
            if (lang[1] == 'e') return astrunc::access::lang_t::DE;
            break;
        case 'e':
            if (lang[1] == 'e') return astrunc::access::lang_t::EE;
            if (lang[1] == 'l') return astrunc::access::lang_t::EL;
            if (lang[1] == 'n') return astrunc::access::lang_t::EN;
            if (lang[1] == 's') return astrunc::access::lang_t::ES;
            if (lang[1] == 't') return astrunc::access::lang_t::ET;
            if (lang[1] == 'u') return astrunc::access::lang_t::EU;
            break;
        case 'f':
            if (lang[1] == 'a') return astrunc::access::lang_t::FA;
            if (lang[1] == 'o') return astrunc::access::lang_t::FO;
            if (lang[1] == 'i') return astrunc::access::lang_t::FI;
            if (lang[1] == 'r') return astrunc::access::lang_t::FR;
            break;
        case 'g':
            if (lang[1] == 'l') return astrunc::access::lang_t::GL;
            if (lang[1] == 'u') return astrunc::access::lang_t::GU;
            break;
        case 'h':
            if (lang[1] == 'e') return astrunc::access::lang_t::HE;
            if (lang[1] == 'i') return astrunc::access::lang_t::HI;
            if (lang[1] == 'y') return astrunc::access::lang_t::HY;
            if (lang[1] == 'r') return astrunc::access::lang_t::HR;
            if (lang[1] == 'u') return astrunc::access::lang_t::HU;
            break;
        case 'i':
            if (lang[1] == 'd') return astrunc::access::lang_t::ID;
            if (lang[1] == 's') return astrunc::access::lang_t::IS;
            if (lang[1] == 't') return astrunc::access::lang_t::IT;
            break;
        case 'j':
            if (lang[1] == 'a') return astrunc::access::lang_t::JA;
            break;
        case 'k':
            if (lang[1] == 'a') return astrunc::access::lang_t::KA;
            if (lang[1] == 'n') return astrunc::access::lang_t::KN;
            if (lang[1] == 'k') return astrunc::access::lang_t::KK;
            if (lang[1] == 'o') return astrunc::access::lang_t::KO;
            if (lang[1] == 'y') return astrunc::access::lang_t::KY;
            break;
        case 'l':
            if (lang[1] == 'a') return astrunc::access::lang_t::LA;
            if (lang[1] == 'v') return astrunc::access::lang_t::LV;
            break;
        case 'm':
            if (lang[1] == 'k') return astrunc::access::lang_t::MK;
            if (lang[1] == 's') return astrunc::access::lang_t::MS;
            if (lang[1] == 'r') return astrunc::access::lang_t::MR;
            if (lang[1] == 'n') return astrunc::access::lang_t::MN;
            break;
        case 'n':
            if (lang[1] == 'e') return astrunc::access::lang_t::NE;
            if (lang[1] == 'l') return astrunc::access::lang_t::NL;
            if (lang[1] == 'o') return astrunc::access::lang_t::NO;
            break;
        case 'p':
            if (lang[1] == 'l') return astrunc::access::lang_t::PL;
            if (lang[1] == 't') return astrunc::access::lang_t::PT;
            if (lang[1] == 'a') return astrunc::access::lang_t::PA;
            break;
        case 'r':
            if (lang[1] == 'o') return astrunc::access::lang_t::RO;
            if (lang[1] == 'u') return astrunc::access::lang_t::RU;
            if (lang[1] == 'z') return astrunc::access::lang_t::RZ;
            break;
        case 's':
            if (lang[1] == 'a') return astrunc::access::lang_t::SA;
            if (lang[1] == 'q') return astrunc::access::lang_t::SQ;
            if (lang[1] == 'k') return astrunc::access::lang_t::SK;
            if (lang[1] == 'l') return astrunc::access::lang_t::SL;
            if (lang[1] == 'r') return astrunc::access::lang_t::SR;
            if (lang[1] == 'w') return astrunc::access::lang_t::SW;
            if (lang[1] == 'v') return astrunc::access::lang_t::SV;
            if (lang[1] == 'y') return astrunc::access::lang_t::SY;
            break;
        case 't':
            if (lang[1] == 'a') return astrunc::access::lang_t::TA;
            if (lang[1] == 'e') return astrunc::access::lang_t::TE;
            if (lang[1] == 'h') return astrunc::access::lang_t::TH;
            if (lang[1] == 'r') return astrunc::access::lang_t::TR;
            if (lang[1] == 't') return astrunc::access::lang_t::TT;
            if (lang[1] == 'w') return astrunc::access::lang_t::TW;
            break;
        case 'u':
            if (lang[1] == 'k') return astrunc::access::lang_t::UK;
            if (lang[1] == 'g') return astrunc::access::lang_t::UG;
            if (lang[1] == 'r') return astrunc::access::lang_t::UR;
            break;
        case 'v':
            if (lang[1] == 'i') return astrunc::access::lang_t::VI;
            break;
        case 'z':
            if (lang[1] == 'h') return astrunc::access::lang_t::ZH;
            break;
    }

    return astrunc::access::lang_t::NONE;
}

static std::vector<std::string> split_to_sentences(const std::string& text,
                                                   split_engine_t engine,
                                                   const std::string& lang,
                                                   const std::string& nb_data) {
    std::vector<std::string> parts;

    switch (engine) {
        case split_engine_t::ssplit: {
            ug::ssplit::SentenceSplitter ssplit{};

            if (!nb_data.empty()) ssplit.loadFromSerialized(nb_data);

            ug::ssplit::SentenceStream sentence_stream{
                text, ssplit,
                ug::ssplit::SentenceStream::splitmode::one_paragraph_per_line};

            std::string_view snt;
            while (sentence_stream >> snt) parts.emplace_back(snt);

            break;
        }
        case split_engine_t::astrunc: {
            int rc = astrunc::access::split(parts, text,
                                            lang_str_to_astrunc_lang(lang), -1);
            if (rc != 0) LOGE("astrunc split error");
            break;
        }
    }

    return parts;
}

std::pair<std::vector<std::string>, std::vector<break_line_info>> split(
    const std::string& text, split_engine_t engine, const std::string& lang,
    const std::string& nb_data) {
    std::pair<std::vector<std::string>, std::vector<break_line_info>> parts;

    parts.first = split_to_sentences(text, engine, lang, nb_data);

    size_t last_pos = 0;

    for (auto& part : parts.first) {
        if (!part.empty()) {
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
        }
    }

    return parts;
}

// source: https://stackoverflow.com/a/148766
static std::string wchar_to_UTF8(const wchar_t* in) {
    std::string out;
    unsigned int codepoint = 0;
    for (; *in != 0; ++in) {
        if (*in >= 0xd800 && *in <= 0xdbff)
            codepoint = ((*in - 0xd800) << 10) + 0x10000;
        else {
            if (*in >= 0xdc00 && *in <= 0xdfff)
                codepoint |= *in - 0xdc00;
            else
                codepoint = *in;

            if (codepoint <= 0x7f)
                out.append(1, static_cast<char>(codepoint));
            else if (codepoint <= 0x7ff) {
                out.append(1,
                           static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
                out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            } else if (codepoint <= 0xffff) {
                out.append(
                    1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
                out.append(1,
                           static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            } else {
                out.append(
                    1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
                out.append(
                    1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
                out.append(1,
                           static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            codepoint = 0;
        }
    }
    return out;
}

// source: https://stackoverflow.com/a/148766
static std::wstring UTF8_to_wchar(const char* in) {
    std::wstring out;
    unsigned int codepoint = 0;
    while (*in != 0) {
        unsigned char ch = static_cast<unsigned char>(*in);
        if (ch <= 0x7f)
            codepoint = ch;
        else if (ch <= 0xbf)
            codepoint = (codepoint << 6) | (ch & 0x3f);
        else if (ch <= 0xdf)
            codepoint = ch & 0x1f;
        else if (ch <= 0xef)
            codepoint = ch & 0x0f;
        else
            codepoint = ch & 0x07;
        ++in;
        if (((*in & 0xc0) != 0x80) && (codepoint <= 0x10ffff)) {
            if (sizeof(wchar_t) > 2)
                out.append(1, static_cast<wchar_t>(codepoint));
            else if (codepoint > 0xffff) {
                codepoint -= 0x10000;
                out.append(1, static_cast<wchar_t>(0xd800 + (codepoint >> 10)));
                out.append(1,
                           static_cast<wchar_t>(0xdc00 + (codepoint & 0x03ff)));
            } else if (codepoint < 0xd800 || codepoint >= 0xe000)
                out.append(1, static_cast<wchar_t>(codepoint));
        }
    }
    return out;
}

static void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}

static void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

void restore_caps(std::string& text) {
    auto wtext = UTF8_to_wchar(text.c_str());

    auto cap_first_letter = false;

    std::transform(
        wtext.cbegin(), wtext.cend(), wtext.begin(),
        [eos = false, &cap_first_letter](auto wc) mutable -> decltype(wc) {
            if (eos && wc != ' ' && wc != '.' && wc != '!' && wc != '?') {
                eos = false;
                return std::towupper(wc);
            }
            if (!eos && (wc == '.' || wc == '!' || wc == '?')) {
                eos = true;
                cap_first_letter = true;
            }

            return wc;
        });

    if (cap_first_letter) wtext.front() = std::towupper(wtext.front());

    text.assign(wchar_to_UTF8(wtext.c_str()));
}

void to_lower_case(std::string& text) {
    auto wtext = UTF8_to_wchar(text.c_str());
    std::transform(wtext.cbegin(), wtext.cend(), wtext.begin(), std::towlower);
    text.assign(wchar_to_UTF8(wtext.c_str()));
}

void remove_hyphen_word_break(std::string& text) {
    auto wtext_in = UTF8_to_wchar(text.c_str());
    std::wstring wtext_out;

    auto is_hyphen = [](wchar_t wc) {
        wchar_t hypens[] =
            L"\U0000002D\U00002010\U00002011\U00002012\U00002013\U00002014";
        for (auto i = 0u; i < 6; ++i)
            if (wc == hypens[i]) return true;
        return false;
    };

    for (auto it = wtext_in.cbegin(); it != wtext_in.cend();
         std::advance(it, 1)) {
        if (it != wtext_in.cbegin() && std::next(it) != wtext_in.cend() &&
            std::next(it, 2) != wtext_in.cend() && is_hyphen(*it) &&
            *std::next(it) == '\n' && std::iswalpha(*std::prev(it)) &&
            std::iswalpha(*std::next(it, 2))) {
            std::advance(it, 1);
        } else {
            wtext_out.push_back(*it);
        }
    }

    text.assign(wchar_to_UTF8(wtext_out.c_str()));
}

void clean_white_characters(std::string& text) {
    text =
        std::regex_replace(text, std::regex{"\n\n+|\r\r+|\r\n(\r\n)+"}, "_n_");
    text = std::regex_replace(text, std::regex{"\\s+"}, " ");
    text = std::regex_replace(text, std::regex{"_n_"}, "\n\n");
    text = std::regex_replace(text, std::regex{" \n|\n "}, "\n");
}

void trim_lines(std::string& text) {
    auto ss_in = std::stringstream{text};
    std::stringstream ss_out;

    bool first_line = true;
    bool prev_line_empty = false;
    for (std::string line; std::getline(ss_in, line);) {
        ltrim(line);
        rtrim(line);

        if (line.empty()) {
            prev_line_empty = !first_line;
        } else {
            if (prev_line_empty) {
                ss_out << '\n';
                prev_line_empty = false;
            }
            ss_out << line << '\n';

            first_line = false;
        }
    }

    auto s = ss_out.str();
    if (!s.empty() && s.back() == '\n') s = s.substr(0, s.size() - 1);

    text.assign(std::move(s));
}

// source: https://stackoverflow.com/a/64359731/7767358
static std::pair<FILE*, FILE*> popen2(const char* __command) {
    int pipes[2][2];

    pipe(pipes[0]);
    pipe(pipes[1]);

    if (fork() > 0) {
        // parent
        close(pipes[0][0]);
        close(pipes[1][1]);

        return {fdopen(pipes[0][1], "w"), fdopen(pipes[1][0], "r")};
    } else {
        // child
        close(pipes[0][1]);
        close(pipes[1][0]);

        dup2(pipes[0][0], STDIN_FILENO);
        dup2(pipes[1][1], STDOUT_FILENO);

        execl("/bin/sh", "/bin/sh", "-c", __command, NULL);

        exit(1);
    }
}

bool has_uroman() { return std::system("perl --version > /dev/null") == 0; }

void uroman(std::string& text, const std::string& lang_code,
            const std::string& prefix_path) {
    auto [p_stdin, p_stdout] =
        popen2(fmt::format("perl {}/uroman/bin/uroman.pl -l {}", prefix_path,
                           lang_code)
                   .c_str());

    std::string result;

    if (p_stdin == nullptr || p_stdout == nullptr) {
        LOGE("uroman popen error");
    }

    fwrite(text.c_str(), 1, text.size(), p_stdin);
    fclose(p_stdin);

    char buf[1024];
    while (fgets(buf, 1024, p_stdout)) result.append(buf);

    if (result.empty()) {
        LOGW("uroman result is empty");
    } else {
        text.assign(result);
    }
}

void numbers_to_words(std::string& text, const std::string& lang,
                      const std::string& prefix_path) {
    Numbertext nt{};
    nt.set_prefix(prefix_path + "/libnumbertext/");

    auto to_words = [&](std::string&& word) {
        auto trailer_idx = word.find_last_not_of(".,");
        if (trailer_idx == std::string::npos) return std::move(word);

        auto trailer =
            trailer_idx < word.size() - 1 ? word.substr(trailer_idx + 1) : "";
        if (!trailer.empty()) word.resize(trailer_idx + 1);

        if (nt.numbertext(word, lang)) {
            word.append(trailer + ' ');
        } else {
            LOGW("can't convert number to words: " << word);
            word.append(trailer);
        }
        return std::move(word);
    };

    std::size_t start = 0, end = 0;
    const char digits[] = "0123456789.,";
    while (true) {
        start = text.find_first_of(digits, end);

        if (start == std::string::npos) break;

        end = text.find_first_not_of(digits, start);

        if (end == std::string::npos) {
            auto words = to_words(text.substr(start));
            text.replace(start, text.size() - start, words);
            break;
        } else {
            auto words = to_words(text.substr(start, end - start));
            text.replace(start, end - start, words);
            end += words.size() - (end - start);
        }
    }
}

static void replace_characters(std::string& text, const std::string& from,
                               const std::string& to) {
    auto w_text = UTF8_to_wchar(text.c_str());
    auto w_from = UTF8_to_wchar(from.c_str());
    auto w_to = UTF8_to_wchar(to.c_str());

    if (w_from.size() != w_to.size()) {
        LOGE("cannot replace characters, from and to sizes are not the same");
        return;
    }

    std::transform(w_text.begin(), w_text.end(), w_text.begin(),
                   [&](const auto c) {
                       auto pos = w_from.find(c);
                       if (pos == std::string::npos) return c;
                       return w_to[pos];
                   });

    text.assign(wchar_to_UTF8(w_text.c_str()));
}

static void add_extra_pause(std::string& text) {
    if (text.empty()) return;

    if (text.back() == '.' || text.back() == '!' || text.back() == '?')
        text.append(" ,.");
    else
        text.append(". ,.");
}

static void convert_subrip_to_html(std::string& text) {
    std::stringstream in_ss{text};
    std::stringstream out_ss;

    static const std::regex html_tags{"<[^>]*>"};

    unsigned int n = 0;
    std::string text_line;
    for (std::string line; std::getline(in_ss, line);) {
        ltrim(line);
        rtrim(line);

        if (line.empty()) {
            if (!text_line.empty()) {
                out_ss << "<p>" << text_line << "</p>";
                text_line.clear();
            }

            out_ss << "<p></p>";

            n = 0;

            continue;
        }

        if (n == 0) {
            if (!std::all_of(line.cbegin(), line.cend(), [](auto c) {
                    return std::isdigit(static_cast<unsigned char>(c)) != 0;
                })) {
                continue;
            }
        }

        if (n < 2) {
            out_ss << "<code>" << line << "</code>";
            ++n;
            continue;
        }

        if (!text_line.empty()) text_line.append("<span></span>");

        line = std::regex_replace(line, html_tags, "");
        text_line.append(line);

        ++n;
    }

    if (!text_line.empty()) {
        out_ss << "<p>" << text_line << "</p><p></p>";
    }

    text.assign(out_ss.str());
}

static void convert_html_to_subrip(std::string& text) {
    text = std::regex_replace(text, std::regex{"<p>|<code>|<span>"}, "");
    text = std::regex_replace(text, std::regex{"</p>|</code>|</span>"}, "\n");
}

static void convert_markdown_to_html(std::string& text) {
    std::stringstream ss{text};

    auto config = std::make_shared<maddy::ParserConfig>();
    config->enabledParsers = maddy::types::ALL;

    text.assign(maddy::Parser{config}.Parse(ss));
}

static void convert_html_to_markdown(std::string& text) {
    bool ok = false;

    text.assign(html2md::Convert(text, &ok));

    if (!ok) LOGW("error in html to markdown conversion");
}

void convert_text_format_to_html(std::string& text,
                                 text_format_t input_format) {
    switch (input_format) {
        case text_format_t::markdown:
            convert_markdown_to_html(text);
            break;
        case text_format_t::subrip: {
            convert_subrip_to_html(text);
            break;
        }
    }
}

void convert_text_format_from_html(std::string& text,
                                   text_format_t output_format) {
    switch (output_format) {
        case text_format_t::markdown:
            convert_html_to_markdown(text);
            break;
        case text_format_t::subrip:
            convert_html_to_subrip(text);
            break;
    }
}

// copied from wisper.cpp
std::string to_timestamp(size_t msec) {
    size_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    size_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    size_t sec = msec / 1000;
    msec = msec - sec * 1000;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", static_cast<int>(hr),
             static_cast<int>(min), static_cast<int>(sec), ",",
             static_cast<int>(msec));

    return std::string{buf};
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static const std::regex subrip_time_rx{
    "(\\d+):(\\d+):(\\d+)[,.](\\d{1,3})\\d*\\s+-->\\s+(\\d+):(\\d+):(\\d+)"
    "\\d*[,.](\\d{1,3})\\s*"};

std::optional<size_t> subrip_text_start(const std::string& text,
                                        size_t max_lines) {
    enum class state_t { unknown, num_line };

    state_t state = state_t::unknown;
    size_t subrip_start = 0;
    std::istringstream is{text};

    size_t n = 0;
    for (std::string line; n < max_lines && std::getline(is, line); ++n) {
        switch (state) {
            case state_t::unknown:
                if (is && !line.empty() &&
                    std::all_of(line.cbegin(), line.cend(), is_digit)) {
                    state = state_t::num_line;
                    subrip_start = is.tellg();
                    subrip_start -= line.size() + 1;
                }
                break;
            case state_t::num_line:
                if (std::regex_match(line.cbegin(), line.cend(),
                                     subrip_time_rx))
                    return subrip_start;
                if (is && std::all_of(line.cbegin(), line.cend(), is_digit)) {
                    state = state_t::num_line;
                    subrip_start = is.tellg();
                    subrip_start -= line.size() + 1;
                } else
                    state = state_t::unknown;
                break;
        }
    }

    return std::nullopt;
}

void segment_to_subrip_text(const segment_t& segment, std::ostringstream& os) {
    os << segment.n << '\n'
       << to_timestamp(segment.t0) << " --> " << to_timestamp(segment.t1)
       << '\n'
       << segment.text << "\n\n";
}

std::string segment_to_subrip_text(const segment_t& segment) {
    std::ostringstream os;
    segment_to_subrip_text(segment, os);
    return os.str();
}

std::string segments_to_subrip_text(const std::vector<segment_t>& segments) {
    std::ostringstream os;

    std::for_each(
        segments.cbegin(), segments.cend(),
        [&os](const auto& segment) { segment_to_subrip_text(segment, os); });

    return os.str();
}

static std::optional<std::pair<size_t, size_t>> parse_subrip_time_line(
    const std::string& text) {
    static const std::regex time_rx{
        "(\\d+):(\\d+):(\\d+)[,.](\\d{1,3})\\d*\\s+-->\\s+(\\d+):(\\d+):(\\d+)"
        "\\d*[,.](\\d{1,3})\\s*"};

    std::pair<size_t, size_t> time{0ll, 0ll};

    std::smatch pieces_match;

    try {
        if (std::regex_match(text, pieces_match, time_rx)) {
            for (std::size_t i = 1; i < pieces_match.size(); ++i) {
                size_t t = std::clamp(std::stoll(pieces_match[i].str()), 0ll,
                                      i == 4 || i == 8 ? 999ll : 60ll);
                if (i == 1)
                    time.first += t * 60 * 60 * 1000;
                else if (i == 2)
                    time.first += t * 60 * 1000;
                else if (i == 3)
                    time.first += t * 1000;
                else if (i == 4)
                    time.first += t;
                else if (i == 5)
                    time.second += t * 60 * 60 * 1000;
                else if (i == 6)
                    time.second += t * 60 * 1000;
                else if (i == 7)
                    time.second += t * 1000;
                else if (i == 8)
                    time.second += t;
            }

            return time;
        }
    } catch (const std::exception& e) {
        LOGE(e.what());
    }

    LOGE("can't parse subrip line");

    return std::nullopt;
}

std::vector<segment_t> subrip_text_to_segments(const std::string& text,
                                               size_t offset) {
    std::vector<segment_t> segments;

    static const std::regex html_tags_rx{"<[^>]*>"};

    std::istringstream in_ss{text};
    in_ss.seekg(offset, std::ios::beg);

    unsigned int n = 0;
    std::string text_line;
    std::pair<size_t, size_t> t;
    for (std::string line; std::getline(in_ss, line);) {
        ltrim(line);
        rtrim(line);

        if (line.empty()) {
            if (!text_line.empty()) {
                segments.push_back({segments.size() + 1, t.first, t.second,
                                    std::move(text_line)});
                text_line.clear();
            }

            n = 0;

            continue;
        }

        if (n == 0) {
            if (std::all_of(line.cbegin(), line.cend(), [](auto c) {
                    return std::isdigit(static_cast<unsigned char>(c)) != 0;
                })) {
                ++n;
            }
            continue;
        }

        if (n < 2) {
            if (auto time = parse_subrip_time_line(line)) {
                t = time.value();
                ++n;
            } else {
                n = 0;
            }

            continue;
        }

        line = std::regex_replace(line, html_tags_rx, "");

        if (!text_line.empty()) text_line.push_back(' ');
        text_line.append(line);

        ++n;
    }

    if (!text_line.empty()) {
        segments.push_back(
            {segments.size() + 1, t.first, t.second, std::move(text_line)});
    }

    return segments;
}

static bool restore_punctuation_in_segment_internal(
    const std::string& text_with_punctuation, const std::string& lower_text,
    std::string::const_iterator& head, segment_t& segment) {
    auto h = head;
    auto s_h = segment.text.begin();

    while (h != lower_text.cend() && s_h != segment.text.end()) {
        auto [it1, it2] = std::mismatch(s_h, segment.text.end(), h);

        if (it2 == lower_text.cend()) break;

        h = std::next(it2);

        if (std::string{".,?!:-"}.find(*it2) == std::string::npos) break;

        s_h = it1 == segment.text.end() ? it1 : std::next(it1);

        if (h != lower_text.cend()) h = std::next(h);
    }

    if (std::max<size_t>(0, std::distance(head, h)) < segment.text.size())
        return false;

    auto beg = std::next(text_with_punctuation.cbegin(),
                         std::distance(lower_text.cbegin(), head));
    auto end = std::next(text_with_punctuation.cbegin(),
                         std::distance(lower_text.cbegin(), h));
    segment.text.assign(beg, end);
    rtrim(segment.text);

    head = h;

    return true;
}

void restore_punctuation_in_segment(const std::string& text_with_punctuation,
                                    segment_t& segment) {
    auto lower_text = text_with_punctuation;
    to_lower_case(lower_text);

    auto head = lower_text.cbegin();

    restore_punctuation_in_segment_internal(text_with_punctuation, lower_text,
                                            head, segment);
}

void restore_punctuation_in_segments(const std::string& text_with_punctuation,
                                     std::vector<segment_t>& segments) {
    auto lower_text = text_with_punctuation;
    to_lower_case(lower_text);

    auto head = lower_text.cbegin();

    for (auto& segment : segments) {
        if (!restore_punctuation_in_segment_internal(text_with_punctuation,
                                                     lower_text, head, segment))
            break;
    }
}

void break_segment_to_multiline(unsigned int min_line_size,
                                unsigned int max_line_size,
                                segment_t& segment) {
    if (min_line_size == 0 || max_line_size == 0) return;

    static const std::array punct_chars{'.', ',', '?', '!', ':', '-'};

    auto head = segment.text.cbegin();
    auto h = head;

    std::string new_segment_text;

    auto trim_iterator = [&](std::string::const_iterator beg,
                             std::string::const_iterator it) {
        auto dist = std::distance(beg, it);

        if (dist <= 0) return it;
        if (it != segment.text.cend() && *it == ' ') return it;

        std::advance(beg, std::min<std::ptrdiff_t>(min_line_size, dist));

        auto init_it = it;
        for (it = std::prev(it); it != beg; --it)
            if (*it == ' ') break;

        if (it == beg) return init_it;

        return it;
    };

    while (true) {
        auto it = std::find_first_of(h, segment.text.cend(),
                                     punct_chars.cbegin(), punct_chars.cend());
        if (it != segment.text.cend()) ++it;

        auto line_size = std::distance(head, it);

        if (line_size == 0) break;

        if (it == segment.text.cend() && line_size < min_line_size) {
            if (!new_segment_text.empty()) new_segment_text.push_back('\n');
            new_segment_text.append(head, it);

            break;
        }

        if (it == segment.text.cend() || line_size > max_line_size) {
            it = trim_iterator(
                head,
                std::next(head,
                          std::min(static_cast<std::ptrdiff_t>(max_line_size),
                                   line_size)));

            if (!new_segment_text.empty()) new_segment_text.push_back('\n');
            new_segment_text.append(head, it);

            if (it != segment.text.cend() && *it == ' ') ++it;

            head = it;
            h = head;

            continue;
        }

        if (line_size >= min_line_size) {
            if (!new_segment_text.empty()) new_segment_text.push_back('\n');
            new_segment_text.append(head, it);

            if (it != segment.text.cend() && *it == ' ') ++it;

            head = it;
            h = head;

            continue;
        }

        h = it;
    }

    segment.text.assign(std::move(new_segment_text));
}

void break_segments_to_multiline(unsigned int min_line_size,
                                 unsigned int max_line_size,
                                 std::vector<segment_t>& segments) {
    if (min_line_size == 0 || max_line_size == 0) return;

    for (auto& segment : segments)
        break_segment_to_multiline(min_line_size, max_line_size, segment);
}

void processor::hebrew_diacritize(std::string& text,
                                  const std::string& model_path) {
    using namespace pybind11::literals;

    auto task = py_executor::instance()->execute(
        [&, dev = m_device < 0 ? "cpu"
                               : fmt::format("{}:{}", "cuda", m_device)]() {
            try {
                if (!m_unikud) {
                    LOGD("creating hebrew diacritizer: device=" << dev);

                    auto framework = py::module_::import("unikud.framework");
                    m_unikud = framework.attr("Unikud")(
                        "hub_name"_a = model_path, "device"_a = dev);
                }

                return m_unikud.value()(text).cast<std::string>();
            } catch (const std::exception& err) {
                LOGE("py error: " << err.what());
            }

            return text;
        });

    if (task) text.assign(std::any_cast<std::string>(task->get()));
}

void processor::arabic_diacritize(std::string& text,
                                  const std::string& model_path) {
    if (!m_tashkeel_state) {
        m_tashkeel_state.emplace();
        tashkeel::tashkeel_load(model_path, *m_tashkeel_state);
    }

    text.assign(tashkeel::tashkeel_run(text, *m_tashkeel_state));
}

static bool has_option(char c, const std::string& options) {
    return options.find(c) != std::string::npos;
}

std::string processor::preprocess(const std::string& text,
                                  const std::string& options,
                                  const std::string& lang,
                                  const std::string& lang_code,
                                  const std::string& prefix_path,
                                  const std::string& diacritizer_path) {
    std::string new_text{text};

    if (has_option('n', options)) {
        LOGD("numbers-to-words pre-processing needed");
        numbers_to_words(new_text, lang, prefix_path);
    }

    if (has_option('r', options)) {
        LOGD("uroman pre-processing needed");
        uroman(new_text, lang_code, prefix_path);
    }

    if (has_option('l', options)) {
        LOGD("to-lower pre-processing needed");
        to_lower_case(new_text);
    }

    if (has_option('c', options)) {
        LOGD("char replace pre-processing needed");
        replace_characters(new_text, "“”‘’", "\"\"''");
    }

    if (has_option('d', options) && !diacritizer_path.empty()) {
        LOGD("diacritize pre-processing needed");
        if (lang == "ar")
            arabic_diacritize(new_text, diacritizer_path);
        else if (lang == "he")
            hebrew_diacritize(new_text, diacritizer_path);
    }

    if (has_option('p', options)) {
        LOGD("extra pause needed");
        add_extra_pause(new_text);
    }
#ifdef DEBUG
    LOGD("text after pre-processing: " << new_text);
#endif

    return new_text;
}

processor::processor(int device) : m_device{device} {}

processor::~processor() {
    auto task = py_executor::instance()->execute([&]() {
        try {
            m_unikud.reset();
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();
}

}  // namespace text_tools

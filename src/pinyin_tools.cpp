/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "pinyin_tools.hpp"

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Pinyin.h>
#include <fmt/format.h>
#include <pinyin.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <initializer_list>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cpp-pinyin/ChineseG2p.h"
#include "logger.hpp"
#include "text_tools.hpp"

namespace {
const std::unordered_set<std::string> punctuation_marks = {
    // ---------- latin ----------
    "!",
    "\"",
    "#",
    "$",
    "%",
    "&",
    "'",
    "(",
    ")",
    "*",
    "+",
    ",",
    "-",
    ".",
    "/",
    ":",
    ";",
    "<",
    "=",
    ">",
    "?",
    "@",
    "[",
    "\\",
    "]",
    "^",
    "_",
    "`",
    "{",
    "|",
    "}",
    "~",
    "“",
    "”",
    "‘",
    "’",
    "«",
    "»",

    // ---------- chinese ----------
    "！",
    "＂",
    "＃",
    "＄",
    "％",
    "＆",
    "＇",
    "（",
    "）",
    "＊",
    "＋",
    "，",
    "－",
    "／",
    "：",
    "；",
    "＜",
    "＝",
    "＞",
    "？",
    "＠",
    "［",
    "＼",
    "］",
    "＾",
    "＿",
    "｀",
    "｛",
    "｜",
    "｝",
    "～",
    "。",
    "、",
    "〈",
    "〉",
    "《",
    "》",
    "「",
    "」",
    "『",
    "』",
    "【",
    "】",
    "〔",
    "〕",
    "〖",
    "〗",
    "·",
    "—",
    "…",
};

const std::unordered_set<char> white_chars = {
    ' ', '\t', '\n', '\r', '\f', '\v',
};

const std::unordered_map<std::string_view, std::string_view>
    punc_cn_to_latin_table = {
        {"，", ", "},  {"。", ". "},  {"！", "! "},  {"？", "? "},
        {"；", "; "},  {"：", ":"},   {"（", " ("},  {"）", ") "},
        {"｛", " {"},  {"｝", "} "},  {"［", "["},   {"］", "]"},
        {"〈", "<"},   {"〉", ">"},   {"《", " <"},  {"》", "> "},
        {"【", " ["},  {"】", "] "},  {"「", " \""}, {"」", "\" "},
        {"『", " \""}, {"』", "\" "}, {"“", "\""},   {"”", "\""},
        {"‘", "'"},    {"’", "'"},    {"、", ","},   {"…", "..."},
        {"—", "--"},   {"～", "~"},   {"·", "·"},    {"＠", "@"},
        {"＃", "#"},   {"＄", "$"},   {"％", "%"},   {"＆", "&"},
        {"＊", "*"},   {"＋", "+"},   {"－", "-"},   {"／", "/"},
        {"＼", "\\"},  {"｜", "|"},   {"＾", "^"},   {"＿", "_"},
        {"＜", "<"},   {"＞", ">"},   {"＝", "="},   {"｀", "`"},
    };

const std::unordered_map<std::string_view, std::string_view>
    punc_latin_to_cn_table = {
        {",", "，"}, {".", "。"}, {"!", "！"},  {"?", "？"}, {";", "；"},
        {":", "："}, {"(", "（"}, {")", "）"},  {"{", "｛"}, {"}", "｝"},
        {"[", "［"}, {"]", "］"}, {"~", "～"},  {"@", "＠"}, {"#", "＃"},
        {"$", "＄"}, {"%", "％"}, {"&", "＆"},  {"*", "＊"}, {"+", "＋"},
        {"-", "－"}, {"/", "／"}, {"\\", "＼"}, {"|", "｜"}, {"^", "＾"},
        {"_", "＿"}, {"=", "＝"}, {"`", "｀"},  {"·", "·"},
    };

const std::unordered_map<std::string, std::string> low_to_up_table = {
    // ASCII letters
    {"a", "A"},
    {"b", "B"},
    {"c", "C"},
    {"d", "D"},
    {"e", "E"},
    {"f", "F"},
    {"g", "G"},
    {"h", "H"},
    {"i", "I"},
    {"j", "J"},
    {"k", "K"},
    {"l", "L"},
    {"m", "M"},
    {"n", "N"},
    {"o", "O"},
    {"p", "P"},
    {"q", "Q"},
    {"r", "R"},
    {"s", "S"},
    {"t", "T"},
    {"u", "U"},
    {"w", "W"},
    {"x", "X"},
    {"y", "Y"},
    {"z", "Z"},
    {"v", "V"},
    // a-series
    {"ā", "Ā"},
    {"á", "Á"},
    {"ǎ", "Ǎ"},
    {"à", "À"},
    // e-series
    {"ē", "Ē"},
    {"é", "É"},
    {"ě", "Ě"},
    {"è", "È"},
    // i-series
    {"ī", "Ī"},
    {"í", "Í"},
    {"ǐ", "Ǐ"},
    {"ì", "Ì"},
    // o-series
    {"ō", "Ō"},
    {"ó", "Ó"},
    {"ǒ", "Ǒ"},
    {"ò", "Ò"},
    // u-series
    {"ū", "Ū"},
    {"ú", "Ú"},
    {"ǔ", "Ǔ"},
    {"ù", "Ù"},
    // ü-series
    {"ü", "Ü"},
    {"ǖ", "Ǖ"},
    {"ǘ", "Ǘ"},
    {"ǚ", "Ǚ"},
    {"ǜ", "Ǜ"},
};

const std::unordered_map<std::string, char> tone_to_null_map = {
    {"ā", 'a'}, {"á", 'a'}, {"ǎ", 'a'}, {"à", 'a'}, {"Ā", 'a'}, {"Á", 'a'},
    {"Ǎ", 'a'}, {"À", 'a'}, {"ō", 'o'}, {"ó", 'o'}, {"ǒ", 'o'}, {"ò", 'o'},
    {"Ō", 'o'}, {"Ó", 'o'}, {"Ǒ", 'o'}, {"Ò", 'o'}, {"ē", 'e'}, {"é", 'e'},
    {"ě", 'e'}, {"è", 'e'}, {"Ē", 'e'}, {"É", 'e'}, {"Ě", 'e'}, {"È", 'e'},
    {"ī", 'i'}, {"í", 'i'}, {"ǐ", 'i'}, {"ì", 'i'}, {"Ī", 'i'}, {"Í", 'i'},
    {"Ǐ", 'i'}, {"Ì", 'i'}, {"ū", 'u'}, {"ú", 'u'}, {"ǔ", 'u'}, {"ù", 'u'},
    {"Ū", 'u'}, {"Ú", 'u'}, {"Ǔ", 'u'}, {"Ù", 'u'}, {"ü", 'v'}, {"ǘ", 'v'},
    {"ǚ", 'v'}, {"ǜ", 'v'}, {"ǖ", 'v'}, {"Ü", 'v'}, {"Ǘ", 'v'}, {"Ǚ", 'v'},
    {"Ǜ", 'v'}, {"Ǖ", 'v'}, {"ê", 'e'}, {"Ê", 'e'},
};

std::string to_up(const std::string& utf8_char) {
    auto it = low_to_up_table.find(utf8_char);
    if (it != low_to_up_table.cend()) {
        return it->second;
    }
    return utf8_char;
}

unsigned int utf8_to_codepoint(const std::string& utf8_char) {
    if (utf8_char.empty()) {
        return 0;
    }

    unsigned char c0 = utf8_char[0];

    if (utf8_char.size() == 1) {
        return c0;
    } else if (utf8_char.size() == 2) {
        return ((c0 & 0x1F) << 6) |
               (static_cast<unsigned char>(utf8_char[1]) & 0x3F);
    } else if (utf8_char.size() == 3) {
        return ((c0 & 0x0F) << 12) |
               ((static_cast<unsigned char>(utf8_char[1]) & 0x3F) << 6) |
               (static_cast<unsigned char>(utf8_char[2]) & 0x3F);
    } else if (utf8_char.size() == 4) {
        return ((c0 & 0x07) << 18) |
               ((static_cast<unsigned char>(utf8_char[1]) & 0x3F) << 12) |
               ((static_cast<unsigned char>(utf8_char[2]) & 0x3F) << 6) |
               (static_cast<unsigned char>(utf8_char[3]) & 0x3F);
    }

    return 0;
}

std::string next_utf8_char(const std::string& input, size_t& i) {
    if (i >= input.size()) return "";
    unsigned char c = input[i];
    if ((c & 0x80) == 0) {
        i++;
        return input.substr(i - 1, 1);
    } else if ((c & 0xE0) == 0xC0) {
        if (i + 1 >= input.size()) return "";
        i += 2;
        return input.substr(i - 2, 2);
    } else if ((c & 0xF0) == 0xE0) {
        if (i + 2 >= input.size()) return "";
        i += 3;
        return input.substr(i - 3, 3);
    } else if ((c & 0xF8) == 0xF0) {
        if (i + 3 >= input.size()) return "";
        i += 4;
        return input.substr(i - 4, 4);
    }
    return "";
}

bool is_white(const std::string& utf8_char) {
    return utf8_char.size() == 1 && white_chars.contains(utf8_char[0]);
}

bool is_punctuation_mark(const std::string& utf8_char) {
    return punctuation_marks.contains(utf8_char);
}

bool is_ascii_letter(const std::string& utf8_char) {
    return utf8_char.size() == 1 &&
           ((utf8_char[0] >= 'a' && utf8_char[0] <= 'z') ||
            (utf8_char[0] >= 'A' && utf8_char[0] <= 'Z'));
}

bool is_ascii(const std::string& utf8_char) {
    return utf8_char.size() == 1 &&
           ((utf8_char[0] >= 'a' && utf8_char[0] <= 'z') ||
            (utf8_char[0] >= 'A' && utf8_char[0] <= 'Z') ||
            (utf8_char[0] >= '0' && utf8_char[0] <= '9'));
}

bool is_ton_number(const std::string& utf8_char) {
    return utf8_char.size() == 1 &&
           (utf8_char[0] >= '0' && utf8_char[0] <= '5');
}

std::string normalize_pinyin_token(const std::string& token) {
    std::string result;
    size_t i = 0;
    while (i < token.size()) {
        std::string utf8_char = next_utf8_char(token, i);
        if (utf8_char.empty()) {
            break;
        }

        if (is_ascii_letter(utf8_char)) {
            result += std::tolower(utf8_char[0]);
        }

        if (utf8_char.size() != 1) {
            auto it = tone_to_null_map.find(utf8_char);
            if (it != tone_to_null_map.end()) {
                result += it->second;
            }
        }
    }
    return result;
}

bool is_valid_pinyin_syllable(const std::string& syl) {
    if (syl.empty() || syl.length() > 6) {
        return false;
    }

    static const std::unordered_set<std::string> common_pinyin = {
        "a",     "ai",     "an",     "ang",   "ao",    "ba",     "bai",
        "ban",   "bang",   "bao",    "bei",   "ben",   "beng",   "bi",
        "bian",  "biao",   "bie",    "bin",   "bing",  "bo",     "bu",
        "ca",    "cai",    "can",    "cang",  "cao",   "ce",     "cen",
        "ceng",  "cha",    "chai",   "chan",  "chang", "chao",   "che",
        "chen",  "cheng",  "chi",    "chong", "chou",  "chu",    "chuai",
        "chuan", "chuang", "chui",   "chun",  "chuo",  "ci",     "cong",
        "cou",   "cu",     "cuan",   "cui",   "cun",   "cuo",    "da",
        "dai",   "dan",    "dang",   "dao",   "de",    "deng",   "di",
        "dian",  "diao",   "die",    "ding",  "diu",   "dong",   "dou",
        "du",    "duan",   "dui",    "dun",   "duo",   "e",      "ei",
        "en",    "eng",    "er",     "fa",    "fan",   "fang",   "fei",
        "fen",   "feng",   "fo",     "fou",   "fu",    "ga",     "gai",
        "gan",   "gang",   "gao",    "ge",    "gei",   "gen",    "geng",
        "gong",  "gou",    "gu",     "gua",   "guai",  "guan",   "guang",
        "gui",   "gun",    "guo",    "ha",    "hai",   "han",    "hang",
        "hao",   "he",     "hei",    "hen",   "heng",  "hong",   "hou",
        "hu",    "hua",    "huai",   "huan",  "huang", "hui",    "hun",
        "huo",   "ji",     "jia",    "jian",  "jiang", "jiao",   "jie",
        "jin",   "jing",   "jiong",  "jiu",   "ju",    "juan",   "jue",
        "jun",   "ka",     "kai",    "kan",   "kang",  "kao",    "ke",
        "ken",   "keng",   "kong",   "kou",   "ku",    "kua",    "kuai",
        "kuan",  "kuang",  "kui",    "kun",   "kuo",   "la",     "lai",
        "lan",   "lang",   "lao",    "le",    "lei",   "leng",   "li",
        "lia",   "lian",   "liang",  "liao",  "lie",   "lin",    "ling",
        "liu",   "long",   "lou",    "lu",    "lv",    "luan",   "lue",
        "lun",   "luo",    "ma",     "mai",   "man",   "mang",   "mao",
        "me",    "mei",    "men",    "meng",  "mi",    "mian",   "miao",
        "mie",   "min",    "ming",   "miu",   "mo",    "mou",    "mu",
        "na",    "nai",    "nan",    "nang",  "nao",   "ne",     "nei",
        "nen",   "neng",   "ni",     "nian",  "niang", "niao",   "nie",
        "nin",   "ning",   "niu",    "nong",  "nu",    "nv",     "nuan",
        "nue",   "nuo",    "o",      "ou",    "pa",    "pai",    "pan",
        "pang",  "pao",    "pei",    "pen",   "peng",  "pi",     "pian",
        "piao",  "pie",    "pin",    "ping",  "po",    "pou",    "pu",
        "qi",    "qia",    "qian",   "qiang", "qiao",  "qie",    "qin",
        "qing",  "qiong",  "qiu",    "qu",    "quan",  "que",    "qun",
        "ran",   "rang",   "rao",    "re",    "ren",   "reng",   "ri",
        "rong",  "rou",    "ru",     "ruan",  "rui",   "run",    "ruo",
        "sa",    "sai",    "san",    "sang",  "sao",   "se",     "sen",
        "seng",  "sha",    "shai",   "shan",  "shang", "shao",   "she",
        "shei",  "shen",   "sheng",  "shi",   "shou",  "shu",    "shua",
        "shuai", "shuan",  "shuang", "shui",  "shun",  "shuo",   "si",
        "song",  "sou",    "su",     "suan",  "sui",   "sun",    "suo",
        "ta",    "tai",    "tan",    "tang",  "tao",   "te",     "teng",
        "ti",    "tian",   "tiao",   "tie",   "ting",  "tong",   "tou",
        "tu",    "tuan",   "tui",    "tun",   "tuo",   "wa",     "wai",
        "wan",   "wang",   "wei",    "wen",   "weng",  "wo",     "wu",
        "xi",    "xia",    "xian",   "xiang", "xiao",  "xie",    "xin",
        "xing",  "xiong",  "xiu",    "xu",    "xuan",  "xue",    "xun",
        "ya",    "yan",    "yang",   "yao",   "ye",    "yi",     "yin",
        "ying",  "yo",     "yong",   "you",   "yu",    "yuan",   "yue",
        "yun",   "za",     "zai",    "zan",   "zang",  "zao",    "ze",
        "zei",   "zen",    "zeng",   "zha",   "zhai",  "zhan",   "zhang",
        "zhao",  "zhe",    "zhei",   "zhen",  "zheng", "zhi",    "zhong",
        "zhou",  "zhu",    "zhua",   "zhuai", "zhuan", "zhuang", "zhui",
        "zhun",  "zhuo",   "zi",     "zong",  "zou",   "zu",     "zuan",
        "zui",   "zun",    "zuo",
    };

    if (common_pinyin.contains(syl)) {
        return true;
    }

    static const std::vector<std::string> illegal_clusters = {
        "bl", "cl", "fl", "gl", "pl",  "sl",  "br",  "cr",  "dr",
        "fr", "gr", "pr", "tr", "str", "spl", "spr", "squ", "sw",
        "tw", "dw", "by", "py", "my",  "dy",  "ty",
    };

    for (const auto& cluster : illegal_clusters) {
        if (syl.starts_with(cluster)) {
            return false;
        }
    }

    return false;
}

std::vector<std::pair<std::string_view, std::string_view>> split_pinyin(
    const std::string& text) {
    std::vector<std::pair<std::string_view, std::string_view>> result;
    if (text.empty()) {
        return result;
    };

    struct TokenInfo {
        std::string_view view;
        size_t start;
        size_t end;
        bool is_pinyin;
    };

    std::vector<TokenInfo> tokens;
    size_t idx = 0;
    size_t n = text.size();

    static const std::unordered_set<std::string> no_split_white_chars = {
        " ",
        "\r",
    };
    static const std::unordered_set<std::string> split_white_chars = {
        "\t",
        "\n",
        "\f",
        "\v",
    };

    while (idx < n) {
        std::string utf8_char;
        size_t start = 0, end = 0;

        // skip spaces
        while (idx < n) {
            utf8_char = next_utf8_char(text, idx);
            if (utf8_char.empty()) {
                break;
            }
            if (!no_split_white_chars.contains(utf8_char)) {
                idx -= utf8_char.size();
                break;
            }
        }

        if (idx >= n) {
            break;
        }

        // skip non-puntuation
        start = idx;
        bool has_black = false;
        while (idx < n) {
            utf8_char = next_utf8_char(text, idx);
            if (utf8_char.empty()) {
                break;
            }

            bool is_black = !is_white(utf8_char);
            bool is_regular = !split_white_chars.contains(utf8_char) &&
                              !is_punctuation_mark(utf8_char);

            if (!is_regular) {
                if (has_black) {
                    idx -= utf8_char.size();
                }
                break;
            }

            if (!is_black) {
                idx -= utf8_char.size();
                break;
            }

            has_black = true;
        }
        end = idx;

        if (end > start) {
            std::string_view sv(text.data() + start, end - start);
            auto norm = normalize_pinyin_token(std::string(sv));
            bool valid = !norm.empty() && is_valid_pinyin_syllable(norm);

            // LOGD("nsegment: >" << sv << "< norm=>" << norm
            //                    << "< valid=" << valid);

            tokens.push_back(
                {.view = sv, .start = start, .end = end, .is_pinyin = valid});
        }
    }

    // for (const auto& t : tokens) {
    //     LOGD("pinyin token: >" << t.view << "< start=" << t.start << ", end="
    //                            << t.end << ", is_pinyin=" << t.is_pinyin);
    // }

    if (tokens.empty()) {
        /* no tokens, text is just full of white characters */
        return result;
    }

    size_t t = 0;
    while (t < tokens.size()) {
        if (!tokens[t].is_pinyin) {
            size_t delim_start_pos = tokens[t].start;
            size_t delim_end_pos = tokens[t].end;
            ++t;
            while (t < tokens.size() && !tokens[t].is_pinyin) {
                delim_end_pos = tokens[t].end;
                ++t;
            }
            std::string_view delimiter(text.data() + delim_start_pos,
                                       delim_end_pos - delim_start_pos);
            result.emplace_back(std::string_view(), delimiter);
            continue;
        }

        size_t seg_start_pos = tokens[t].start;
        size_t seg_end_pos = tokens[t].end;
        ++t;
        while (t < tokens.size() && tokens[t].is_pinyin) {
            seg_end_pos = tokens[t].end;
            ++t;
        }
        std::string_view segment(text.data() + seg_start_pos,
                                 seg_end_pos - seg_start_pos);

        std::string_view delimiter;
        if (t < tokens.size() && !tokens[t].is_pinyin) {
            size_t delim_start_pos = tokens[t].start;
            size_t delim_end_pos = tokens[t].end;
            ++t;
            while (t < tokens.size() && !tokens[t].is_pinyin) {
                delim_end_pos = tokens[t].end;
                ++t;
            }
            delimiter = std::string_view(text.data() + delim_start_pos,
                                         delim_end_pos - delim_start_pos);
        }

        result.emplace_back(segment, delimiter);
    }

    // for (const auto& t : result) {
    //     LOGD("pinyin result: pinyin=>" << t.first << "< delim=>" << t.second
    //                                    << "<");
    // }

    return result;
}

enum class tone_conversion_t { none, remove, to_digit };

std::string convert_pinyin_tones(const std::string& text,
                                 tone_conversion_t conversion_type,
                                 pinyin_instance_t* inst, bool remove_punct,
                                 bool remove_white, bool remove_non_ascii);

std::vector<std::string_view> extract_syllables(pinyin_instance_t* inst,
                                                const std::string& text) {
    std::vector<std::string_view> syllables;

    /* pinyin_parse_more_full_pinyins doesn't support tones, so removing it */
    auto ctext =
        convert_pinyin_tones(text, tone_conversion_t::remove, nullptr,
                             /*remove_punct=*/true, /*remove_white=*/true,
                             /*remove_non_ascii*/ true);

    size_t nkeys = pinyin_parse_more_full_pinyins(inst, ctext.c_str());
    for (size_t i = 0; i < nkeys;) {
        ChewingKey* key = nullptr;
        ChewingKeyRest* rest = nullptr;
        if (!pinyin_get_pinyin_key(inst, i, &key)) {
            LOGW("pinyin_get_pinyin_key error: offset=" << i << "/" << nkeys
                                                        << ", text=" << ctext);
            break;
        }
        if (!pinyin_get_pinyin_key_rest(inst, i, &rest)) {
            LOGW("pinyin_get_pinyin_key_rest error: offset="
                 << i << "/" << nkeys << ", text=" << ctext);
            break;
        }

        unsigned short begin = 0, end = 0;
        if (!pinyin_get_pinyin_key_rest_positions(inst, rest, &begin, &end)) {
            LOGW("pinyin_get_pinyin_key_rest_positions error: range="
                 << begin << "-" << end << ", text=" << ctext);
            break;
        }
        i = end;

        syllables.emplace_back(ctext.data() + begin, ctext.data() + end);
    }

    pinyin_reset(inst);

    return syllables;
}

std::string convert_pinyin_tones(const std::string& text,
                                 tone_conversion_t conversion_type,
                                 pinyin_instance_t* inst, bool remove_punct,
                                 bool remove_white, bool remove_non_ascii) {
    static const std::unordered_map<std::string, std::pair<std::string, char>>
        tone_to_digit_map = {
            {"ā", {"a", '1'}}, {"á", {"a", '2'}}, {"ǎ", {"a", '3'}},
            {"à", {"a", '4'}}, {"ē", {"e", '1'}}, {"é", {"e", '2'}},
            {"ě", {"e", '3'}}, {"è", {"e", '4'}}, {"ī", {"i", '1'}},
            {"í", {"i", '2'}}, {"ǐ", {"i", '3'}}, {"ì", {"i", '4'}},
            {"ō", {"o", '1'}}, {"ó", {"o", '2'}}, {"ǒ", {"o", '3'}},
            {"ò", {"o", '4'}}, {"ū", {"u", '1'}}, {"ú", {"u", '2'}},
            {"ǔ", {"u", '3'}}, {"ù", {"u", '4'}}, {"ǖ", {"ü", '1'}},
            {"ǘ", {"v", '2'}}, {"ǚ", {"v", '3'}}, {"ǜ", {"v", '4'}},
            {"Ā", {"a", '1'}}, {"Á", {"a", '2'}}, {"Ǎ", {"a", '3'}},
            {"À", {"a", '4'}}, {"Ē", {"e", '1'}}, {"É", {"e", '2'}},
            {"Ě", {"e", '3'}}, {"È", {"e", '4'}}, {"Ī", {"i", '1'}},
            {"Í", {"i", '2'}}, {"Ǐ", {"i", '3'}}, {"Ì", {"i", '4'}},
            {"Ō", {"o", '1'}}, {"Ó", {"o", '2'}}, {"Ǒ", {"o", '3'}},
            {"Ò", {"o", '4'}}, {"Ū", {"u", '1'}}, {"Ú", {"u", '2'}},
            {"Ǔ", {"u", '3'}}, {"Ù", {"u", '4'}}, {"Ǖ", {"v", '1'}},
            {"Ǘ", {"v", '2'}}, {"Ǚ", {"v", '3'}}, {"Ǜ", {"v", '4'}},
        };

    auto to_lower = [](const std::string& utf8_char) {
        return static_cast<char>(
            std::tolower(static_cast<unsigned char>(utf8_char[0])));
    };

    std::string result;

    switch (conversion_type) {
        case tone_conversion_t::none:
        case tone_conversion_t::remove: {
            for (size_t i = 0; i < text.size();) {
                auto utf8_char = next_utf8_char(text, i);
                if (utf8_char.empty()) {
                    break;
                }

                if (conversion_type != tone_conversion_t::none) {
                    auto it = tone_to_null_map.find(utf8_char);
                    if (it != tone_to_null_map.end()) {
                        // replace with tone-less character
                        result += it->second;
                        continue;
                    }
                }

                if (remove_punct && is_punctuation_mark(utf8_char)) {
                    // ignore punctuation
                    continue;
                }

                if (remove_white && is_white(utf8_char)) {
                    // ignore white character
                    continue;
                }

                if (is_ton_number(utf8_char)) {
                    // ignore tone number
                    continue;
                }

                if (remove_non_ascii && !is_ascii(utf8_char)) {
                    // ignore non-ascii
                    continue;
                }

                result += to_lower(utf8_char);
            }
            break;
        }
        case tone_conversion_t::to_digit: {
            if (inst == nullptr) {
                LOGF("pinyin instance is null");
            }

            // array of syllables without tones
            auto syllables = extract_syllables(inst, text);
            if (syllables.empty()) {
                return text;
            }

            std::string current_syllable;
            size_t syllable_count = 0;
            char found_tone = '0';

            auto same_syllable_as_current = [&] {
                if (syllables.size() <= syllable_count) {
                    return false;
                }
                return convert_pinyin_tones(
                           current_syllable, tone_conversion_t::remove, nullptr,
                           /*remove_punct=*/false, /*remove_white=*/false,
                           /*remove_non_ascii*/ false) ==
                       syllables.at(syllable_count);
            };

            auto flush_syllable = [&]() {
                if (!current_syllable.empty()) {
                    result.append(current_syllable);
                    if (found_tone != '0') {
                        result += found_tone;
                    }
                    current_syllable.clear();
                    found_tone = '0';
                    ++syllable_count;
                }
            };

            for (size_t i = 0; i < text.size();) {
                auto utf8_char = next_utf8_char(text, i);
                if (utf8_char.empty()) {
                    break;
                }

                if (is_ascii_letter(utf8_char)) {
                    current_syllable += to_lower(utf8_char);
                    continue;
                }

                if (auto it = tone_to_digit_map.find(utf8_char);
                    it != tone_to_digit_map.end()) {
                    current_syllable.append(it->second.first);
                    found_tone = it->second.second;
                    if (same_syllable_as_current()) {
                        flush_syllable();
                    }
                    continue;
                }

                if (auto it = tone_to_null_map.find(utf8_char);
                    it != tone_to_null_map.end()) {
                    current_syllable += it->second;
                    if (same_syllable_as_current()) {
                        flush_syllable();
                    }
                    continue;
                }

                flush_syllable();

                if (remove_punct && is_punctuation_mark(utf8_char)) {
                    // ignore punctuation
                    continue;
                }

                if (remove_white && is_white(utf8_char)) {
                    // ignore white character
                    continue;
                }

                if (remove_non_ascii && !is_ascii(utf8_char)) {
                    // ignore non ascii character
                    continue;
                }

                result.append(utf8_char);
            }

            flush_syllable();
            break;
        }
    }

    // LOGD("convert: >" << text << "< => >" << result << "<");

    return result;
}

std::string punc_convert(
    const std::string& text,
    const std::unordered_map<std::string_view, std::string_view>& table) {
    std::string result;

    // LOGD("punc in:  >" << text << "<");

    std::string white_prefix;
    bool last_punct = false;
    bool first = true;
    for (size_t i = 0; i < text.size();) {
        auto utf8_char = next_utf8_char(text, i);
        if (utf8_char.empty()) {
            break;
        }

        if (is_white(utf8_char)) {
            if (!last_punct ||
                (utf8_char.size() == 1 && utf8_char[0] == '\n')) {
                white_prefix.append(utf8_char);
            }
            continue;
        }

        auto it = table.find(utf8_char);
        if (it != punc_cn_to_latin_table.cend()) {
            result.append(it->second);
            white_prefix.clear();
            last_punct = true;
            if (it->second.size() == 1 &&
                (it->second[0] == '.' || it->second[0] == '!' ||
                 it->second[0] == '?' || it->second[0] == '\n')) {
                first = true;
            }
            continue;
        }

        result.append(white_prefix);
        if (first) {
            result.append(to_up(utf8_char));
        } else {
            result.append(utf8_char);
        }
        white_prefix.clear();
        last_punct = false;
        first = false;
    }

    result.append(white_prefix);

    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    // LOGD("punc out: >" << result << "<");

    return result;
}
}  // namespace

namespace pinyin_tools {
std::string punc_ch_to_latin(const std::string& text) {
    return punc_convert(text, punc_cn_to_latin_table);
}

std::string punc_latin_to_ch(const std::string& text) {
    return punc_convert(text, punc_latin_to_cn_table);
}

void hanzi_to_pinyin(std::string& text, const std::string& model_path) {
    if (model_path.empty()) {
        LOGD("hanzi-to-pinyin model path is empty");
        return;
    }

    if (text.empty()) {
        return;
    }

    if (is_pinyin(text)) {
        LOGD("skip hanzi-to-pinyin because text is already pinyin");
        return;
    }
    if (!is_hanzi(text)) {
        LOGD("skip hanzi-to-pinyin because text isn't hanzi");
        return;
    }

    text.assign(punc_latin_to_ch(text));

    Pinyin::setDictionaryPath(model_path);

    static const auto g2p_man = std::make_unique<Pinyin::Pinyin>();
    auto vec = g2p_man->hanziToPinyin(text, Pinyin::ManTone::Style::TONE,
                                      Pinyin::Error::Default,
                                      /*candidates=*/false, /*v_to_u=*/false,
                                      /*neutral_tone_with_five=*/false);
    // for (const auto& v : vec) {
    //     LOGD("v: >" << v.pinyin << "<" << " >" << v.hanzi << "<" << " error="
    //     << v.error);
    // }

    bool first = true;
    text.assign(punc_ch_to_latin([&] {
        std::string result;
        bool last_ok = false;
        for (const auto& res : vec) {
            if (!first && (last_ok || !res.error)) {
                result += ' ';
            }
            last_ok = !res.error;
            result += last_ok ? res.pinyin : res.hanzi;
            first = false;
        }
        return result;
    }()));
}

void pinyin_to_hanzi(std::string& text, const std::string& model_path) {
    if (model_path.empty()) {
        LOGD("pinyin-to-hanzi model path is empty");
        return;
    }

    if (text.empty()) {
        return;
    }

    if (is_hanzi(text)) {
        LOGD("skip pinyin-to-hanzi because text is hanzi");
        return;
    }
    if (!is_pinyin(text)) {
        LOGD("skip pinyin-to-hanzi because text isn't pinyin");
        return;
    }

    // libpinyin context init is expensive => do it once
    static pinyin_context_t* ctx = nullptr;
    if (!ctx) {
        ctx = pinyin_init(model_path.c_str(), nullptr);
        if (!ctx) {
            LOGE("pinyin_init failed");
            return;
        }
        pinyin_set_options(ctx, USE_TONE);
    }

    auto* inst = pinyin_alloc_instance(ctx);

    std::string result;
    auto pieces = split_pinyin(text);

    if (pieces.empty()) {
        return;
    }

    for (auto& [text, delim] : pieces) {
        auto ctext = convert_pinyin_tones(
            std::string{text}, tone_conversion_t::to_digit, inst,
            /*remove_punct=*/true, /*remove_white=*/true,
            /*remove_non_asci*/ true);
        bool ok = false;
        int parsed = pinyin_parse_more_full_pinyins(inst, ctext.c_str());
        if (parsed >= 0 && pinyin_guess_sentence(inst)) {
            char* utf8_str = nullptr;
            if (pinyin_get_sentence(inst, 0, &utf8_str) && utf8_str) {
                // text_tools::trim_line(delim);
                result.append(utf8_str);
                result.append(delim);

                // LOGD("pinyin to hanzi: >" << text << "< => >" << ctext
                //                           << "< => >" << utf8_str << "<");

                free(utf8_str);  // should be g_free()
                ok = true;
            }
        }
        if (!ok) {
            if (!text.empty()) {
                result.append(text);
            }
            result.append(delim);
        }
        pinyin_reset(inst);
    }

    pinyin_free_instance(inst);

    text.assign(punc_latin_to_ch(result));
}

bool is_pinyin(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    std::stringstream ss(text);
    std::string token;
    int total_tokens = 0;
    int valid_pinyin_tokens = 0;

    while (ss >> token) {
        auto normalized = normalize_pinyin_token(token);
        if (!normalized.empty()) {
            ++total_tokens;
            if (is_valid_pinyin_syllable(normalized)) {
                ++valid_pinyin_tokens;
            }
        }
    }

    if (total_tokens == 0) {
        return false;
    }

    // decision rule: 60% or more recognized pinyin words
    double ratio = static_cast<double>(valid_pinyin_tokens) / total_tokens;
    return ratio >= 0.60;
}

bool is_hanzi(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    size_t chinese_count = 0;
    size_t non_chinese_count = 0;

    for (size_t i = 0; i < text.size();) {
        auto utf8_char = next_utf8_char(text, i);
        if (utf8_char.empty()) {
            break;
        }

        auto cp = utf8_to_codepoint(utf8_char);

        if (is_white(utf8_char)) {
            // ignore white character
            continue;
        }

        bool is_ideograph = (cp >= 0x4E00 && cp <= 0x9FFF) ||
                            (cp >= 0x3400 && cp <= 0x4DBF) ||
                            (cp >= 0x20000 && cp <= 0x2A6DF);
        bool is_punctuation =
            (cp >= 0x3001 && cp <= 0x303F) || (cp >= 0xFF00 && cp <= 0xFFEF);
        if (is_ideograph || is_punctuation) {
            ++chinese_count;
        } else {
            ++non_chinese_count;
        }
    }

    size_t total = chinese_count + non_chinese_count;

    // decision rule: 75% or more recognized chinese characters
    double ratio = static_cast<double>(chinese_count) / total;
    return total == 0 ? false : ratio >= 0.75;
}

}  // namespace pinyin_tools

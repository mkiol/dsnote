/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "text_tools.hpp"

#include <ssplit.h>

#include <string_view>

#include "astrunc/astrunc.h"
#include "logger.hpp"

namespace text_tools {
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
                                                   engine_t engine,
                                                   const std::string& lang,
                                                   const std::string& nb_data) {
    std::vector<std::string> parts;

    switch (engine) {
        case engine_t::ssplit: {
            ug::ssplit::SentenceSplitter ssplit{};

            if (!nb_data.empty()) ssplit.loadFromSerialized(nb_data);

            ug::ssplit::SentenceStream sentence_stream{
                text, ssplit,
                ug::ssplit::SentenceStream::splitmode::one_paragraph_per_line};

            std::string_view snt;
            while (sentence_stream >> snt) parts.emplace_back(snt);

            break;
        }
        case engine_t::astrunc: {
            int rc = astrunc::access::split(parts, text,
                                            lang_str_to_astrunc_lang(lang), -1);
            if (rc != 0) LOGE("astrunc split error");
            break;
        }
    }

    return parts;
}

std::pair<std::vector<std::string>, std::vector<break_line_info>> split(
    const std::string& text, engine_t engine, const std::string& lang,
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
}  // namespace text_tools

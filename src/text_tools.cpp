/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "text_tools.hpp"

#include <fmt/format.h>
#include <ssplit.h>
#include <unistd.h>

#include <algorithm>
#include <cwctype>
#include <libnumbertext/Numbertext.hxx>
#include <string_view>

#include "astrunc/astrunc.h"
#include "logger.hpp"
#include "py_executor.hpp"

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
    unsigned int codepoint;
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

void to_lower_case(std::string& text) {
    auto wtext = UTF8_to_wchar(text.c_str());
    std::transform(wtext.cbegin(), wtext.cend(), wtext.begin(), std::towlower);
    text.assign(wchar_to_UTF8(wtext.c_str()));
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
        if (trailer_idx == std::string::npos) return word;

        auto trailer =
            trailer_idx < word.size() - 1 ? word.substr(trailer_idx + 1) : "";
        if (!trailer.empty()) word.resize(trailer_idx + 1);

        if (nt.numbertext(word, lang)) {
            word.append(trailer + ' ');
        } else {
            LOGW("can't convert number to words: " << word);
            word.append(trailer);
        }
        return word;
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

void processor::hebrew_diacritize(std::string& text,
                                  const std::string& model_path) {
    using namespace pybind11::literals;

    auto* pe = py_executor::instance();

    try {
        text = pe->execute([&, dev = pe->libs_availability->torch_cuda
                                         ? m_device
                                         : -1]() {
                     try {
                         if (!m_unikud) {
                             LOGD("creating hebrew diacritizer");

                             auto framework =
                                 py::module_::import("unikud.framework");
                             m_unikud = framework.attr("Unikud")(
                                 "hub_name"_a = model_path, "device"_a = dev);
                         }

                         return m_unikud.value()(text).cast<std::string>();
                     } catch (const std::exception& err) {
                         LOGE("py error: " << err.what());
                     }

                     return text;
                 }).get();
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
    }
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
#ifdef DEBUG
    LOGD("text after pre-processing: " << new_text);
#endif

    return new_text;
}

processor::processor(int device) : m_device{device} {}

processor::~processor() {
    auto* pe = py_executor::instance();

    try {
        pe->execute([&]() {
              try {
                  m_unikud.reset();
              } catch (const std::exception& err) {
                  LOGE("py error: " << err.what());
              }
              return std::string{};
          }).get();
    } catch (const std::exception& err) {
        LOGE("error: " << err.what());
    }
}

}  // namespace text_tools

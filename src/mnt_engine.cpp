/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mnt_engine.hpp"

#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "text_tools.hpp"

std::ostream& operator<<(std::ostream& os,
                         const mnt_engine::model_files_t& model_files) {
    os << "model-path-first=" << model_files.model_path_first
       << ", model-path-second=" << model_files.model_path_second;

    return os;
}

std::ostream& operator<<(std::ostream& os, const mnt_engine::config_t& config) {
    os << "lang=" << config.lang << ", clean-text=" << config.clean_text
       << ", model-files=[" << config.model_files << "]";

    return os;
}

std::ostream& operator<<(std::ostream& os, mnt_engine::state_t state) {
    switch (state) {
        case mnt_engine::state_t::idle:
            os << "idle";
            break;
        case mnt_engine::state_t::initializing:
            os << "initializing";
            break;
        case mnt_engine::state_t::translating:
            os << "translating";
            break;
        case mnt_engine::state_t::error:
            os << "error";
            break;
    }

    return os;
}

mnt_engine::mnt_engine(config_t config, callbacks_t call_backs)
    : m_config{std::move(config)}, m_call_backs{std::move(call_backs)} {
    if (!m_call_backs.text_translated)
        throw std::runtime_error("translated callback not provided");
    if (m_config.model_files.model_path_first.empty())
        throw std::runtime_error("model-path-first is empty");

    open_bergamot_lib();
}

mnt_engine::~mnt_engine() {
    LOGD("mnt dtor");

    stop();

    if (m_bergamot_api_api.ok()) {
        if (m_bergamot_ctx_first) {
            m_bergamot_api_api.bergamot_api_delete(m_bergamot_ctx_first);
            m_bergamot_ctx_first = nullptr;
        }
        if (m_bergamot_ctx_second) {
            m_bergamot_api_api.bergamot_api_delete(m_bergamot_ctx_second);
            m_bergamot_ctx_second = nullptr;
        }
    }

    m_bergamot_api_api = {};

    if (m_bergamotlib_handle) {
        dlclose(m_bergamotlib_handle);
        m_bergamotlib_handle = nullptr;
    }
}

void mnt_engine::open_bergamot_lib() {
#ifdef ARCH_X86_64
    if (cpu_tools::avx_avx2_supported()) {
        m_bergamotlib_handle = dlopen("libbergamot_api.so", RTLD_LAZY);
    } else if (cpu_tools::avx_supported()) {
        LOGW("using bergamot-fallback");
        m_bergamotlib_handle = dlopen("libbergamot_api-fallback.so", RTLD_LAZY);
    } else {
        LOGE("avx not supported by bergamot needs it");
        throw std::runtime_error(
            "failed to open bergamot lib: avx not supported");
    }
#else
    m_bergamotlib_handle = dlopen("libbergamot_api.so", RTLD_LAZY);
#endif

    if (m_bergamotlib_handle == nullptr) {
        LOGE("failed to open bergamot lib: " << dlerror());
        throw std::runtime_error("failed to open bergamot lib");
    }

    m_bergamot_api_api.bergamot_api_make =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_make)>(
            dlsym(m_bergamotlib_handle, "bergamot_api_make"));
    m_bergamot_api_api.bergamot_api_delete =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_delete)>(
            dlsym(m_bergamotlib_handle, "bergamot_api_delete"));
    m_bergamot_api_api.bergamot_api_translate =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_translate)>(
            dlsym(m_bergamotlib_handle, "bergamot_api_translate"));
    m_bergamot_api_api.bergamot_api_cancel =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_cancel)>(
            dlsym(m_bergamotlib_handle, "bergamot_api_cancel"));

    if (!m_bergamot_api_api.ok()) {
        LOGE("failed to register bergamon api");
        throw std::runtime_error("failed to register bergamot api");
    }
}

void mnt_engine::start() {
    LOGD("mnt start");

    m_shutting_down = true;
    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    m_queue = std::queue<task_t>{};
    m_state = state_t::idle;
    m_shutting_down = false;
    m_processing_thread = std::thread{&mnt_engine::process, this};

    LOGD("mnt start completed");
}

void mnt_engine::stop() {
    LOGD("mnt stop started");

    m_shutting_down = true;

    set_state(state_t::idle);

    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    LOGD("mnt stop completed");
}

void mnt_engine::request_stop() {
    LOGD("mnt stop requested");

    m_shutting_down = true;

    set_state(state_t::idle);
}

std::string mnt_engine::find_file_with_name_prefix(std::string dir_path,
                                                   std::string prefix) {
    auto* dirp = opendir(dir_path.c_str());
    if (!dirp) return {};

    while (auto* dirent = readdir(dirp)) {
        if (dirent->d_type != DT_REG) continue;

        std::string fn{dirent->d_name};

        if (fn.size() < prefix.size()) continue;

        if (fn.substr(0, prefix.size()) == prefix)
            return dir_path.append("/").append(fn);
    }

    return {};
}

void mnt_engine::translate(std::string text) {
    if (m_shutting_down) return;

    {
        std::lock_guard lock{m_mutex};
        m_queue.push({std::move(text)});
    }

    LOGD("task pushed");

    m_cv.notify_one();
}

void mnt_engine::set_state(state_t new_state) {
    if (m_shutting_down) new_state = state_t::idle;

    if (m_state != new_state) {
        LOGD("mnt engine state: " << m_state << " => " << new_state);
        m_state = new_state;
        m_call_backs.state_changed(m_state);
    }
}

static std::string fix_text(std::string text) {
    if (text.size() > 1 && text[0] == '-' && text[1] == ' ')
        return text.substr(2);
    return text;
}

std::string mnt_engine::translate_internal(std::string text) {
    std::vector<std::string> out_parts;

    auto engine = m_config.options.find('a') != std::string::npos
                      ? text_tools::engine_t::astrunc
                      : text_tools::engine_t::ssplit;

    if (m_config.clean_text) {
        text_tools::trim_lines(text);
        text_tools::remove_hyphen_word_break(text);
        text_tools::clean_white_characters(text);
    }

    auto in_parts =
        text_tools::split(text, engine, m_config.lang, m_config.nb_data);

    out_parts.reserve(in_parts.first.size());

    auto start = std::chrono::steady_clock::now();

    for (auto& in_part : in_parts.first) {
        std::string out_part = m_bergamot_api_api.bergamot_api_translate(
            m_bergamot_ctx_first, in_part.c_str());
        if (m_shutting_down) return {};
        if (m_bergamot_ctx_second)
            out_part = m_bergamot_api_api.bergamot_api_translate(
                m_bergamot_ctx_second, out_part.c_str());
        if (m_shutting_down) return {};
        if (!out_part.empty()) out_parts.push_back(std::move(out_part));
    }

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start)
                   .count();

    LOGD("translation completed, stats: duration=" << dur << "ms");

    const auto& break_lines = in_parts.second;

    return std::accumulate(
        out_parts.cbegin(), out_parts.cend(), std::string{},
        [&break_lines, i = static_cast<size_t>(0)](std::string a,
                                                   std::string b) mutable {
            b = fix_text(std::move(b));
            if (a.empty()) {
                return b;
            } else {
                std::string c{std::move(a)};
                if (i < break_lines.size()) {
                    const auto& bl = break_lines.at(i);
                    if (bl.break_line)
                        c.append(std::string(bl.count, '\n'));
                    else
                        c.push_back(' ');
                    c.append(b);
                } else {
                    c.append(' ' + b);
                }

                ++i;
                return c;
            }
        });
}

void mnt_engine::process() {
    LOGD("mnt processing started");

    decltype(m_queue) queue;

    while (!m_shutting_down && m_state != state_t::error) {
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_cv.wait(lock, [this] {
                return (m_shutting_down || m_state == state_t::error) ||
                       !m_queue.empty();
            });
            std::swap(queue, m_queue);
        }

        if (m_shutting_down || m_state == state_t::error) break;

        if (!model_created()) {
            set_state(state_t::initializing);

            create_model();

            if (!model_created()) {
                set_state(state_t::error);
                if (m_call_backs.error) m_call_backs.error();
                break;
            }
        }

        set_state(state_t::translating);

        while (!m_shutting_down && !queue.empty()) {
            auto task = std::move(queue.front());
            queue.pop();

            auto text = translate_internal(task.text);

            if (m_shutting_down) break;

            m_call_backs.text_translated(task.text, m_config.lang,
                                         std::move(text), m_config.out_lang);
        }

        set_state(state_t::idle);
    }

    if (m_shutting_down) set_state(state_t::idle);

    LOGD("mnt processing done");
}

bool mnt_engine::model_created() const {
    return static_cast<bool>(m_bergamot_ctx_first) &&
           (m_config.model_files.model_path_second.empty() ||
            static_cast<bool>(m_bergamot_ctx_second));
}

void mnt_engine::create_model() {
    auto create = [this](void** bergamot_ctx, const std::string& model_path) {
        auto model_file = find_file_with_name_prefix(model_path, "model");
        auto vocab_file = find_file_with_name_prefix(model_path, "vocab");
        auto src_vocab_file =
            find_file_with_name_prefix(model_path, "srcvocab");
        auto trg_vocab_file =
            find_file_with_name_prefix(model_path, "trgvocab");
        auto shortlist_path = find_file_with_name_prefix(model_path, "lex");

        if (model_file.empty() ||
            (vocab_file.empty() &&
             (src_vocab_file.empty() || trg_vocab_file.empty())) ||
            shortlist_path.empty()) {
            LOGE("failed to find all model files");
            return;
        }

        if (!vocab_file.empty()) {
            src_vocab_file.assign(vocab_file);
            trg_vocab_file.assign(vocab_file);
        }

        try {
            *bergamot_ctx = m_bergamot_api_api.bergamot_api_make(
                model_file.c_str(), src_vocab_file.c_str(),
                trg_vocab_file.c_str(), shortlist_path.c_str(),
                /*num_workers=*/1,
                /*cache_size=*/500000, nullptr);
        } catch (const std::exception& err) {
            LOGE("error: " << err.what());
        }

        if (!*bergamot_ctx) LOGE("failed to make bergamot api");
    };

    create(&m_bergamot_ctx_first, m_config.model_files.model_path_first);

    if (m_bergamot_ctx_first && !m_config.model_files.model_path_second.empty())
        create(&m_bergamot_ctx_second, m_config.model_files.model_path_second);
}

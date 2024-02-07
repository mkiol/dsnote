/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
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
#include <regex>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "text_tools.hpp"

std::ostream& operator<<(std::ostream& os,
                         mnt_engine::text_format_t text_format) {
    switch (text_format) {
        case mnt_engine::text_format_t::raw:
            os << "raw";
            break;
        case mnt_engine::text_format_t::html:
            os << "html";
            break;
        case mnt_engine::text_format_t::markdown:
            os << "markdown";
            break;
        case mnt_engine::text_format_t::subrip:
            os << "subrip";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, mnt_engine::error_t error_type) {
    switch (error_type) {
        case mnt_engine::error_t::init:
            os << "init";
            break;
        case mnt_engine::error_t::runtime:
            os << "runtime";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const mnt_engine::model_files_t& model_files) {
    os << "model-path-first=" << model_files.model_path_first
       << ", model-path-second=" << model_files.model_path_second;

    return os;
}

std::ostream& operator<<(std::ostream& os, const mnt_engine::config_t& config) {
    os << "lang=" << config.lang << ", clean-text=" << config.clean_text
       << ", text-format=" << config.text_format
       << ", options=" << config.options << ", model-files=["
       << config.model_files << "]";

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

    open_lib();
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

    if (m_lib_handle) {
        dlclose(m_lib_handle);
        m_lib_handle = nullptr;
    }
}

void mnt_engine::open_lib() {
#ifdef ARCH_X86_64
    if (auto cpuinfo = cpu_tools::cpuinfo();
        cpuinfo.feature_flags & cpu_tools::feature_flags_t::avx &&
        cpuinfo.feature_flags & cpu_tools::feature_flags_t::avx2) {
        m_lib_handle = dlopen("libbergamot_api.so", RTLD_LAZY);
    } else if (cpu_tools::cpuinfo().feature_flags &
               cpu_tools::feature_flags_t::avx) {
        LOGW("using bergamot-fallback");
        m_lib_handle = dlopen("libbergamot_api-fallback.so", RTLD_LAZY);
    } else {
        LOGE("avx not supported but bergamot needs it");
        throw std::runtime_error(
            "failed to open bergamot lib: avx not supported");
    }
#else
    m_lib_handle = dlopen("libbergamot_api.so", RTLD_LAZY);
#endif

    if (m_lib_handle == nullptr) {
        LOGE("failed to open bergamot lib: " << dlerror());
        throw std::runtime_error("failed to open bergamot lib");
    }

    m_bergamot_api_api.bergamot_api_make =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_make)>(
            dlsym(m_lib_handle, "bergamot_api_make"));
    m_bergamot_api_api.bergamot_api_delete =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_delete)>(
            dlsym(m_lib_handle, "bergamot_api_delete"));
    m_bergamot_api_api.bergamot_api_translate =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_translate)>(
            dlsym(m_lib_handle, "bergamot_api_translate"));
    m_bergamot_api_api.bergamot_api_cancel =
        reinterpret_cast<decltype(m_bergamot_api_api.bergamot_api_cancel)>(
            dlsym(m_lib_handle, "bergamot_api_cancel"));

    if (!m_bergamot_api_api.ok()) {
        LOGE("failed to register bergamon api");
        throw std::runtime_error("failed to register bergamot api");
    }
}

bool mnt_engine::available() {
    void* lib_handle = nullptr;
#ifdef ARCH_X86_64
    if (auto cpuinfo = cpu_tools::cpuinfo();
        cpuinfo.feature_flags & cpu_tools::feature_flags_t::avx &&
        cpuinfo.feature_flags & cpu_tools::feature_flags_t::avx2) {
        lib_handle = dlopen("libbergamot_api.so", RTLD_LAZY);
    } else if (cpu_tools::cpuinfo().feature_flags &
               cpu_tools::feature_flags_t::avx) {
        lib_handle = dlopen("libbergamot_api-fallback.so", RTLD_LAZY);
    } else {
        LOGW("mnt not available because cpu doesn't have avx");
        return false;
    }
#else
    lib_handle = dlopen("libbergamot_api.so", RTLD_LAZY);
#endif

    if (lib_handle == nullptr) {
        LOGE("failed to open bergamot lib: " << dlerror());
        return false;
    }

    dlclose(lib_handle);

    return true;
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

double mnt_engine::progress() const {
    if (m_progress.total != 0)
        return static_cast<double>(m_progress.current) / m_progress.total;
    return 0.0;
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

static text_tools::text_format_t text_fromat_from_mnt_format(
    mnt_engine::text_format_t format) {
    switch (format) {
        case mnt_engine::text_format_t::markdown:
            return text_tools::text_format_t::markdown;
        case mnt_engine::text_format_t::subrip:
            return text_tools::text_format_t::subrip;
        case mnt_engine::text_format_t::raw:
        case mnt_engine::text_format_t::html:
            break;
    }

    throw std::runtime_error{"invalid text format"};
}

std::string mnt_engine::translate_internal(std::string text) {
    if (m_config.clean_text) {
        switch (m_config.text_format) {
            case text_format_t::raw:
                text_tools::trim_lines(text);
                text_tools::remove_hyphen_word_break(text);
                text_tools::clean_white_characters(text);
                break;
            case text_format_t::html:
                text_tools::trim_lines(text);
                text_tools::clean_white_characters(text);
                break;
            case text_format_t::markdown:
                text_tools::clean_white_characters(text);
                break;
            case text_format_t::subrip:
                break;
        }
    }

    switch (m_config.text_format) {
        case text_format_t::raw:
        case text_format_t::html:
            break;
        case text_format_t::markdown:
        case text_format_t::subrip:
            text_tools::convert_text_format_to_html(
                text, text_fromat_from_mnt_format(m_config.text_format));
            break;
    }

    bool html = m_config.text_format != text_format_t::raw;

    m_progress.total = text.size();
    m_progress.current = 0;
    if (m_call_backs.progress_changed) m_call_backs.progress_changed();

    auto start = std::chrono::steady_clock::now();

    std::ostringstream out_ss;

    std::regex r{html ? "</p>|</div>|</h1>|</h2>|</h3>|</h4>" : "\n"};
    std::string line;

    const size_t segment_size = 1000;
    const size_t segment_max_size = 10 * segment_size;

    for (std::smatch sm;
         std::regex_search(text, sm, r) || !line.empty() || !text.empty();) {
        if (sm.empty()) {
            line.append(text);
            text.clear();
        } else {
            line.append(sm.prefix().str() + sm.str());
            text.assign(sm.suffix());
        }

        if (line.size() > segment_max_size) {
            text.insert(0, line, segment_max_size);
            line.resize(segment_max_size);
        }

        if (sm.empty() || line.size() > segment_size) {
            try {
                if (m_shutting_down) return {};

                line.assign(m_bergamot_api_api.bergamot_api_translate(
                    m_bergamot_ctx_first, line.c_str(), html));

                if (m_shutting_down) return {};

                if (m_bergamot_ctx_second)
                    line.assign(m_bergamot_api_api.bergamot_api_translate(
                        m_bergamot_ctx_second, line.c_str(), html));

                if (m_shutting_down) return {};
            } catch (const std::runtime_error& err) {
                LOGE("translation error: " << err.what());
                if (m_call_backs.error) m_call_backs.error(error_t::runtime);
            }

            out_ss << line;
            line.clear();

            m_progress.current = m_progress.total - text.size();
            if (m_call_backs.progress_changed) m_call_backs.progress_changed();
        }
    }

    text.assign(out_ss.str());

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start)
                   .count();

    LOGD("translation completed, stats: duration=" << dur << "ms");

    switch (m_config.text_format) {
        case text_format_t::raw:
        case text_format_t::html:
            break;
        case text_format_t::markdown:
        case text_format_t::subrip:
            text_tools::convert_text_format_from_html(
                text, text_fromat_from_mnt_format(m_config.text_format));
            break;
    }

    m_progress.current = m_progress.total;
    if (m_call_backs.progress_changed) m_call_backs.progress_changed();

    return text;
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
                if (m_call_backs.error) m_call_backs.error(error_t::init);
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

        m_progress = {};

        set_state(state_t::idle);
    }

    m_progress = {};

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

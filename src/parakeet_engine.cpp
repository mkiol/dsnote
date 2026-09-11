/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "parakeet_engine.hpp"

#include <dirent.h>
#include <dlfcn.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "text_tools.hpp"

parakeet_engine::parakeet_engine(config_t config, callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    LOGD("parakeet ctor");
    open_parakeet_lib();
    m_wparams = make_wparams();
    m_speech_buf.reserve(m_speech_max_size);
}

parakeet_engine::~parakeet_engine() {
    LOGD("parakeet dtor");

    stop();

    if (m_parakeet_api.ok()) {
        if (m_parakeet_ctx) {
            m_parakeet_api.parakeet_free(m_parakeet_ctx);
            m_parakeet_ctx = nullptr;
        }

        unload_all_backends();
    }

    m_parakeet_api = {};

    if (m_parakeetlib_handle) {
        dlclose(m_parakeetlib_handle);
        m_parakeetlib_handle = nullptr;
    }

    if (m_ggmllib_handle) {
        dlclose(m_ggmllib_handle);
        m_ggmllib_handle = nullptr;
    }

    unsetenv("GGML_OPENCL_PLATFORM");
    unsetenv("GGML_OPENCL_DEVICE");
    unsetenv("GGML_BACKEND_DIR");
}

static bool try_open_lib(const char* lib) {
    LOGD("try to open parakeet lib: " << lib);

    setenv("GGML_NO_BACKTRACE", "1", 1);

    auto* handle = dlopen(lib, RTLD_LAZY);
    if (!handle) {
        LOGW("failed to open parakeet lib: " << dlerror());
        return false;
    }

    dlclose(handle);

    return true;
}

bool parakeet_engine::available() { return try_open_lib("libparakeet.so"); }

void parakeet_engine::set_visible_devices() {
    if (m_config.use_gpu && m_config.gpu_device.api == gpu_api_t::vulkan &&
        !m_config.available_devices.empty()) {
        auto devs_str =
            fmt::format("{}", fmt::join(m_config.available_devices, ","));
        LOGD("setting GGML_VK_VISIBLE_DEVICES=" << devs_str);
        setenv("GGML_VK_VISIBLE_DEVICES", devs_str.c_str(), 1);

        // update dev-id because dev-id in whisper.cpp is an index of device
        // listed in GGML_VK_VISIBLE_DEVICES, not an index of device in vulkan
        // enumeratePhysicalDevices array
        auto dev_it = std::find(m_config.available_devices.cbegin(),
                                m_config.available_devices.cend(),
                                m_config.gpu_device.id);
        if (dev_it != m_config.available_devices.cend()) {
            auto new_id = static_cast<int>(
                std::distance(m_config.available_devices.cbegin(), dev_it));
            if (new_id != m_config.gpu_device.id)
                LOGD("changing vulkan dev id: " << m_config.gpu_device.id
                                                << " => " << new_id);
            m_config.gpu_device.id = new_id;
        }
    } else {
        unsetenv("GGML_VK_VISIBLE_DEVICES");
    }
}

bool parakeet_engine::has_cuda() {
#ifdef DEBUG
    return false;
#else
    return try_open_lib("libggml-cuda.so");
#endif
}

bool parakeet_engine::has_opencl() {
#ifdef DEBUG
    return false;
#else
    return try_open_lib("libggml-opencl.so");
#endif
}

bool parakeet_engine::has_hip() {
#ifdef DEBUG
    return false;
#else
    return try_open_lib("libggml-hip.so");
#endif
}

bool parakeet_engine::has_vulkan() {
#ifdef DEBUG
    return false;
#else
    return try_open_lib("libggml-vulkan.so");
#endif
}

bool parakeet_engine::use_gpu() const {
    return m_config.use_gpu && (m_config.gpu_device.api == gpu_api_t::cuda ||
                                m_config.gpu_device.api == gpu_api_t::rocm);
}

bool parakeet_engine::load_backend(const std::string& name) {
    LOGD("load parakeet backend: " << name);
    if (m_backend_regs.find(name) != m_backend_regs.end()) {
        // already loaded
        return true;
    }

    /* patched version of ggml lib uses GGML_BACKEND_DIR env var to set
     * directory for backend libs */
#ifdef USE_FLATPAK
#define NVIDIA_LIB_DIR "/app/extensions/nvidia/lib"
#define AMD_LIB_DIR "/app/extensions/amd/lib"
    if (name == "cuda" && file_exists(NVIDIA_LIB_DIR)) {
        setenv("GGML_BACKEND_DIR", NVIDIA_LIB_DIR, 1);
    } else if (name == "hip" && file_exists(AMD_LIB_DIR)) {
        setenv("GGML_BACKEND_DIR", AMD_LIB_DIR, 1);
    } else {
        setenv("GGML_BACKEND_DIR", m_config.lib_dir.c_str(), 1);
    }
#else
    setenv("GGML_BACKEND_DIR", m_config.lib_dir.c_str(), 1);
#endif

    auto* reg = m_parakeet_api.ggml_backend_load_best_ex(name.c_str());
    if (reg == nullptr) {
        LOGW("failed to load parakeet backed: " << name);
        return false;
    }
    auto device_count = m_parakeet_api.ggml_backend_reg_dev_count(reg);
    if (device_count == 0) {
        m_parakeet_api.ggml_backend_unload(reg);
        LOGW("failed to load parakeet backed (no devices): " << name);
        return false;
    }

    m_backend_regs.insert({name, reg});
    return true;
}

void parakeet_engine::unload_all_backends() {
    auto it = m_backend_regs.cbegin();
    while (it != m_backend_regs.cend()) {
        m_parakeet_api.ggml_backend_unload(it->second);
        it = m_backend_regs.erase(it);
    }
}

void parakeet_engine::open_parakeet_lib() {
    // load ggml

    LOGD("using ggml backend dir: " << m_config.lib_dir);
    setenv("GGML_NO_BACKTRACE", "1", 1);

    m_ggmllib_handle = dlopen("libggml.so", RTLD_LAZY);
    if (m_ggmllib_handle == nullptr) {
        LOGF("failed to open libggml.so: " << dlerror());
    }

    // load parakeet

    set_visible_devices();

    m_parakeetlib_handle = dlopen("libparakeet.so", RTLD_LAZY);
    if (m_parakeetlib_handle == nullptr) {
        LOGF("failed to open libparakeet.so: " << dlerror());
    }

#define PARAKEET_ENGINE_REGISTER_API(handle, name)                             \
    m_parakeet_api.name =                                                      \
        reinterpret_cast<decltype(m_parakeet_api.name)>(dlsym(handle, #name)); \
    if (m_parakeet_api.name == nullptr) {                                      \
        LOGF("failed to register parakeet api: " #name);                       \
    }

    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle,
                                parakeet_init_from_file_with_params)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle, parakeet_print_system_info)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle, parakeet_full)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle, parakeet_full_n_segments)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle, parakeet_full_n_segments)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle,
                                parakeet_full_get_segment_text)                         
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle,
                                parakeet_full_get_segment_t0)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle,
                                parakeet_full_get_segment_t1)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle, parakeet_free)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle,
                                parakeet_full_default_params)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle,
                                parakeet_context_default_params)
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle, parakeet_version);
    PARAKEET_ENGINE_REGISTER_API(m_parakeetlib_handle, parakeet_log_set);
    PARAKEET_ENGINE_REGISTER_API(m_ggmllib_handle, ggml_backend_load_all)
    PARAKEET_ENGINE_REGISTER_API(m_ggmllib_handle, ggml_backend_load_best_ex)
    PARAKEET_ENGINE_REGISTER_API(m_ggmllib_handle, ggml_backend_unload)
    PARAKEET_ENGINE_REGISTER_API(m_ggmllib_handle, ggml_log_set)
    PARAKEET_ENGINE_REGISTER_API(m_ggmllib_handle, ggml_backend_reg_dev_count)

#undef PARAKEET_ENGINE_REGISTER_API

    // set logger

    m_parakeet_api.parakeet_log_set(
        [](ggml_log_level level, const char* text,
           [[maybe_unused]] void* user_data) {
            switch (level) {
                case GGML_LOG_LEVEL_DEBUG:
                case GGML_LOG_LEVEL_CONT:
                    LOGD("parakeet: " << text);
                    break;
                case GGML_LOG_LEVEL_INFO:
                    LOGI("parakeet: " << text);
                    break;
                case GGML_LOG_LEVEL_WARN:
                    LOGW("parakeet: " << text);
                    break;
                case GGML_LOG_LEVEL_ERROR:
                    LOGE("parakeet: " << text);
                    break;
                case GGML_LOG_LEVEL_NONE:
                    break;
            }
        },
        nullptr);

    LOGD("parakeet lib version: " << m_parakeet_api.parakeet_version());

    // load backends

    if (!load_backend("cpu")) {
        LOGF("failed to load parakeet mandatory backend");
    }

    load_backend("blas");

#ifdef ARCH_ARM_64
    if (m_config.use_gpu && m_config.gpu_device.api == gpu_api_t::vulkan) {
        load_backend("vulkan");
    }
#else
    if (m_config.use_gpu) {
        if (m_config.gpu_device.api == gpu_api_t::cuda) {
            load_backend("cuda");
        } else if (m_config.gpu_device.api == gpu_api_t::rocm) {
            load_backend("hip");
        } else if (m_config.gpu_device.api == gpu_api_t::vulkan) {
            load_backend("vulkan");
        } else if (m_config.gpu_device.api == gpu_api_t::opencl) {
            if (!m_config.gpu_device.platform_name.empty() &&
                !m_config.gpu_device.name.empty()) {
                setenv("GGML_OPENCL_PLATFORM",
                       m_config.gpu_device.platform_name.c_str(), 1);
                setenv("GGML_OPENCL_DEVICE", m_config.gpu_device.name.c_str(),
                       1);
            } else {
                unsetenv("GGML_OPENCL_PLATFORM");
                unsetenv("GGML_OPENCL_DEVICE");
            }
            load_backend("opencl");
        }
    }
#endif
}

void parakeet_engine::push_buf_to_parakeet_buf(
    const std::vector<in_buf_t::buf_t::value_type>& buf,
    parakeet_buf_t& parakeet_buf) {
    // convert s16 to f32 sample format
    std::transform(buf.cbegin(), buf.cend(), std::back_inserter(parakeet_buf),
                   [](auto sample) {
                       return static_cast<parakeet_buf_t::value_type>(sample) /
                              32768.0F;
                   });
}

void parakeet_engine::push_buf_to_parakeet_buf(in_buf_t::buf_t::value_type* data,
                                             in_buf_t::buf_t::size_type size,
                                             parakeet_buf_t& parakeet_buf) {
    // convert s16 to f32 sample format
    parakeet_buf.reserve(parakeet_buf.size() + size);
    for (size_t i = 0; i < size; ++i) {
        parakeet_buf.push_back(static_cast<parakeet_buf_t::value_type>(data[i]) /
                              32768.0F);
    }
}

void parakeet_engine::reset_impl() { m_speech_buf.clear(); }

void parakeet_engine::stop_processing_impl() {
    if (m_parakeet_ctx) {
        LOGD("parakeet cancel");
    }
}

void parakeet_engine::start_processing_impl() { create_model(); }

void parakeet_engine::create_model() {
    if (m_parakeet_ctx) return;

    LOGD("creating parakeet model");

    auto params = m_parakeet_api.parakeet_context_default_params();
    params.use_gpu = m_config.use_gpu;
    params.gpu_device = m_config.gpu_device.id;

    m_parakeet_ctx = m_parakeet_api.parakeet_init_from_file_with_params(
        m_config.model_files.model_file.c_str(), params);

    if (m_parakeet_ctx == nullptr) {
        LOGF("failed to create parakeet model");
    }

    LOGD("parakeet model created");
}

stt_engine::samples_process_result_t parakeet_engine::process_buff() {
    if (!lock_buff_for_processing())
        return samples_process_result_t::wait_for_samples;

    auto eof = m_in_buf.eof;
    auto sof = m_in_buf.sof;

    LOGD("process samples buf: mode="
         << m_config.speech_mode << ", in-buf size=" << m_in_buf.size
         << ", speech-buf size=" << m_speech_buf.size() << ", sof=" << sof
         << ", eof=" << eof);

    if (sof) {
        m_speech_buf.clear();
        m_start_time.reset();
        m_vad.reset();
        reset_segment_counters();
    }

    m_denoiser.process(m_in_buf.buf.data(), m_in_buf.size);

    const auto& vad_buf =
        m_vad.remove_silence(m_in_buf.buf.data(), m_in_buf.size);

    bool vad_status = !vad_buf.empty();

    if (vad_status) {
        LOGD("vad: speech detected");

        if (m_config.speech_mode != speech_mode_t::manual &&
            m_config.speech_mode != speech_mode_t::single_sentence)
            set_speech_detection_status(
                speech_detection_status_t::speech_detected);

        if (m_config.text_format == text_format_t::raw)
            push_buf_to_parakeet_buf(vad_buf, m_speech_buf);
        else
            push_buf_to_parakeet_buf(m_in_buf.buf.data(), m_in_buf.size,
                                    m_speech_buf);

        restart_sentence_timer();
    } else {
        LOGD("vad: no speech");

        if (m_config.speech_mode == speech_mode_t::single_sentence &&
            m_speech_buf.empty() && sentence_timer_timed_out()) {
            LOGD("sentence timeout");
            m_call_backs.sentence_timeout();
        }

        if (m_config.speech_mode == speech_mode_t::automatic)
            set_speech_detection_status(speech_detection_status_t::no_speech);

        if (m_speech_buf.empty())
            m_segment_time_discarded_before +=
                (1000 * m_in_buf.size) / m_sample_rate;
        else
            m_segment_time_discarded_after +=
                (1000 * m_in_buf.size) / m_sample_rate;
    }

    m_in_buf.clear();

    auto decode_samples = [&] {
        if (m_speech_buf.size() > m_speech_max_size) {
            LOGD("speech buf reached max size");
            return true;
        }

        if (m_speech_buf.empty()) return false;

        if ((m_config.speech_mode == speech_mode_t::manual ||
             m_speech_detection_status ==
                 speech_detection_status_t::speech_detected) &&
            vad_status && !eof)
            return false;

        if ((m_config.speech_mode == speech_mode_t::manual ||
             m_config.speech_mode == speech_mode_t::single_sentence) &&
            m_speech_detection_status == speech_detection_status_t::no_speech &&
            !eof)
            return false;

        return true;
    }();

    if (!decode_samples) {
        if (eof || (m_config.speech_mode == speech_mode_t::manual &&
                    m_speech_detection_status ==
                        speech_detection_status_t::no_speech)) {
            flush(eof ? flush_t::eof : flush_t::regular);
            free_buf();
            return samples_process_result_t::no_samples_needed;
        }

        free_buf();
        return samples_process_result_t::wait_for_samples;
    }

    if (m_thread_exit_requested) {
        free_buf();
        return samples_process_result_t::no_samples_needed;
    }

    set_state(state_t::decoding);

    if (!vad_status) {
        set_speech_detection_status(speech_detection_status_t::no_speech);
    }

    LOGD("speech frame: samples=" << m_speech_buf.size());

    m_segment_time_offset += m_segment_time_discarded_before;
    m_segment_time_discarded_before = 0;

    decode_speech(m_speech_buf);

    m_segment_time_offset += (m_segment_time_discarded_after +
                              (1000 * m_speech_buf.size() / m_sample_rate));
    m_segment_time_discarded_after = 0;

    set_state(state_t::idle);

    if (m_config.speech_mode == speech_mode_t::single_sentence &&
        (!m_intermediate_text || m_intermediate_text->empty())) {
        LOGD("no speech decoded, forcing sentence timeout");
        m_call_backs.sentence_timeout();
    }

    m_speech_buf.clear();

    flush(eof || m_config.speech_mode == speech_mode_t::single_sentence
              ? flush_t::eof
              : flush_t::regular);

    free_buf();

    return samples_process_result_t::wait_for_samples;
}

static bool encoder_begin_callback([[maybe_unused]] void* ctx,
                                   [[maybe_unused]] void* state,
                                   void* user_data) {
    bool is_aborted = *static_cast<bool*>(user_data);
    return !is_aborted;
}

static bool abort_callback(void* user_data) {
    bool is_aborted = *static_cast<bool*>(user_data);
    return is_aborted;
}

parakeet_full_params parakeet_engine::make_wparams() {
    parakeet_full_params wparams =
        m_parakeet_api.parakeet_full_default_params(PARAKEET_SAMPLING_GREEDY);

    if (auto pos = m_config.lang.find('-'); pos != std::string::npos) {
        m_config.lang = m_config.lang.substr(0, pos);
    }

    wparams.encoder_begin_callback = encoder_begin_callback;
    wparams.encoder_begin_callback_user_data = &m_thread_exit_requested;
    wparams.abort_callback = abort_callback;
    wparams.abort_callback_user_data = &m_thread_exit_requested;
    wparams.audio_ctx = 0;
    wparams.no_context = false;

    if (m_config.whisper_config.has_value()) {
        wparams.n_threads = static_cast<int>(
            std::min(m_config.whisper_config->cpu_threads,
                     std::max(1U, std::thread::hardware_concurrency())));
    }

    LOGD("cpu info: arch=" << cpu_tools::arch() << ", cores="
                           << std::thread::hardware_concurrency());
    LOGD("using threads: " << wparams.n_threads << "/"
                           << std::thread::hardware_concurrency());
    LOGD("system info: " << m_parakeet_api.parakeet_print_system_info());

    return wparams;
}

void parakeet_engine::decode_speech(const parakeet_buf_t& buf) {
    LOGD("speech decoding started");

    create_model();

    auto decoding_start = std::chrono::steady_clock::now();

    bool subrip = m_config.text_format == text_format_t::subrip;
    bool inline_ts = m_config.text_format == text_format_t::inline_timestamp;

    // if (m_config.whisper_config.has_value()) {
    //     if (m_config.whisper_config->audio_ctx_conf ==
    //             audio_ctx_conf_t::dynamic &&
    //         !use_gpu()) {
    //         // short audio clips optimization
    //         // https://github.com/ggml-org/whisper.cpp/issues/1855
    //         m_wparams.audio_ctx = std::min<int>(
    //             static_cast<int>(std::clamp<size_t>(
    //                 ((1500 * buf.size()) / (m_sample_rate * 30)) + 128, 0,
    //                 std::numeric_limits<int>::max())),
    //             1500);
    //     }
    // }
    LOGD("audio_ctx: " << m_wparams.audio_ctx);

    std::ostringstream os;

    if (auto ret = m_parakeet_api.parakeet_full(m_parakeet_ctx, m_wparams,
                                              buf.data(), buf.size());
        ret == 0) {
        auto n = m_parakeet_api.parakeet_full_n_segments(m_parakeet_ctx);
        LOGD("decoded segments: " << n);

        bool add_spc = false;
        int seg_n = 0;
        std::vector<text_tools::segment_t> inline_segments;

        for (auto i = 0; i < n; ++i) {
            std::string text =
                m_parakeet_api.parakeet_full_get_segment_text(m_parakeet_ctx, i);
            if (text.empty()) continue;
            if (text.at(0) == '!') text.erase(0, 1);
            rtrim(text);
            ltrim(text);
            if (text.empty()) continue;
#ifdef DEBUG
            LOGD("segment " << i << ": " << text);
#endif
            if (subrip || inline_ts) {
                size_t t0 = std::max<int64_t>(
                                0, m_parakeet_api.parakeet_full_get_segment_t0(
                                       m_parakeet_ctx, i)) *
                            10;
                size_t t1 = std::max<int64_t>(
                                0, m_parakeet_api.parakeet_full_get_segment_t1(
                                       m_parakeet_ctx, i)) *
                            10;

                t0 += m_segment_time_offset;
                t1 += m_segment_time_offset;

                text_tools::segment_t segment{
                    .n = i + 1 + m_segment_offset,
                    .t0 = t0,
                    .t1 = t1,
                    .text = text,
                };

                if (subrip) {
                    text_tools::break_segment_to_multiline(
                        m_config.sub_config.min_line_length,
                        m_config.sub_config.max_line_length, segment);
                    text_tools::segment_to_subrip_text(segment, os);
                } else {
                    inline_segments.push_back(std::move(segment));
                }
            } else {
                if (add_spc) os << ' ';
                os << text;
                add_spc = true;
            }

            ++seg_n;
        }

        if (inline_ts && !inline_segments.empty()) {
            os << text_tools::format_segments_inline(
                inline_segments, m_config.inline_timestamp_template,
                m_config.inline_timestamp_min_interval,
                m_last_inline_timestamp_t0);
        }

        m_segment_offset += seg_n;
    } else {
        LOGE("whisper error: " << ret);
        return;
    }

    if (m_thread_exit_requested) return;

    auto stats = report_stats(
        buf.size(), m_sample_rate,
        static_cast<size_t>(std::max(
            0L, static_cast<long int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - decoding_start)
                        .count()))));

    auto result =
        merge_texts(m_intermediate_text.value_or(std::string{}), os.str());

    if (m_config.insert_stats && !result.empty()) result.append(" " + stats);

#ifdef DEBUG
    LOGD("speech decoded: text=" << result);
#endif

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result, m_config.lang);
}

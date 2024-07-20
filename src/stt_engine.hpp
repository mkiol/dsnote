/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef STT_ENGINE_H
#define STT_ENGINE_H

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <thread>
#include <utility>

#include "denoiser.hpp"
#include "punctuator.hpp"
#include "vad.hpp"

using namespace std::chrono_literals;

class stt_engine {
   public:
    enum class speech_mode_t { automatic = 0, manual = 1, single_sentence = 2 };
    friend std::ostream& operator<<(std::ostream& os, speech_mode_t mode);

    enum class speech_detection_status_t {
        no_speech = 0,
        speech_detected = 1,
        decoding = 2,
        initializing = 3
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    speech_detection_status_t status);

    enum class vad_mode_t {
        aggressiveness0 = 0,
        aggressiveness1 = 1,
        aggressiveness2 = 2,
        aggressiveness3 = 3
    };
    friend std::ostream& operator<<(std::ostream& os, vad_mode_t mode);

    enum class gpu_api_t { opencl, cuda, rocm, openvino };
    friend std::ostream& operator<<(std::ostream& os, gpu_api_t api);

    enum class audio_ctx_conf_t { dynamic, no_change, custom };
    friend std::ostream& operator<<(std::ostream& os, audio_ctx_conf_t conf);

    enum class text_format_t { raw, subrip };
    friend std::ostream& operator<<(std::ostream& os,
                                    text_format_t text_format);

    struct model_files_t {
        std::string model_file;
        std::string scorer_file;
        std::string ttt_model_file;
        std::string openvino_model_file; /* used only in whisper.cpp */

        inline bool operator==(const model_files_t& rhs) const {
            return model_file == rhs.model_file &&
                   scorer_file == rhs.scorer_file &&
                   ttt_model_file == rhs.ttt_model_file &&
                   openvino_model_file == rhs.openvino_model_file;
        };
        inline bool operator!=(const model_files_t& rhs) const {
            return !(*this == rhs);
        };
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const model_files_t& model_files);

    struct sub_config_t {
        size_t min_segment_dur = 0;
        size_t min_line_length = 0;
        size_t max_line_length = 0;
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const sub_config_t& sub_config);

    struct callbacks_t {
        std::function<void(const std::string& text, const std::string& lang)>
            text_decoded;
        std::function<void(const std::string& text, const std::string& lang)>
            intermediate_text_decoded;
        std::function<void(speech_detection_status_t status)>
            speech_detection_status_changed;
        std::function<void()> sentence_timeout;
        std::function<void()> eof;
        std::function<void()> error;
        std::function<void()> stopping;
        std::function<void()> stopped;
    };

    struct gpu_device_t {
        int id = -1;
        gpu_api_t api = gpu_api_t::opencl;
        std::string name;
        std::string platform_name;
        bool flash_attn = false;

        inline bool operator==(const gpu_device_t& rhs) const {
            return platform_name == rhs.platform_name && name == rhs.name &&
                   id == rhs.id && flash_attn == rhs.flash_attn;
        }
        inline bool operator!=(const gpu_device_t& rhs) const {
            return !(*this == rhs);
        }
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const gpu_device_t& gpu_device);

    struct config_t {
        std::string lang;
        std::string lang_code;
        model_files_t model_files;
        std::string cache_dir;
        speech_mode_t speech_mode = speech_mode_t::automatic;
        vad_mode_t vad_mode = vad_mode_t::aggressiveness3;
        bool translate = false; /*extra whisper feature*/
        bool speech_started = false;
        bool insert_stats = false;
        bool use_gpu = false;
        unsigned int cpu_threads = 4;
        unsigned int beam_search = 5;

        audio_ctx_conf_t audio_ctx_conf =
            audio_ctx_conf_t::dynamic; /*extra whisper feature*/
        int audio_ctx_size = 1500;     /*extra whisper feature*/
        text_format_t text_format = text_format_t::raw;
        std::string options;
        gpu_device_t gpu_device;
        sub_config_t sub_config;
        inline bool has_option(char c) const {
            return options.find(c) != std::string::npos;
        }
    };
    friend std::ostream& operator<<(std::ostream& os, const config_t& config);

    stt_engine(config_t config, callbacks_t call_backs);
    virtual ~stt_engine();
    std::pair<char*, size_t> borrow_buf();
    void return_buf(const char* c_buf, size_t size, bool sof, bool eof);
    void start();
    void stop();
    void request_stop();
    bool started() const;
    bool stopping() const;
    inline void restart() { m_restart_requested = true; }
    speech_detection_status_t speech_detection_status() const;
    void set_speech_mode(speech_mode_t mode);
    inline auto speech_mode() const { return m_config.speech_mode; }
    void set_speech_started(bool value);
    inline auto speech_status() const { return m_config.speech_started; }
    inline const model_files_t& model_files() const {
        return m_config.model_files;
    }
    inline const std::string& lang() const { return m_config.lang; }
    inline auto translate() const { return m_config.translate; }
    inline auto use_gpu() const { return m_config.use_gpu; }
    inline auto gpu_device() const { return m_config.gpu_device; }
    inline auto text_format() const { return m_config.text_format; }
    inline void set_text_format(text_format_t value) {
        m_config.text_format = value;
    }
    inline void set_sub_config(sub_config_t value) {
        m_config.sub_config = value;
    }
    inline bool stop_requested() const { return m_thread_exit_requested; }
    inline void set_insert_stats(bool value) { m_config.insert_stats = value; }
    inline auto audio_ctx_conf() const { return m_config.audio_ctx_conf; }
    inline auto audio_ctx_size() const { return m_config.audio_ctx_size; }
    inline auto cpu_threads() const { return m_config.cpu_threads; }
    inline auto beam_search() const { return m_config.beam_search; }

   protected:
    enum class lock_type_t { free, processed, borrowed };
    friend std::ostream& operator<<(std::ostream& os, lock_type_t lock_type);

    enum class flush_t { regular, eof, exit, restart };
    friend std::ostream& operator<<(std::ostream& os, flush_t flush_type);

    enum class samples_process_result_t { wait_for_samples, no_samples_needed };
    friend std::ostream& operator<<(std::ostream& os,
                                    samples_process_result_t result);

    enum class state_t { idle, initializing, decoding };
    friend std::ostream& operator<<(std::ostream& os, state_t state);

    inline static const size_t m_sample_rate = 16000;  // 1s
    inline static const size_t m_in_buf_max_size = 24000;
    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s
    inline static const unsigned int m_min_text_size = 4;
    inline static const auto m_timeout = 10s;

    struct in_buf_t {
        using buf_t = std::array<int16_t, m_in_buf_max_size>;
        buf_t buf{};
        buf_t::size_type size = 0;
        bool sof = true;
        bool eof = false;
        std::atomic<lock_type_t> lock = lock_type_t::free;
        [[nodiscard]] inline bool full() const { return size == buf.size(); }
        inline void clear() {
            size = 0;
            sof = false;
            eof = false;
        }
    };

    config_t m_config;
    callbacks_t m_call_backs;
    std::thread m_processing_thread;
    std::mutex m_processing_mtx;
    std::condition_variable m_processing_cv;
    bool m_thread_exit_requested = false;
    in_buf_t m_in_buf;
    std::optional<std::string> m_intermediate_text;
    std::string m_intermediate_lang;
    vad m_vad;
    denoiser m_denoiser{16000, denoiser::task_flags::task_denoise_hard |
                                   denoiser::task_flags::task_normalize};
    speech_detection_status_t m_speech_detection_status =
        speech_detection_status_t::no_speech;
    bool m_restart_requested = false;
    std::optional<std::chrono::steady_clock::time_point> m_start_time;
    state_t m_state = state_t::idle;
    std::optional<punctuator> m_punctuator;
    unsigned int m_segment_offset = 0;
    size_t m_segment_time_offset = 0;
    size_t m_segment_time_discarded_before = 0;
    size_t m_segment_time_discarded_after = 0;

    static void ltrim(std::string& s);
    static void rtrim(std::string& s);
    static std::string merge_texts(const std::string& old_text,
                                   std::string&& new_text);
    virtual samples_process_result_t process_buff();
    virtual void reset_impl() = 0;
    virtual void stop_processing_impl();
    virtual void start_processing_impl();
    virtual std::string report_stats(size_t nb_samples, size_t sample_rate,
                                     size_t processing_duration_ms);
    void flush(flush_t type);
    bool lock_buf(lock_type_t desired_lock);
    bool lock_buff_for_processing();
    void free_buf(lock_type_t lock);
    void free_buf();
    void set_speech_detection_status(speech_detection_status_t status);
    void set_intermediate_text(const std::string& text,
                               const std::string& lang);
    void set_state(state_t new_state);
    void reset_in_processing();
    void process();
    bool sentence_timer_timed_out();
    void restart_sentence_timer();
    void create_punctuator();
    void reset_segment_counters();
};

#endif  // STT_ENGINE_H

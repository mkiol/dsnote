/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef TTS_ENGINE_HPP
#define TTS_ENGINE_HPP

#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "text_tools.hpp"

class tts_engine {
   public:
    struct wav_header {
        uint8_t RIFF[4] = {'R', 'I', 'F', 'F'};
        uint32_t chunk_size = 0;
        uint8_t WAVE[4] = {'W', 'A', 'V', 'E'};
        uint8_t fmt[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_size = 16;
        uint16_t audio_format = 1;
        uint16_t num_channels = 0;
        uint32_t sample_rate = 0;
        uint32_t bytes_per_sec = 0;
        uint16_t block_align = 2;
        uint16_t bits_per_sample = 16;
        uint8_t data[4] = {'d', 'a', 't', 'a'};
        uint32_t data_size = 0;
    };

    enum class state_t {
        idle,
        stopping,
        initializing,
        speech_encoding,
        text_restoring,
        stopped,
        error
    };
    friend std::ostream& operator<<(std::ostream& os, state_t state);

    enum class gpu_api_t { opencl, cuda, rocm };
    friend std::ostream& operator<<(std::ostream& os, gpu_api_t api);

    enum class audio_format_t { wav, mp3, ogg_vorbis, ogg_opus, flac };
    friend std::ostream& operator<<(std::ostream& os, audio_format_t format);

    enum class text_format_t { raw, subrip };
    friend std::ostream& operator<<(std::ostream& os,
                                    text_format_t text_format);

    struct model_files_t {
        std::string model_path;
        std::string vocoder_path;
        std::string diacritizer_path;
        std::string hub_path;

        inline bool operator==(const model_files_t& rhs) const {
            return model_path == rhs.model_path &&
                   vocoder_path == rhs.vocoder_path &&
                   diacritizer_path == rhs.diacritizer_path &&
                   hub_path == rhs.hub_path;
        };
        inline bool operator!=(const model_files_t& rhs) const {
            return !(*this == rhs);
        };
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const model_files_t& model_files);

    struct callbacks_t {
        std::function<void(const std::string& text,
                           const std::string& audio_file_path,
                           audio_format_t format, double progress, bool last)>
            speech_encoded;
        std::function<void(const std::string& text)> text_restored;
        std::function<void(state_t state)> state_changed;
        std::function<void()> error;
    };

    struct gpu_device_t {
        int id = -1;
        gpu_api_t api = gpu_api_t::opencl;
        std::string name;
        std::string platform_name;

        inline bool operator==(const gpu_device_t& rhs) const {
            return platform_name == rhs.platform_name && name == rhs.name &&
                   id == rhs.id;
        }
        inline bool operator!=(const gpu_device_t& rhs) const {
            return !(*this == rhs);
        }
    };
    friend std::ostream& operator<<(std::ostream& os,
                                    const gpu_device_t& gpu_device);

    struct config_t {
        std::string lang;
        model_files_t model_files;
        std::string speaker_id;
        std::string ref_voice_file;
        std::string cache_dir;
        std::string data_dir;
        std::string config_dir;
        std::string share_dir;
        text_format_t text_format = text_format_t::raw;
        bool sync_subs = false;
        std::string options;
        std::string nb_data;
        std::string lang_code;
        unsigned int speech_speed = 10;
        bool split_into_sentences = true;
        bool use_engine_speed_control = true;
        bool use_gpu = false;
        gpu_device_t gpu_device;
        audio_format_t audio_format = audio_format_t::wav;
        inline bool has_option(char c) const {
            return options.find(c) != std::string::npos;
        }
    };
    friend std::ostream& operator<<(std::ostream& os, const config_t& config); 

    tts_engine(config_t config, callbacks_t call_backs);
    virtual ~tts_engine();
    void start();
    void stop();
    void request_stop();
    inline auto lang() const { return m_config.lang; }
    inline auto model_files() const { return m_config.model_files; }
    inline auto state() const { return m_state; }
    inline auto speaker() const { return m_config.speaker_id; }
    inline auto use_gpu() const { return m_config.use_gpu; }
    inline auto gpu_device() const { return m_config.gpu_device; }
    inline auto ref_voice_file() const { return m_config.ref_voice_file; }
    inline void restart() { m_restart_requested = true; }
    inline auto text_format() const { return m_config.text_format; }
    inline void set_text_format(text_format_t value) {
        m_config.text_format = value;
    }
    inline auto split_into_sentences() const {
        return m_config.split_into_sentences;
    }
    inline void set_split_into_sentences(bool value) {
        m_config.split_into_sentences = value;
    }
    inline auto use_engine_speed_control() const {
        return m_config.use_engine_speed_control;
    }
    inline void set_use_engine_speed_control(bool value) {
        m_config.use_engine_speed_control = value;
    }
    inline void set_sync_subs(bool value) { m_config.sync_subs = value; }
    void encode_speech(const std::string& text);
    void restore_text(const std::string& text);
    static std::string merge_wav_files(std::vector<std::string>&& files);
    void set_speech_speed(unsigned int speech_speed);
    void set_ref_voice_file(std::string ref_voice_file);

   protected:
    enum class task_type_t { speech_encoding, text_restoration };

    struct task_t {
        std::string text;
        size_t t0 = 0;
        size_t t1 = 0;
        task_type_t type = task_type_t::speech_encoding;
        bool first = false;
        bool last = false;

        inline bool empty() const {
            return text.empty() && t0 == 0ll && t1 == 0ll;
        }
    };

    config_t m_config;
    callbacks_t m_call_backs;
    std::thread m_processing_thread;
    std::queue<task_t> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    state_t m_state = state_t::idle;
    text_tools::processor m_text_processor;
    std::string m_ref_voice_wav_file;
    bool m_restart_requested = false;

    static std::string first_file_with_ext(std::string dir_path,
                                           std::string&& ext);
    static std::string find_file_with_name_prefix(std::string dir_path,
                                                  std::string prefix);
    static void write_wav_header(int sample_rate, int sample_width,
                                 int channels, uint32_t num_samples,
                                 std::ofstream& wav_file);
    static wav_header read_wav_header(std::ifstream& wav_file);
    static float vits_length_scale(unsigned int speech_speed,
                                   float initial_length_scale);
    static float overflow_duration_threshold(unsigned int speech_speed,
                                             float initial_duration_threshold);
    virtual bool model_created() const = 0;
    virtual bool model_supports_speed() const = 0;
    virtual void create_model() = 0;
    virtual void reset_ref_voice();
    virtual bool encode_speech_impl(const std::string& text,
                                    const std::string& out_file) = 0;
    void set_state(state_t new_state);
    std::string path_to_output_file(const std::string& text,
                                    unsigned int speech_speed,
                                    bool do_speech_change) const;
    std::string path_to_output_silence_file(size_t duration,
                                            unsigned int sample_rate,
                                            audio_format_t format) const;
    void process();
    void process_encode_speech(const task_t& task, size_t& speech_time,
                               double progress);
    void process_restore_text(const task_t& task, std::string& restored_text);
    std::vector<task_t> make_tasks(const std::string& text, bool split,
                                   task_type_t type) const;
    void apply_speed(const std::string& file) const;
    void setup_ref_voice();
    void make_silence_wav_file(size_t duration_msec, unsigned int sample_rate,
                               const std::string& output_file) const;
    void push_tasks(const std::string& text, task_type_t type);
    inline bool is_shutdown() const {
        return m_state == state_t::stopping || m_state == state_t::stopped ||
               m_state == state_t::error;
    }
    static bool add_silence(const std::string& wav_fil, size_t duration_msec);
#ifdef ARCH_X86_64
    static bool stretch(const std::string& input_file,
                        const std::string& output_file, double time_ration,
                        double pitch_ratio);
#endif
};

#endif // TTS_ENGINE_HPP

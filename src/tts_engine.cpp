/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tts_engine.hpp"

#include <dirent.h>
#include <fmt/format.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "denoiser.hpp"
#include "logger.hpp"
#include "media_compressor.hpp"
#include "mtag_tools.hpp"

static std::string file_ext_for_format(tts_engine::audio_format_t format) {
    switch (format) {
        case tts_engine::audio_format_t::wav:
            return "wav";
        case tts_engine::audio_format_t::mp3:
            return "mp3";
        case tts_engine::audio_format_t::ogg_vorbis:
            return "ogg";
        case tts_engine::audio_format_t::ogg_opus:
            return "opus";
        case tts_engine::audio_format_t::flac:
            return "flac";
    }

    throw std::runtime_error("invalid audio format");
}

static media_compressor::format_t compressor_format_from_format(
    tts_engine::audio_format_t format) {
    switch (format) {
        case tts_engine::audio_format_t::wav:
            return media_compressor::format_t::audio_wav;
        case tts_engine::audio_format_t::mp3:
            return media_compressor::format_t::audio_mp3;
        case tts_engine::audio_format_t::ogg_vorbis:
            return media_compressor::format_t::audio_ogg_vorbis;
        case tts_engine::audio_format_t::ogg_opus:
            return media_compressor::format_t::audio_ogg_opus;
        case tts_engine::audio_format_t::flac:
            return media_compressor::format_t::audio_flac;
    }

    throw std::runtime_error("invalid audio format");
}

std::ostream& operator<<(std::ostream& os, tts_engine::gpu_api_t api) {
    switch (api) {
        case tts_engine::gpu_api_t::opencl:
            os << "opencl";
            break;
        case tts_engine::gpu_api_t::cuda:
            os << "cuda";
            break;
        case tts_engine::gpu_api_t::rocm:
            os << "rocm";
            break;
        case tts_engine::gpu_api_t::openvino:
            os << "openvino";
            break;
        case tts_engine::gpu_api_t::vulkan:
            os << "vulkan";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, tts_engine::audio_format_t format) {
    switch (format) {
        case tts_engine::audio_format_t::wav:
            os << "wav";
            break;
        case tts_engine::audio_format_t::mp3:
            os << "mp3";
            break;
        case tts_engine::audio_format_t::ogg_vorbis:
            os << "ogg-vorbis";
            break;
        case tts_engine::audio_format_t::ogg_opus:
            os << "ogg-opus";
            break;
        case tts_engine::audio_format_t::flac:
            os << "flac";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         tts_engine::text_format_t text_format) {
    switch (text_format) {
        case tts_engine::text_format_t::raw:
            os << "raw";
            break;
        case tts_engine::text_format_t::subrip:
            os << "subrip";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         tts_engine::subtitles_sync_mode_t mode) {
    switch (mode) {
        case tts_engine::subtitles_sync_mode_t::off:
            os << "off";
            break;
        case tts_engine::subtitles_sync_mode_t::on_dont_fit:
            os << "on-dont-fit";
            break;
        case tts_engine::subtitles_sync_mode_t::on_always_fit:
            os << "on-always-fit";
            break;
        case tts_engine::subtitles_sync_mode_t::on_fit_only_if_longer:
            os << "on-fit-only-if-longer";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, tts_engine::tag_mode_t mode) {
    switch (mode) {
        case tts_engine::tag_mode_t::disable:
            os << "disable";
            break;
        case tts_engine::tag_mode_t::ignore:
            os << "ignore";
            break;
        case tts_engine::tag_mode_t::support:
            os << "support";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const tts_engine::model_files_t& model_files) {
    os << "model-path=" << model_files.model_path
       << ", vocoder-path=" << model_files.vocoder_path
       << ", diacritizer=" << model_files.diacritizer_path
       << ", hub-path=" << model_files.hub_path;

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const tts_engine::gpu_device_t& gpu_device) {
    os << "id=" << gpu_device.id << ", api=" << gpu_device.api
       << ", name=" << gpu_device.name
       << ", platform-name=" << gpu_device.platform_name;

    return os;
}

std::ostream& operator<<(std::ostream& os, const tts_engine::config_t& config) {
    os << "lang=" << config.lang << ", speaker=" << config.speaker_id
       << ", model-files=[" << config.model_files << "]"
       << ", speaker=" << config.speaker_id
       << ", ref_voice_file=" << config.ref_voice_file
       << ", ref_prompt=" << config.ref_prompt
       << ", text-format=" << config.text_format
       << ", sync_subs=" << config.sync_subs << ", tag_mode=" << config.tag_mode
       << ", options=" << config.options << ", lang_code=" << config.lang_code
       << ", share-dir=" << config.share_dir
       << ", cache-dir=" << config.cache_dir << ", data-dir=" << config.data_dir
       << ", speech-speed=" << config.speech_speed
       << ", split-into-sentences=" << config.split_into_sentences
       << ", use-engine-speed-control=" << config.use_engine_speed_control
       << ", normalize-audio=" << config.normalize_audio
       << ", use-gpu=" << config.use_gpu << ", gpu-device=["
       << config.gpu_device << "]"
       << ", audio-format=" << config.audio_format;
    return os;
}

std::ostream& operator<<(std::ostream& os, tts_engine::state_t state) {
    switch (state) {
        case tts_engine::state_t::idle:
            os << "idle";
            break;
        case tts_engine::state_t::stopping:
            os << "stopping";
            break;
        case tts_engine::state_t::stopped:
            os << "stopped";
            break;
        case tts_engine::state_t::initializing:
            os << "initializing";
            break;
        case tts_engine::state_t::speech_encoding:
            os << "speech-encoding";
            break;
        case tts_engine::state_t::text_restoring:
            os << "text-restoring";
            break;
        case tts_engine::state_t::error:
            os << "error";
            break;
    }

    return os;
}

tts_engine::tts_engine(config_t config, callbacks_t call_backs)
    : m_config{std::move(config)},
      m_call_backs{std::move(call_backs)},
      m_text_processor{m_config.use_gpu ? m_config.gpu_device.id : -1} {}

tts_engine::~tts_engine() {
    LOGD("tts dtor");

    if (!m_ref_voice_wav_file.empty()) unlink(m_ref_voice_wav_file.c_str());
}

void tts_engine::start() {
    LOGD("tts start");

    m_state = state_t::stopping;
    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    m_queue = std::queue<task_t>{};
    m_state = state_t::idle;
    m_processing_thread = std::thread{&tts_engine::process, this};

    LOGD("tts start completed");
}

void tts_engine::stop() {
    LOGD("tts stop started");

    set_state(state_t::stopping);

    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    set_state(state_t::stopped);

    LOGD("tts stop completed");
}

void tts_engine::request_stop() {
    LOGD("tts stop requested");

    set_state(state_t::stopping);
    m_cv.notify_one();
}

std::string tts_engine::first_file_with_ext(std::string dir_path,
                                            const std::string& ext) {
    auto* dirp = opendir(dir_path.c_str());
    if (!dirp) return {};

    while (auto* dirent = readdir(dirp)) {
        if (dirent->d_type != DT_REG) continue;

        std::string fn{dirent->d_name};

        if (!fn.empty() && fn.front() != '.' && fn.substr(fn.find_last_of('.') + 1) == ext)
            return dir_path.append("/").append(fn);
    }

    return {};
}

std::string tts_engine::find_file_with_name_prefix(std::string dir_path,
                                                   const std::string& prefix) {
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

void tts_engine::push_tasks(std::string&& text, task_type_t type) {
    auto tasks = make_tasks(
        std::move(text),
        [this]() {
            if (m_config.has_option('w')) return split_type_t::by_words;
            if (m_config.has_option('q') || !m_config.split_into_sentences)
                return split_type_t::none;
            return split_type_t::by_sentence;
        }(),
        type);

    if (tasks.empty()) {
        LOGW("no task to process");
        tasks.push_back(
            task_t{"", 0, 0, 0, 0, type,
                   task_flags::task_flag_first | task_flags::task_flag_last});
    }

    {
        std::lock_guard lock{m_mutex};
        for (auto& task : tasks) m_queue.push(std::move(task));
    }

    LOGD("task pushed");

    m_cv.notify_one();
}

void tts_engine::encode_speech(std::string text) {
    if (is_shutdown()) return;

    LOGD("tts encode speech");

    push_tasks(std::move(text), task_type_t::speech_encoding);
}

void tts_engine::restore_text(std::string text) {
    if (is_shutdown()) return;

    LOGD("tts restore text");

    push_tasks(std::move(text), task_type_t::text_restoration);
}

void tts_engine::set_speech_speed(unsigned int speech_speed) {
    m_config.speech_speed = std::clamp(speech_speed, 1U, 20U);
}

void tts_engine::set_ref_voice_file(std::string ref_voice_file) {
    m_config.ref_voice_file.assign(std::move(ref_voice_file));
    m_ref_voice_wav_file.clear();
    m_ref_voice_text.clear();
    reset_ref_voice();
}

void tts_engine::set_ref_prompt(std::string ref_prompt) {
    m_config.ref_prompt.assign(std::move(ref_prompt));
    reset_ref_voice();
}

void tts_engine::reset_ref_voice() {
    // do nothing
}

void tts_engine::set_state(state_t new_state) {
    if (is_shutdown()) {
        if (m_state == state_t::error || m_state == state_t::stopped) return;

        switch (new_state) {
            case state_t::idle:
            case state_t::stopping:
            case state_t::initializing:
            case state_t::speech_encoding:
            case state_t::text_restoring:
                new_state = state_t::stopping;
                break;
            case state_t::stopped:
            case state_t::error:
                break;
        }
    }

    if (m_state != new_state) {
        LOGD("tts engine state: " << m_state << " => " << new_state);
        m_state = new_state;
        m_call_backs.state_changed(m_state);
    }
}

static decltype(timespec::tv_sec) create_date_sec(const std::string& file) {
    struct stat result{};
    if (stat(file.c_str(), &result) == 0) return result.st_ctim.tv_sec;
    return 0;
}

std::string tts_engine::path_to_output_file(const std::string& text,
                                            unsigned int speech_speed,
                                            bool do_speech_change) const {
    auto hash = std::hash<std::string>{}(
        text + m_config.model_files.model_path +
        m_config.model_files.vocoder_path + m_config.ref_voice_file +
        std::to_string(create_date_sec(m_config.ref_voice_file)) +
        m_config.ref_prompt + m_config.model_files.diacritizer_path +
        m_config.speaker_id + m_config.lang +
        (m_config.normalize_audio ? "1" : "0") +
        (do_speech_change ? "1" : "0") +
        (speech_speed == 10 ? "" : std::to_string(speech_speed)));
    return m_config.cache_dir + "/" + std::to_string(hash) + '.' +
           file_ext_for_format(m_config.audio_format);
}

std::string tts_engine::path_to_output_silence_file(
    size_t duration_msec, unsigned int sample_rate,
    audio_format_t format) const {
    return fmt::format("{}/silence_{}_{}.{}", m_config.cache_dir, duration_msec,
                       sample_rate, file_ext_for_format(format));
}

bool tts_engine::file_exists(const std::string& file_path) {
    struct stat buffer {};
    return stat(file_path.c_str(), &buffer) == 0;
}

int64_t tts_engine::file_size(const std::string& file_path) {
    struct stat buffer{};
    if (stat(file_path.c_str(), &buffer) != 0) {
        return 0;
    }
    return buffer.st_size;
}

// source: https://stackoverflow.com/a/217605
// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}
// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}
// trim from both ends (in place)
static inline void trim(std::string& s) {
    rtrim(s);
    ltrim(s);
}

void tts_engine::make_subrip_tasks(
    const std::string& text, unsigned int speed, unsigned int silence_duration,
    task_type_t type, std::vector<tts_engine::task_t>& tasks) const {
    auto subrip_start_idx = text_tools::subrip_text_start(text, 100);
    if (subrip_start_idx) {
        auto segments =
            text_tools::subrip_text_to_segments(text, *subrip_start_idx);

        if (!segments.empty()) {
            tasks.reserve(segments.size());

            task_t task;

            for (auto& seg : segments) {
                task_t task;

                if (m_config.sync_subs != subtitles_sync_mode_t::off) {
                    task.t0 = seg.t0;
                    task.t1 = seg.t1;
                }

                task.text = std::move(seg.text);
                task.speed = speed;
                task.silence_duration = silence_duration;
                task.type = type;

                tasks.push_back(std::move(task));
            }
        }
    }
}

// source: https://rosettacode.org/wiki/Word_wrap#C++
static std::vector<std::string> wrap(const std::string& text,
                                     size_t line_length) {
    std::vector<std::string> wrapped;
    std::istringstream words(text);
    std::string word;
    std::ostringstream line;

    if (words >> word) {
        line << word;
        size_t space_left = line_length - word.length();
        while (words >> word) {
            if (space_left < word.length() + 1) {
                wrapped.push_back(line.str());
                line = {};
                line << word;
                space_left = line_length - word.length();
            } else {
                line << ' ' << word;
                space_left -= word.length() + 1;
            }
        }
    }

    if (line.tellp() > 0) wrapped.push_back(line.str());

    return wrapped;
}

void tts_engine::make_plain_tasks(
    const std::string& text, split_type_t split_type, unsigned int speed,
    unsigned int silence_duration, task_type_t type,
    std::vector<tts_engine::task_t>& tasks) const {
    switch (split_type) {
        case split_type_t::by_sentence: {
            auto [parts, _] = text_tools::split(
                text,
                m_config.has_option('a') ? text_tools::split_engine_t::astrunc
                                         : text_tools::split_engine_t::ssplit,
                m_config.lang, m_config.nb_data);
            if (parts.empty()) {
                task_t task;
                task.speed = speed;
                task.silence_duration = silence_duration;
                task.type = type;
                tasks.push_back(std::move(task));
            } else {
                tasks.reserve(parts.size());
                for (auto& part : parts) {
                    trim(part);
                    if (!part.empty()) {
                        task_t task;
                        task.text = std::move(part);
                        task.speed = speed;
                        task.silence_duration = silence_duration;
                        task.type = type;
                        tasks.push_back(std::move(task));
                    }
                }
            }
            break;
        }
        case split_type_t::by_words: {
            auto parts = wrap(text, 100);
            if (parts.empty()) {
                task_t task;
                task.speed = speed;
                task.silence_duration = silence_duration;
                task.type = type;
                tasks.push_back(std::move(task));
            } else {
                tasks.reserve(parts.size());
                for (auto& part : parts) {
                    trim(part);
                    if (!part.empty()) {
                        task_t task;
                        task.text = std::move(part);
                        task.speed = speed;
                        task.silence_duration = silence_duration;
                        task.type = type;
                        tasks.push_back(std::move(task));
                    }
                }
            }
            break;
        }
        case split_type_t::none: {
            auto t_text{text};
            text_tools::trim_line(t_text);

            task_t task;
            task.text = std::move(t_text);
            task.speed = speed;
            task.silence_duration = silence_duration;
            task.type = type;

            tasks.push_back(std::move(task));
            break;
        }
    }
}

std::vector<tts_engine::task_t> tts_engine::make_tasks(std::string text,
                                                       split_type_t split_type,
                                                       task_type_t type) const {
    std::vector<tts_engine::task_t> tasks;

    auto speed{m_config.speech_speed};

    text_tools::remove_stats_tag(text);

    if (m_config.text_format == text_format_t::subrip) {
        make_subrip_tasks(text, speed, 0, type, tasks);

        if (tasks.empty()) LOGW("tts fallback to plain text");
    }

    if (tasks.empty()) {
        switch (m_config.tag_mode) {
            case tag_mode_t::disable:
                make_plain_tasks(text, split_type, speed, 0, type, tasks);
                break;
            case tag_mode_t::ignore:
                make_plain_tasks(text_tools::remove_control_tags(text),
                                 split_type, speed, 0, type, tasks);
                break;
            case tag_mode_t::support:
                for (const auto& part : text_tools::split_by_control_tags(text)) {
                    auto silent_duration = 0U;

                    for (const auto& tag : part.tags) {
                        switch (tag.type) {
                            case text_tools::tag_type_t::none:
                                break;
                            case text_tools::tag_type_t::silence:
                                silent_duration += tag.value;
                                break;
                            case text_tools::tag_type_t::speech_change:
                                speed = tag.value;
                                break;
                        }
                    }

                    make_plain_tasks(part.text, split_type, speed,
                                     silent_duration, type, tasks);
                }
                break;
        }
    }

    if (!tasks.empty()) {
        tasks.front().flags |= task_flags::task_flag_first;
        tasks.back().flags |= task_flags::task_flag_last;
    }

    return tasks;
}

bool tts_engine::convert_wav_to_16bits(const std::string& wav_file) {
    {
        std::ifstream is{wav_file, std::ios::binary | std::ios::ate};
        if (!is) {
            LOGE("failed to open input file: " << wav_file);
            return false;
        }

        size_t size = is.tellg();
        if (size < sizeof(wav_header)) {
            LOGE("file header is too short");
            return false;
        }

        is.seekg(0, std::ios::beg);
        auto header = read_wav_header(is);

        LOGD("wav file info before convert: sample-rate="
             << header.sample_rate << ", channels=" << header.num_channels
             << ", bits=" << header.bits_per_sample);

        if (header.bits_per_sample == 16)
            return true;  // already converted to 16bits
    }

    auto tmp_file = wav_file + "_tmp.wav";

    media_compressor{}.decompress_to_file({wav_file}, tmp_file, {});

    if (!file_exists(tmp_file)) {
        LOGE("file doesn't exist after convert: " << tmp_file);
        return false;
    }

    unlink(wav_file.c_str());
    rename(tmp_file.c_str(), wav_file.c_str());

    return true;
}

void tts_engine::post_process_wav(const std::string& wav_file,
                                  size_t silence_duration_msec) const {
    std::ifstream is{wav_file, std::ios::binary | std::ios::ate};
    if (!is) {
        LOGF("failed to open input file: " << wav_file);
    }

    auto tmp_file = wav_file + "_tmp";

    std::ofstream os{tmp_file, std::ios::binary};
    if (!os) {
        LOGF("failed to open output file: " << tmp_file);
    }

    size_t size = is.tellg();
    if (size < sizeof(wav_header)) {
        os.close();
        unlink(tmp_file.c_str());
        LOGF("file header is too short");
    }

    is.seekg(0, std::ios::beg);
    auto header = read_wav_header(is);

    size_t header_size = is.tellg();
    size -= header_size;

    LOGD("wav file info: sample-rate=" << header.sample_rate
                                       << ", channels=" << header.num_channels
                                       << ", bits=" << header.bits_per_sample);

    auto silence_size =
        (header.num_channels * silence_duration_msec * header.sample_rate) /
        500;

    os.seekp(0);
    write_wav_header(header.sample_rate, sizeof(int16_t), header.num_channels,
                     (size + silence_size) / sizeof(int16_t), os);

    static const size_t buf_size = 8192;
    char buf[buf_size];

    if (m_config.normalize_audio) {
        denoiser dn{static_cast<int>(header.sample_rate),
                    denoiser::task_flags::task_normalize_two_pass, size};

        auto tmp_size = size;
        while (is && tmp_size > 0) {
            auto size_to_read = std::min<size_t>(tmp_size, buf_size);
            is.read(buf, size_to_read);

            dn.process_char(buf, size_to_read);

            tmp_size -= size_to_read;
        }

        is.seekg(header_size);
        tmp_size = size;

        while (is && tmp_size > 0) {
            auto size_to_read = std::min<size_t>(tmp_size, buf_size);
            is.read(buf, size_to_read);

            dn.normalize_second_pass_char(buf, size_to_read);

            os.write(buf, size_to_read);
            tmp_size -= size_to_read;
        }
    } else {
        // copy data without normalization
        auto tmp_size = size;
        while (is && tmp_size > 0) {
            auto size_to_read = std::min<size_t>(tmp_size, buf_size);
            is.read(buf, size_to_read);
            os.write(buf, size_to_read);
            tmp_size -= size_to_read;
        }
    }

    if (silence_size > 0) {
        std::fill(&buf[0], &buf[buf_size - 1], 0);

        while (os && silence_size > 0) {
            auto size_to_write = std::min<size_t>(silence_size, buf_size);
            os.write(buf, size_to_write);
            silence_size -= size_to_write;
        }
    }

    unlink(wav_file.c_str());
    rename(tmp_file.c_str(), wav_file.c_str());
}

void tts_engine::process_restore_text(const task_t& task,
                                      std::string& restored_text) {
    if (task.empty() && (task.flags & task_flags::task_flag_last)) {
        if (m_call_backs.text_restored) {
            m_call_backs.text_restored(restored_text);
        }
        return;
    }

    if (!restored_text.empty()) restored_text.append(" ");

    restored_text.append(m_text_processor.preprocess(
        /*text=*/task.text, /*options=*/m_config.options,
        /*lang=*/m_config.lang,
        /*lang_code=*/m_config.lang_code,
        /*prefix_path=*/m_config.share_dir,
        /*diacritizer_path=*/m_config.model_files.diacritizer_path));

    if (m_call_backs.text_restored &&
        (task.flags & task_flags::task_flag_last)) {
        m_call_backs.text_restored(restored_text);
    }
}

size_t tts_engine::handle_silence(unsigned long duration,
                                  unsigned int sample_rate, double progress,
                                  bool last) const {
    auto silence_out_file = path_to_output_silence_file(duration, sample_rate,
                                                        m_config.audio_format);

    if (!file_exists(silence_out_file)) {
        auto silence_out_file_wav = path_to_output_silence_file(
            duration, sample_rate, audio_format_t::wav);

        make_silence_wav_file(duration, sample_rate, silence_out_file_wav);

        if (m_config.audio_format != audio_format_t::wav) {
            media_compressor::options_t opts{
                media_compressor::quality_t::vbr_high,
                media_compressor::flags_t::flag_none,
                1.0,
                {},
                {}};

            media_compressor{}.compress_to_file(
                {silence_out_file_wav}, silence_out_file,
                compressor_format_from_format(m_config.audio_format), opts);

            unlink(silence_out_file_wav.c_str());
        } else {
            silence_out_file = std::move(silence_out_file_wav);
        }
    }

    auto [silence_duration, _] =
        media_compressor{}.duration_and_rate(silence_out_file);

    if (m_call_backs.speech_encoded) {
        m_call_backs.speech_encoded("", silence_out_file, m_config.audio_format,
                                    progress, last);
    }

    return silence_duration;
}

void tts_engine::process_encode_speech(const task_t& task, size_t& speech_time,
                                       double progress) {
    if (task.empty() && task.flags & task_flags::task_flag_last) {
        if (m_call_backs.speech_encoded) {
            m_call_backs.speech_encoded({}, {}, audio_format_t::wav, 1.0, true);
        }
        return;
    }

    auto encode_speech = [&](const std::string& output_file,
                             unsigned int speed) {
        auto new_text = m_text_processor.preprocess(
            /*text=*/task.text, /*options=*/m_config.options,
            /*lang=*/m_config.lang,
            /*lang_code=*/m_config.lang_code,
            /*prefix_path=*/m_config.share_dir,
            /*diacritizer_path=*/m_config.model_files.diacritizer_path);

        auto output_file_wav = m_config.audio_format == audio_format_t::wav
                                   ? output_file
                                   : output_file + ".wav";

        if (!encode_speech_impl(new_text, speed, output_file_wav)) {
            unlink(output_file.c_str());
            LOGE("speech encoding error");
            if (m_call_backs.speech_encoded) {
                m_call_backs.speech_encoded(
                    "", "", m_config.audio_format, progress,
                    (task.flags & task_flags::task_flag_last) > 0);
            }

            return std::string{};
        }

        if (file_size(output_file_wav) == 0) {
            LOGW("tts engine returned empty wav file");
            unlink(output_file_wav.c_str());
            return std::string{};
        }

        convert_wav_to_16bits(output_file_wav);

        post_process_wav(output_file_wav, m_config.has_option('0') ? 150 : 0);

        return output_file_wav;
    };

    bool fit_into_timestamp =
        m_config.sync_subs == subtitles_sync_mode_t::on_always_fit ||
        m_config.sync_subs == subtitles_sync_mode_t::on_fit_only_if_longer;

    bool follow_timestamps = task.t1 != 0;

    if (task.speed != m_config.speech_speed) {
        LOGD("speed change: " << m_config.speech_speed << " => " << task.speed);
    }

    auto make_output_file = [&]() {
        if (task.text.empty()) return std::string{};

        bool do_speed_change = !m_config.use_engine_speed_control ||
                               !model_supports_speed() ||
                               (fit_into_timestamp && follow_timestamps);

        auto output_file = path_to_output_file(
            task.text, follow_timestamps && fit_into_timestamp ? 0 : task.speed,
            do_speed_change);

        if ((follow_timestamps && fit_into_timestamp) ||
            !file_exists(output_file)) {
            if (!do_speed_change) {
                auto output_file_wav = encode_speech(output_file, task.speed);
                if (output_file_wav.empty()) return std::string{};

                if (m_config.audio_format != audio_format_t::wav) {
                    media_compressor::options_t opts{
                        media_compressor::quality_t::vbr_high,
                        media_compressor::flags_t::flag_none,
                        1.0,
                        {},
                        {}};

                    media_compressor{}.compress_to_file(
                        {output_file_wav}, output_file,
                        compressor_format_from_format(m_config.audio_format),
                        opts);

                    unlink(output_file_wav.c_str());
                }
            } else {
                auto output_file_no_speed =
                    path_to_output_file(task.text, 10, false);

                if (!file_exists(output_file_no_speed)) {
                    auto output_file_wav =
                        encode_speech(output_file_no_speed, 10);
                    if (output_file_wav.empty()) return std::string{};

                    if (m_config.audio_format != audio_format_t::wav) {
                        media_compressor::options_t opts{
                            media_compressor::quality_t::vbr_high,
                            media_compressor::flags_t::flag_none,
                            1.0,
                            {},
                            {}};

                        media_compressor{}.compress_to_file(
                            {output_file_wav}, output_file_no_speed,
                            compressor_format_from_format(
                                m_config.audio_format),
                            opts);

                        unlink(output_file_wav.c_str());
                    }
                }

                auto output_file_wav = output_file_no_speed;

                if (m_config.audio_format != audio_format_t::wav) {
                    output_file_wav = output_file_no_speed + ".wav";
                    media_compressor{}.decompress_to_file(
                        {output_file_no_speed}, output_file_wav, {});
                }

                media_compressor::options_t opts{
                    media_compressor::quality_t::vbr_high,
                    media_compressor::flags_t::flag_none,
                    1.0,
                    {},
                    {}};

                if (follow_timestamps && fit_into_timestamp &&
                    task.t1 > task.t0) {
                    auto [speech_duration, _] =
                        media_compressor{}.duration_and_rate(output_file_wav);
                    auto segment_duration = task.t1 - task.t0;

                    if (segment_duration != speech_duration &&
                        (m_config.sync_subs ==
                             subtitles_sync_mode_t::on_always_fit ||
                         (m_config.sync_subs ==
                              subtitles_sync_mode_t::on_fit_only_if_longer &&
                          segment_duration < speech_duration))) {
                        opts.flags =
                            media_compressor::flags_t::flag_change_speed;
                        opts.speed = speech_duration /
                                     static_cast<double>(segment_duration);

                        LOGD("duration change to fit: "
                             << speech_duration << " => " << segment_duration
                             << ", adjusted speed=" << opts.speed);
                    }
                } else {
                    if (task.speed > 0 && task.speed <= 20 &&
                        task.speed != 10) {
                        opts.flags =
                            media_compressor::flags_t::flag_change_speed;
                        opts.speed = static_cast<double>(task.speed) / 10.0;
                    }
                }

                media_compressor{}.compress_to_file(
                    {output_file_wav}, output_file,
                    compressor_format_from_format(m_config.audio_format), opts);

                unlink(output_file_wav.c_str());
            }
        }

        return output_file;
    };

    std::string output_file = make_output_file();
    size_t speech_duration = 0;

    if (!output_file.empty()) {
        std::tie(speech_duration, m_last_speech_sample_rate) =
            media_compressor{}.duration_and_rate(output_file);
        if (speech_duration == 0 || m_last_speech_sample_rate == 0) {
            LOGW("can't get duration, most likely corupted audio file: "
                 << output_file);
            unlink(output_file.c_str());
            output_file.clear();
        }
    }

    bool no_speech = output_file.empty();
    bool delayed_silence = follow_timestamps && speech_time < task.t0;
    bool last_task = (task.flags & task_flags::task_flag_last) > 0;

    if (task.silence_duration > 0) {
        speech_time += handle_silence(
            task.silence_duration, m_last_speech_sample_rate, progress,
            !delayed_silence && no_speech && last_task);
    }

    if (follow_timestamps) {
        if (speech_time < task.t0) {
            speech_time +=
                handle_silence(task.t0 - speech_time, m_last_speech_sample_rate,
                               progress, no_speech && last_task);
        } else if (speech_time > task.t0) {
            LOGW("speech is delayed: " << speech_time - task.t0
                                       << ", consider increasing speech speed");
        }
    }

    speech_time += speech_duration;

    if (is_shutdown()) return;

    if (!no_speech || last_task) {
        if (m_call_backs.speech_encoded) {
            m_call_backs.speech_encoded(no_speech && last_task ? "" : task.text,
                                        output_file, m_config.audio_format,
                                        progress, last_task);
        }
    }
}

void tts_engine::process() {
    LOGD("tts prosessing started");

    decltype(m_queue) queue;

    while (!is_shutdown()) {
        {
            std::unique_lock<std::mutex> lock{m_mutex};
            m_cv.wait(lock,
                      [this] { return is_shutdown() || !m_queue.empty(); });
            std::swap(queue, m_queue);
        }

        if (is_shutdown()) break;

        if (!model_created()) {
            set_state(state_t::initializing);

            create_model();

            if (!model_created()) {
                set_state(state_t::error);
                if (m_call_backs.error) m_call_backs.error();
            }
        }

        if (is_shutdown()) break;

        if (m_restart_requested) {
            m_restart_requested = false;

            if (!m_ref_voice_wav_file.empty()) {
                unlink(m_ref_voice_wav_file.c_str());
                m_ref_voice_wav_file.clear();
                m_ref_voice_text.clear();
            }
        }

        setup_ref_voice();

        if (is_shutdown()) break;

        size_t speech_time = 0;
        size_t total_tasks_nb = 0;
        std::string restored_text;

        while (!is_shutdown() && !queue.empty()) {
            auto task = std::move(queue.front());

            if (task.flags & task_flags::task_flag_first) {
                speech_time = 0;
                total_tasks_nb = queue.size();
            }

            queue.pop();

            double progress =
                static_cast<double>(total_tasks_nb - queue.size()) /
                total_tasks_nb;

            switch (task.type) {
                case task_type_t::speech_encoding:
                    set_state(state_t::speech_encoding);
                    process_encode_speech(task, speech_time, progress);
                    break;
                case task_type_t::text_restoration:
                    set_state(state_t::text_restoring);
                    process_restore_text(task, restored_text);
                    break;
            }
        }

        if (!is_shutdown()) set_state(state_t::idle);
    }

    set_state(state_t::stopped);

    LOGD("tts processing done");
}

void tts_engine::make_silence_wav_file(size_t duration_msec,
                                       unsigned int sample_rate,
                                       const std::string& output_file) const {
    uint32_t nb_samples = (sample_rate * duration_msec) / 1000;

    std::ofstream wav_file{output_file};

    write_wav_header(sample_rate, 2, 1, nb_samples, wav_file);

    std::string silence(nb_samples * 2, '\0');
    wav_file.write(silence.data(), silence.size());
}

void tts_engine::setup_ref_voice() {
    if (m_config.ref_voice_file.empty()) return;

    auto hash = std::hash<std::string>{}(m_config.ref_voice_file);

    m_ref_voice_wav_file =
        m_config.cache_dir + "/" + std::to_string(hash) + ".wav";

    if (!file_exists(m_ref_voice_wav_file)) {
        media_compressor{}.decompress_to_file({m_config.ref_voice_file},
                                              m_ref_voice_wav_file, {});
    }

    if (auto mtag = mtag_tools::read(m_config.ref_voice_file)) {
        m_ref_voice_text = mtag->comment;
        LOGD("ref voice text: " << m_ref_voice_text);
    }
}

// borrowed from:
// https://github.com/rhasspy/piper/blob/master/src/cpp/wavfile.hpp
void tts_engine::write_wav_header(int sample_rate, int sample_width,
                                  int channels, uint32_t num_samples,
                                  std::ofstream& wav_file) {
    wav_header header;
    header.data_size = num_samples * sample_width * channels;
    header.chunk_size = header.data_size + sizeof(wav_header) - 8;
    header.sample_rate = sample_rate;
    header.num_channels = channels;
    header.bytes_per_sec = sample_rate * sample_width * channels;
    header.block_align = sample_width * channels;
    header.bits_per_sample = 8 * sample_width;
    wav_file.write(reinterpret_cast<const char*>(&header), sizeof(wav_header));
}

tts_engine::wav_header tts_engine::read_wav_header(std::ifstream& wav_file) {
    wav_header header;

    if (!wav_file.read(reinterpret_cast<char*>(&header), sizeof(wav_header)))
        throw std::runtime_error("failed to read file");

    if (header.data[0] != 'd' || header.data[1] != 'a' ||
        header.data[2] != 't' || header.data[3] != 'a') {
        wav_file.seekg(sizeof(wav_header) + header.data_size);

        if (!wav_file.read(reinterpret_cast<char*>(&header.data),
                           sizeof(header.data)))
            throw std::runtime_error("failed to read file");
        if (!wav_file.read(reinterpret_cast<char*>(&header.data_size),
                           sizeof(header.data_size)))
            throw std::runtime_error("failed to read file");
    }

    return header;
}

float tts_engine::vits_length_scale(unsigned int speech_speed,
                                    float initial_length_scale) {
    return initial_length_scale *
           std::pow<float>(
               (-0.9F * std::clamp(speech_speed, 1U, 20U) + 19) / 10.0F, 2);
}

float tts_engine::overflow_duration_threshold(
    unsigned int speech_speed, float initial_duration_threshold) {
    return initial_duration_threshold *
           (static_cast<float>(std::clamp(speech_speed, 1U, 20U)) / 10.0F);
}

void tts_engine::make_hf_link(const char* model_name,
                              const std::string& hub_path,
                              const std::string& cache_dir) {
    static const auto* hash = "0feb3fdd929bcd6649e0e7c5a688cf7dd012ef21";
    auto model_path = fmt::format("{}/hf_{}_hub", hub_path, model_name);

    auto dir = fmt::format("{}/models--{}", cache_dir, model_name);
    mkdir(dir.c_str(), 0755);
    mkdir(fmt::format("{}/snapshots", dir).c_str(), 0755);
    mkdir(fmt::format("{}/refs", dir).c_str(), 0755);

    if (std::ofstream os{fmt::format("{}/refs/main", dir).c_str()}) {
        os << hash;
    } else {
        LOGE("can't write hf ref file");
        return;
    }

    auto ln_target = fmt::format("{}/snapshots/{}", dir, hash);
    mkdir(dir.c_str(), 0755);

    remove(ln_target.c_str());
    LOGD("ln: " << model_path << " => " << ln_target);
    (void)symlink(model_path.c_str(), ln_target.c_str());
}

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
#include <cmath>
#include <cstdio>
#include <fstream>
#include <locale>

#ifdef ARCH_X86_64
#include <rubberband/RubberBandStretcher.h>
#endif

#include "logger.hpp"
#include "media_compressor.hpp"

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

std::ostream& operator<<(std::ostream& os,
                         const tts_engine::model_files_t& model_files) {
    os << "model-path=" << model_files.model_path
       << ", vocoder-path=" << model_files.vocoder_path
       << ", diacritizer=" << model_files.diacritizer_path
       << ", diacritizer=" << model_files.hub_path;

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
       << ", text-format=" << config.text_format
       << ", sync_subs=" << config.sync_subs << ", options=" << config.options
       << ", lang_code=" << config.lang_code
       << ", share-dir=" << config.share_dir
       << ", cache-dir=" << config.cache_dir << ", data-dir=" << config.data_dir
       << ", speech-speed=" << config.speech_speed
       << ", split-into-sentences=" << config.split_into_sentences
       << ", use-engine-speed-control=" << config.use_engine_speed_control
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
      m_text_processor{config.use_gpu ? config.gpu_device.id : -1} {}

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
                                            std::string&& ext) {
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

void tts_engine::push_tasks(const std::string& text, task_type_t type) {
    auto tasks = make_tasks(
        text, m_config.split_into_sentences && !m_config.has_option('q'), type);

    if (tasks.empty()) {
        LOGW("no task to process");
        tasks.push_back(task_t{"", 0, 0, type, true, true});
    }

    {
        std::lock_guard lock{m_mutex};
        for (auto& task : tasks) m_queue.push(std::move(task));
    }

    LOGD("task pushed");

    m_cv.notify_one();
}

void tts_engine::encode_speech(const std::string& text) {
    if (is_shutdown()) return;

    LOGD("tts encode speech");

    push_tasks(text, task_type_t::speech_encoding);
}

void tts_engine::restore_text(const std::string& text) {
    if (is_shutdown()) return;

    LOGD("tts restore text");

    push_tasks(text, task_type_t::text_restoration);
}

void tts_engine::set_speech_speed(unsigned int speech_speed) {
    m_config.speech_speed = std::clamp(speech_speed, 1u, 20u);
}

void tts_engine::set_ref_voice_file(std::string ref_voice_file) {
    m_config.ref_voice_file.assign(std::move(ref_voice_file));
    m_ref_voice_wav_file.clear();
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
    struct stat result;
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
        m_config.model_files.diacritizer_path + m_config.speaker_id +
        m_config.lang + (do_speech_change ? "1" : "0") +
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

static bool file_exists(const std::string& file_path) {
    struct stat buffer {};
    return stat(file_path.c_str(), &buffer) == 0;
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

std::vector<tts_engine::task_t> tts_engine::make_tasks(const std::string& text,
                                                       bool split,
                                                       task_type_t type) const {
    std::vector<tts_engine::task_t> tasks;

    if (m_config.text_format == text_format_t::subrip) {
        auto subrip_start_idx = text_tools::subrip_text_start(text, 100);
        if (subrip_start_idx) {
            auto segments =
                text_tools::subrip_text_to_segments(text, *subrip_start_idx);

            if (!segments.empty()) {
                tasks.reserve(segments.size());

                if (m_config.sync_subs == subtitles_sync_mode_t::off) {
                    segments.front().t0 = 0;
                    segments.front().t1 = 0;
                }

                tasks.push_back(task_t{std::move(segments.front().text),
                                       segments.front().t0, segments.front().t1,
                                       type, true, false});

                for (auto it = segments.begin() + 1; it != segments.end();
                     ++it) {
                    if (m_config.sync_subs == subtitles_sync_mode_t::off) {
                        it->t0 = 0;
                        it->t1 = 0;
                    }
                    tasks.push_back(task_t{std::move(it->text), it->t0, it->t1,
                                           type, false, false});
                }

                tasks.back().last = true;

                return tasks;
            }
        }

        LOGW("tts fallback to plain text");
    }

    if (split) {
        auto engine = m_config.has_option('a')
                          ? text_tools::split_engine_t::astrunc
                          : text_tools::split_engine_t::ssplit;
        auto [parts, _] =
            text_tools::split(text, engine, m_config.lang, m_config.nb_data);
        if (!parts.empty()) {
            tasks.reserve(parts.size());
            tasks.push_back(
                task_t{std::move(parts.front()), 0, 0, type, true, false});

            for (auto it = parts.begin() + 1; it != parts.end(); ++it) {
                trim(*it);
                if (!it->empty())
                    tasks.push_back(
                        task_t{std::move(*it), 0, 0, type, false, false});
            }

            tasks.back().last = true;
        }
    } else {
        tasks.push_back(task_t{text, 0, 0, type, true, true});
    }

    return tasks;
}

bool tts_engine::add_silence(const std::string& wav_file,
                             size_t duration_msec) {
    std::ifstream is{wav_file, std::ios::binary | std::ios::ate};
    if (!is) {
        LOGE("failed to open input file: " << wav_file);
        return false;
    }

    auto tmp_file = wav_file + "_tmp";

    std::ofstream os{tmp_file, std::ios::binary};
    if (!os) {
        LOGE("failed to open output file: " << tmp_file);
        return false;
    }

    size_t size = is.tellg();
    if (size < sizeof(wav_header)) {
        LOGE("file header is too short");
        os.close();
        unlink(tmp_file.c_str());
        return false;
    }

    is.seekg(0, std::ios::beg);
    auto header = read_wav_header(is);

    size -= is.tellg();

    LOGD("wav file info: sample-rate=" << header.sample_rate
                                       << ", channels=" << header.num_channels);

    auto silence_size =
        (header.num_channels * duration_msec * header.sample_rate) / 500;

    os.seekp(0);
    write_wav_header(header.sample_rate, sizeof(int16_t), header.num_channels,
                     (size + silence_size) / sizeof(int16_t), os);

    static const size_t buf_size = 8192;
    char buf[buf_size];

    while (is && size > 0) {
        auto size_to_read = std::min<size_t>(size, buf_size);
        is.read(buf, size_to_read);
        os.write(buf, size_to_read);
        size -= size_to_read;
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

    return true;
}

void tts_engine::process_restore_text(const task_t& task,
                                      std::string& restored_text) {
    if (task.empty() && task.last) {
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

    if (m_call_backs.text_restored && task.last) {
        m_call_backs.text_restored(restored_text);
    }
}

void tts_engine::process_encode_speech(const task_t& task, size_t& speech_time,
                                       double progress) {
    if (task.empty() && task.last) {
        if (m_call_backs.speech_encoded) {
            m_call_backs.speech_encoded({}, {}, audio_format_t::wav, 1.0, true);
        }
        return;
    }

    auto encode_speech = [&](const std::string& output_file) {
        auto new_text = m_text_processor.preprocess(
            /*text=*/task.text, /*options=*/m_config.options,
            /*lang=*/m_config.lang,
            /*lang_code=*/m_config.lang_code,
            /*prefix_path=*/m_config.share_dir,
            /*diacritizer_path=*/m_config.model_files.diacritizer_path);

        auto output_file_wav = m_config.audio_format == audio_format_t::wav
                                   ? output_file
                                   : output_file + ".wav";

        if (!encode_speech_impl(new_text, output_file_wav)) {
            unlink(output_file.c_str());
            LOGE("speech encoding error");
            if (m_call_backs.speech_encoded) {
                m_call_backs.speech_encoded("", "", m_config.audio_format,
                                            progress, task.last);
            }

            return std::string{};
        }

        if (m_config.has_option('0')) add_silence(output_file_wav, 100);

        return output_file_wav;
    };

    bool fit_into_timestamp =
        m_config.sync_subs == subtitles_sync_mode_t::on_always_fit ||
        m_config.sync_subs == subtitles_sync_mode_t::on_fit_only_if_longer;

    bool follow_timestamps = task.t1 != 0;

    bool do_speech_change = !m_config.use_engine_speed_control ||
                            !model_supports_speed() ||
                            (fit_into_timestamp && follow_timestamps);

    auto output_file = path_to_output_file(
        task.text,
        follow_timestamps && fit_into_timestamp ? 0 : m_config.speech_speed,
        do_speech_change);

    if ((follow_timestamps && fit_into_timestamp) ||
        !file_exists(output_file)) {
        if (!do_speech_change) {
            auto output_file_wav = encode_speech(output_file);
            if (output_file_wav.empty()) return;

            if (m_config.audio_format != audio_format_t::wav) {
                media_compressor::options_t opts{
                    media_compressor::quality_t::vbr_high,
                    media_compressor::flags_t::flag_none,
                    1.0,
                    {},
                    {}};

                media_compressor{}.compress_to_file(
                    {output_file_wav}, output_file,
                    compressor_format_from_format(m_config.audio_format), opts);

                unlink(output_file_wav.c_str());
            }
        } else {
            auto output_file_no_speed =
                path_to_output_file(task.text, 10, false);

            if (!file_exists(output_file_no_speed)) {
                auto output_file_wav = encode_speech(output_file_no_speed);
                if (output_file_wav.empty()) return;

                if (m_config.audio_format != audio_format_t::wav) {
                    media_compressor::options_t opts{
                        media_compressor::quality_t::vbr_high,
                        media_compressor::flags_t::flag_none,
                        1.0,
                        {},
                        {}};

                    media_compressor{}.compress_to_file(
                        {output_file_wav}, output_file_no_speed,
                        compressor_format_from_format(m_config.audio_format),
                        opts);

                    unlink(output_file_wav.c_str());
                }
            }

            auto output_file_wav = output_file_no_speed;

            if (m_config.audio_format != audio_format_t::wav) {
                output_file_wav = output_file_no_speed + ".wav";
                media_compressor{}.decompress_to_file({output_file_no_speed},
                                                      output_file_wav, {});
            }

            media_compressor::options_t opts{
                media_compressor::quality_t::vbr_high,
                media_compressor::flags_t::flag_none,
                1.0,
                {},
                {}};

            if (follow_timestamps && fit_into_timestamp && task.t1 > task.t0) {
                auto [speech_duration, _] =
                    media_compressor{}.duration_and_rate(output_file_wav);
                auto segment_duration = task.t1 - task.t0;

                if (segment_duration != speech_duration &&
                    (m_config.sync_subs ==
                         subtitles_sync_mode_t::on_always_fit ||
                     (m_config.sync_subs ==
                          subtitles_sync_mode_t::on_fit_only_if_longer &&
                      segment_duration < speech_duration))) {
                    opts.flags = media_compressor::flags_t::flag_change_speed;
                    opts.speed =
                        speech_duration / static_cast<double>(segment_duration);

                    LOGD("duration change to fit: "
                         << speech_duration << " => " << segment_duration
                         << ", adjusted speed=" << opts.speed);
                }
            } else {
                if (m_config.speech_speed > 0 && m_config.speech_speed <= 20 &&
                    m_config.speech_speed != 10) {
                    opts.flags = media_compressor::flags_t::flag_change_speed;
                    opts.speed =
                        static_cast<double>(20 - (m_config.speech_speed - 1)) /
                        10.0;
                }
            }

            media_compressor{}.compress_to_file(
                {output_file_wav}, output_file,
                compressor_format_from_format(m_config.audio_format), opts);

            auto [speech_duration_after_fit, __] =
                media_compressor{}.duration_and_rate(output_file);
            LOGD("fit durations after: " << speech_duration_after_fit);

            unlink(output_file_wav.c_str());
        }
    }

    auto [speech_duration, speech_sample_rate] =
        media_compressor{}.duration_and_rate(output_file);

    if (follow_timestamps) {
        if (speech_time < task.t0) {
            auto duration = task.t0 - speech_time;

            auto silence_out_file = path_to_output_silence_file(
                duration, speech_sample_rate, m_config.audio_format);

            if (!file_exists(silence_out_file)) {
                auto silence_out_file_wav = path_to_output_silence_file(
                    duration, speech_sample_rate, audio_format_t::wav);

                make_silence_wav_file(duration, speech_sample_rate,
                                      silence_out_file_wav);

                if (m_config.audio_format != audio_format_t::wav) {
                    media_compressor::options_t opts{
                        media_compressor::quality_t::vbr_high,
                        media_compressor::flags_t::flag_none,
                        1.0,
                        {},
                        {}};

                    media_compressor{}.compress_to_file(
                        {silence_out_file_wav}, silence_out_file,
                        compressor_format_from_format(m_config.audio_format),
                        opts);

                    unlink(silence_out_file_wav.c_str());
                } else {
                    silence_out_file = std::move(silence_out_file_wav);
                }
            }

            auto [silence_duration, _] =
                media_compressor{}.duration_and_rate(silence_out_file);

            speech_time += silence_duration;

            if (m_call_backs.speech_encoded) {
                m_call_backs.speech_encoded("", silence_out_file,
                                            m_config.audio_format, progress,
                                            false);
            }

        } else if (speech_time > task.t0) {
            LOGW("speech is delayed: " << speech_time - task.t0
                                       << ", consider increasing speech speed");
        }

        speech_time += speech_duration;
    }

    if (is_shutdown()) return;

    if (m_call_backs.speech_encoded) {
        m_call_backs.speech_encoded(task.text, output_file,
                                    m_config.audio_format, progress, task.last);
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
            }
        }

        setup_ref_voice();

        if (is_shutdown()) break;

        size_t speech_time = 0;
        size_t total_tasks_nb = 0;
        std::string restored_text;

        while (!is_shutdown() && !queue.empty()) {
            auto task = std::move(queue.front());

            if (task.first) {
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
               (-0.9f * std::clamp(speech_speed, 1u, 20u) + 19) / 10.0f, 2);
}

float tts_engine::overflow_duration_threshold(
    unsigned int speech_speed, float initial_duration_threshold) {
    return initial_duration_threshold *
           (static_cast<float>(std::clamp(speech_speed, 1u, 20u)) / 10.0f);
}

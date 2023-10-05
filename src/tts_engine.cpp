/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "tts_engine.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <locale>

#ifdef ARCH_X86_64
#include <rubberband/RubberBandStretcher.h>
#endif

#include "logger.hpp"
#include "text_tools.hpp"

std::ostream& operator<<(std::ostream& os,
                         const tts_engine::model_files_t& model_files) {
    os << "model-path=" << model_files.model_path
       << ", vocoder-path=" << model_files.vocoder_path;

    return os;
}

std::ostream& operator<<(std::ostream& os, const tts_engine::config_t& config) {
    os << "lang=" << config.lang << ", speaker=" << config.speaker
       << ", model-files=[" << config.model_files << "]"
       << ", speaker=" << config.speaker << ", options=" << config.options
       << ", lang_code=" << config.lang_code
       << ", share-dir=" << config.share_dir
       << ", cache-dir=" << config.cache_dir << ", data-dir=" << config.data_dir
       << ", speech-speed=" << config.speech_speed;
    return os;
}

std::ostream& operator<<(std::ostream& os, tts_engine::state_t state) {
    switch (state) {
        case tts_engine::state_t::idle:
            os << "idle";
            break;
        case tts_engine::state_t::initializing:
            os << "initializing";
            break;
        case tts_engine::state_t::encoding:
            os << "encoding";
            break;
        case tts_engine::state_t::error:
            os << "error";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         tts_engine::speech_speed_t speech_speed) {
    switch (speech_speed) {
        case tts_engine::speech_speed_t::very_slow:
            os << "very-slow";
            break;
        case tts_engine::speech_speed_t::slow:
            os << "slow";
            break;
        case tts_engine::speech_speed_t::normal:
            os << "normal";
            break;
        case tts_engine::speech_speed_t::fast:
            os << "fast";
            break;
        case tts_engine::speech_speed_t::very_fast:
            os << "very-fast";
            break;
    }

    return os;
}

tts_engine::tts_engine(config_t config, callbacks_t call_backs)
    : m_config{std::move(config)},
      m_call_backs{std::move(call_backs)} {}

tts_engine::~tts_engine() {
    LOGD("tts dtor");
}

void tts_engine::start() {
    LOGD("tts start");

    m_shutting_down = true;
    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    m_queue = std::queue<task_t>{};
    m_state = state_t::idle;
    m_shutting_down = false;
    m_processing_thread = std::thread{&tts_engine::process, this};

    LOGD("tts start completed");
}

void tts_engine::stop() {
    LOGD("tts stop started");

    m_shutting_down = true;

    set_state(state_t::idle);

    m_cv.notify_one();
    if (m_processing_thread.joinable()) m_processing_thread.join();

    LOGD("tts stop completed");
}

void tts_engine::request_stop() {
    LOGD("tts stop requested");

    m_shutting_down = true;

    set_state(state_t::idle);
}

std::string tts_engine::first_file_with_ext(std::string dir_path,
                                            std::string&& ext) {
    auto* dirp = opendir(dir_path.c_str());
    if (!dirp) return {};

    while (auto* dirent = readdir(dirp)) {
        if (dirent->d_type != DT_REG) continue;

        std::string fn{dirent->d_name};

        if (fn.substr(fn.find_last_of('.') + 1) == ext)
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

void tts_engine::encode_speech(std::string text) {
    if (m_shutting_down) return;

    auto tasks = make_tasks(text);

    {
        std::lock_guard lock{m_mutex};
        for (auto& task : tasks) {
            LOGD("task: " << task.text);
            m_queue.push(std::move(task));
        }
    }

    LOGD("task pushed");

    m_cv.notify_one();
}

void tts_engine::set_speech_speed(speech_speed_t speech_speed) {
    m_config.speech_speed = speech_speed;
}

void tts_engine::set_state(state_t new_state) {
    if (m_shutting_down) new_state = state_t::idle;

    if (m_state != new_state) {
        LOGD("tts engine state: " << m_state << " => " << new_state);
        m_state = new_state;
        m_call_backs.state_changed(m_state);
    }
}

std::string tts_engine::path_to_output_file(const std::string& text) const {
    auto hash = std::hash<std::string>{}(
        text + m_config.model_files.model_path +
        m_config.model_files.vocoder_path + m_config.speaker + m_config.lang +
        (m_config.speech_speed == speech_speed_t::normal
             ? ""
             : std::to_string(static_cast<int>(m_config.speech_speed))));
    return m_config.cache_dir + "/" + std::to_string(hash) + ".wav";
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
                                                       bool split) const {
    std::vector<tts_engine::task_t> tasks;

    if (split) {
        auto engine = m_config.has_option('a') ? text_tools::engine_t::astrunc
                                               : text_tools::engine_t::ssplit;
        auto [parts, _] =
            text_tools::split(text, engine, m_config.lang, m_config.nb_data);
        if (!parts.empty()) {
            tasks.reserve(parts.size());

            for (auto& part : parts) {
                trim(part);
                if (!part.empty())
                    tasks.push_back(task_t{std::move(part), false});
            }

            tasks.back().last = true;
        }
    } else {
        tasks.push_back(task_t{text, true});
    }

    return tasks;
}

#ifdef ARCH_X86_64
static void sample_buf_s16_to_f32(const int16_t* input, float* output,
                                  size_t size) {
    for (size_t i = 0; i < size; ++i)
        output[i] = static_cast<float>(input[i]) / 32768.0F;
}

static void sample_buf_f32_to_s16(const float* input, int16_t* output,
                                  size_t size) {
    for (size_t i = 0; i < size; ++i)
        output[i] = static_cast<int16_t>(input[i] * 32768.0F);
}

bool tts_engine::stretch(const std::string& input_file,
                         const std::string& output_file, double time_ration,
                         double pitch_ratio) {
    std::ifstream is{input_file, std::ios::binary | std::ios::ate};
    if (!is) {
        LOGE("failed to open input file for stretch: " << input_file);
        return false;
    }

    std::ofstream os{output_file, std::ios::binary};
    if (!os) {
        LOGE("failed to open output file for stretch: " << output_file);
        return false;
    }

    size_t size = is.tellg();
    if (size < sizeof(wav_header)) {
        LOGE("file header is too short");
        os.close();
        unlink(output_file.c_str());
        return false;
    }

    is.seekg(0, std::ios::beg);
    auto header = read_wav_header(is);

    if (header.num_channels != 1) {
        LOGE("stretching is supported only for mono");
        os.close();
        unlink(output_file.c_str());
        return false;
    }

    size -= is.tellg();

    LOGD("stretcher sample rate: " << header.sample_rate);

    RubberBand::RubberBandStretcher rb{
        header.sample_rate, /*mono*/ 1,
        RubberBand::RubberBandStretcher::DefaultOptions |
            RubberBand::RubberBandStretcher::OptionProcessOffline |
            RubberBand::RubberBandStretcher::OptionEngineFiner |
            RubberBand::RubberBandStretcher::OptionSmoothingOn |
            RubberBand::RubberBandStretcher::OptionTransientsSmooth |
            RubberBand::RubberBandStretcher::OptionWindowLong,
        time_ration, pitch_ratio};

    static const size_t buf_c_size = 8192;
    static const size_t buf_f_size = 4096;

    char buf_c[buf_c_size];
    float buf_f[buf_f_size];
    float* buf_f_ptr[2] = {buf_f, nullptr};  // mono

    while (is) {
        const auto size_to_read = std::min<size_t>(size, buf_c_size);
        const auto size_to_write = size_to_read / sizeof(int16_t);
        is.read(buf_c, size_to_read);
        sample_buf_s16_to_f32(reinterpret_cast<int16_t*>(buf_c), buf_f,
                              size_to_write);
        float* buf_f_c[2] = {buf_f, nullptr};
        rb.study(buf_f_c, size_to_write, !static_cast<bool>(is));
    }

    is.clear();
    is.seekg(sizeof(wav_header), std::ios::beg);
    os.seekp(sizeof(wav_header));

    while (is) {
        const auto size_to_read = std::min<size_t>(size, buf_c_size);
        const auto size_to_write = size_to_read / sizeof(int16_t);
        is.read(buf_c, size_to_read);
        sample_buf_s16_to_f32(reinterpret_cast<int16_t*>(buf_c), buf_f,
                              size_to_write);
        rb.process(buf_f_ptr, size_to_write, !is);

        while (true) {
            auto size_rb = rb.available();
            if (size_rb <= 0) break;

            auto size_r =
                rb.retrieve(buf_f_ptr, std::min<size_t>(size_rb, buf_f_size));
            if (size_r == 0) break;

            sample_buf_f32_to_s16(buf_f, reinterpret_cast<int16_t*>(buf_c),
                                  size_r);
            os.write(buf_c, size_r * sizeof(int16_t));
        }
    }

    auto data_size = static_cast<size_t>(os.tellp()) - sizeof(wav_header);

    if (data_size == 0) {
        os.close();
        unlink(output_file.c_str());
        return false;
    }

    os.seekp(0);
    write_wav_header(header.sample_rate, sizeof(int16_t), 1,
                     data_size / sizeof(int16_t), os);

    return true;
}
#endif  // ARCH_X86_64

void tts_engine::apply_speed([[maybe_unused]] const std::string& file) const {
#ifdef ARCH_X86_64
    auto tmp_file = file + "_tmp";

    if (m_config.speech_speed != speech_speed_t::normal) {
        double time_ratio = 1.0;
        switch (m_config.speech_speed) {
            case speech_speed_t::very_slow:
                time_ratio = 1.5;
                break;
            case speech_speed_t::slow:
                time_ratio = 1.2;
                break;
            case speech_speed_t::fast:
                time_ratio = 0.8;
                break;
            case speech_speed_t::very_fast:
                time_ratio = 0.5;
                break;
            case speech_speed_t::normal:
                break;
        }

        if (stretch(file, tmp_file, time_ratio, 1.0)) {
            unlink(file.c_str());
            rename(tmp_file.c_str(), file.c_str());
        }
    }
#endif  // ARCH_X86_64
}

void tts_engine::process() {
    LOGD("tts prosessing started");

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

        set_state(state_t::encoding);

        while (!m_shutting_down && !queue.empty()) {
            auto task = std::move(queue.front());
            queue.pop();

            auto output_file = path_to_output_file(task.text);

            if (!file_exists(output_file)) {
                auto new_text = text_tools::preprocess(
                    /*text=*/task.text, /*options=*/m_config.options,
                    /*lang=*/m_config.lang,
                    /*lang_code=*/m_config.lang_code,
                    /*prefix_path=*/m_config.share_dir);

                if (!encode_speech_impl(new_text, output_file)) {
                    unlink(output_file.c_str());
                    LOGE("speech encoding error");
                    if (m_call_backs.speech_encoded) {
                        m_call_backs.speech_encoded("", "", task.last);
                    }

                    continue;
                }

                if (!model_supports_speed()) apply_speed(output_file);
            }

            if (m_call_backs.speech_encoded) {
                m_call_backs.speech_encoded(task.text, output_file, task.last);
            }
        }

        set_state(state_t::idle);
    }

    if (m_shutting_down) set_state(state_t::idle);

    LOGD("tts processing done");
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

    return header;
}

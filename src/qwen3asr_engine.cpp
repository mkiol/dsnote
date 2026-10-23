/* Copyright (C) 2026 James David Clarke <james@jamesdavidclarke.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "qwen3asr_engine.hpp"

#include <fmt/format.h>
#include <pybind11/numpy.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "cpu_tools.hpp"
#include "logger.hpp"
#include "py_executor.hpp"
#include "py_tools.hpp"
#include "text_tools.hpp"

using namespace pybind11::literals;

namespace {
// ISO 639-1 code (dsnote lang id) => language name accepted by Qwen3-ASR
constexpr std::array<std::pair<std::string_view, std::string_view>, 29>
    qwen_langs{{{"zh", "Chinese"},    {"en", "English"},    {"ar", "Arabic"},
                {"de", "German"},     {"fr", "French"},     {"es", "Spanish"},
                {"pt", "Portuguese"}, {"id", "Indonesian"}, {"it", "Italian"},
                {"ko", "Korean"},     {"ru", "Russian"},    {"th", "Thai"},
                {"vi", "Vietnamese"}, {"ja", "Japanese"},   {"tr", "Turkish"},
                {"hi", "Hindi"},      {"ms", "Malay"},      {"nl", "Dutch"},
                {"sv", "Swedish"},    {"da", "Danish"},     {"fi", "Finnish"},
                {"pl", "Polish"},     {"cs", "Czech"},      {"tl", "Filipino"},
                {"fa", "Persian"},    {"el", "Greek"},      {"ro", "Romanian"},
                {"hu", "Hungarian"},  {"mk", "Macedonian"}}};
constexpr std::string_view qwen_cantonese = "Cantonese";
}  // namespace

qwen3asr_engine::qwen3asr_engine(config_t config, callbacks_t call_backs)
    : stt_engine{std::move(config), std::move(call_backs)} {
    if ((cpu_tools::cpuinfo().feature_flags &
         cpu_tools::feature_flags_t::feature_avx) == 0) {
        LOGE("avx not supported but qwen3asr engine needs it");
        throw std::runtime_error(
            "failed to init qwen3asr engine: avx not supported");
    }

    if (m_config.model_files.model_file.empty()) {
        LOGE("model path missing but qwen3asr engine needs it");
        throw std::runtime_error(
            "failed to init qwen3asr engine: no model path");
    }

    m_speech_buf.reserve(m_speech_max_size);
    m_auto_lang = m_config.lang == "auto";
    m_lang_name = lang_to_qwen_name(m_config.lang, m_config.lang_code);

    LOGD("qwen3asr lang: " << m_config.lang << " (" << m_config.lang_code
                           << ") => "
                           << (m_lang_name.empty() ? "auto-detection"
                                                   : m_lang_name));
}

qwen3asr_engine::~qwen3asr_engine() {
    LOGD("qwen3asr dtor");

    stop();
}

void qwen3asr_engine::stop() {
    stt_engine::stop();

    auto task = py_executor::instance()->execute([&]() {
        try {
            if (m_model) {
                m_model.reset();

                // release mem
                py::module_::import("gc").attr("collect")();
                py::object torch_cuda =
                    py::module_::import("torch").attr("cuda");
                if (torch_cuda.attr("is_available")().cast<bool>())
                    torch_cuda.attr("empty_cache")();
            }
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
        }
        return std::any{};
    });

    if (task) task->get();

    LOGD("qwen3asr stopped");
}

void qwen3asr_engine::push_buf_to_whisper_buf(
    const std::vector<in_buf_t::buf_t::value_type>& buf,
    whisper_buf_t& whisper_buf) {
    // convert s16 to f32 sample format
    std::transform(buf.cbegin(), buf.cend(), std::back_inserter(whisper_buf),
                   [](auto sample) {
                       return static_cast<whisper_buf_t::value_type>(sample) /
                              32768.0F;
                   });
}

void qwen3asr_engine::push_buf_to_whisper_buf(in_buf_t::buf_t::value_type* data,
                                              in_buf_t::buf_t::size_type size,
                                              whisper_buf_t& whisper_buf) {
    // convert s16 to f32 sample format
    whisper_buf.reserve(whisper_buf.size() + size);
    for (size_t i = 0; i < size; ++i) {
        whisper_buf.push_back(static_cast<whisper_buf_t::value_type>(data[i]) /
                              32768.0F);
    }
}

std::string qwen3asr_engine::lang_to_qwen_name(const std::string& lang,
                                               const std::string& lang_code) {
    if (lang_code == "yue") return std::string{qwen_cantonese};

    auto find_name = [](std::string_view code) {
        auto it = std::find_if(qwen_langs.cbegin(), qwen_langs.cend(),
                               [&](const auto& p) { return p.first == code; });
        return it == qwen_langs.cend() ? std::string{}
                                       : std::string{it->second};
    };

    // empty name => language auto-detection (also for "auto")
    auto name = find_name(lang);
    if (name.empty()) {
        // lang id with region suffix, e.g. "pt-BR"
        if (auto pos = lang.find('-'); pos != std::string::npos)
            name = find_name(std::string_view{lang}.substr(0, pos));
    }

    return name;
}

std::string qwen3asr_engine::qwen_name_to_lang(
    const std::string& qwen_langs_str) {
    // qwen3-asr returns comma separated language names (e.g.
    // "Chinese,English") or empty string when language is unknown
    auto name = qwen_langs_str.substr(0, qwen_langs_str.find(','));
    rtrim(name);
    ltrim(name);

    if (name.empty()) return {};

    if (name == qwen_cantonese) return "zh";

    auto it = std::find_if(qwen_langs.cbegin(), qwen_langs.cend(),
                           [&](const auto& p) { return p.second == name; });
    return it == qwen_langs.cend() ? std::string{} : std::string{it->first};
}

std::string qwen3asr_engine::make_context(const std::string& prompt) {
    // last m_context_max_size unicode chars (utf-8 safe)
    size_t chars = 0;
    auto pos = prompt.size();
    while (pos > 0) {
        --pos;
        // count only utf-8 lead bytes (skip 10xxxxxx continuation bytes)
        if ((static_cast<unsigned char>(prompt[pos]) & 0xC0U) != 0x80U &&
            ++chars == m_context_max_size)
            break;
    }

    return prompt.substr(pos);
}

// bf16 only on GPUs with native support (ROCm, NVIDIA Ampere or newer),
// torch.cuda.is_bf16_supported() also reports emulated bf16 as supported
bool qwen3asr_engine::gpu_has_native_bf16(const py::module_& torch,
                                          int device_id) {
    if (!torch.attr("version").attr("hip").is_none()) return true;
    auto capability = torch.attr("cuda")
                          .attr("get_device_capability")(device_id)
                          .cast<py::tuple>();
    return capability[0].cast<int>() >= 8;
}

// qwen3-asr doesn't provide timestamps (forced aligner is not used), so text
// of decoded chunk is split into sentences and duration of the chunk is
// divided proportionally to the length of each sentence
std::vector<text_tools::segment_t> qwen3asr_engine::make_segments(
    const std::string& text, const std::string& lang, size_t first_n, size_t t0,
    size_t duration) {
    auto utf8_length = [](const std::string& str) {
        return static_cast<size_t>(
            std::count_if(str.cbegin(), str.cend(), [](char c) {
                return (static_cast<unsigned char>(c) & 0xC0U) != 0x80U;
            }));
    };

    static const std::array astrunc_langs{"hi", "ja", "ko", "th", "zh"};
    auto split_engine = std::find(astrunc_langs.cbegin(), astrunc_langs.cend(),
                                  lang) != astrunc_langs.cend()
                            ? text_tools::split_engine_t::astrunc
                            : text_tools::split_engine_t::ssplit;

    std::vector<std::pair<std::string, size_t>> parts;
    size_t total_length = 0;

    for (auto& sentence : text_tools::split(text, split_engine, lang).first) {
        rtrim(sentence);
        ltrim(sentence);
        if (sentence.empty()) continue;
        auto length = utf8_length(sentence);
        total_length += length;
        parts.emplace_back(std::move(sentence), length);
    }

    if (parts.empty() || total_length == 0) {
        parts.clear();
        parts.emplace_back(text, 1);
        total_length = 1;
    }

    std::vector<text_tools::segment_t> segments;
    segments.reserve(parts.size());

    size_t length = 0;
    for (size_t i = 0; i < parts.size(); ++i) {
        auto seg_t0 = t0 + (duration * length) / total_length;
        length += parts[i].second;
        auto seg_t1 = i + 1 == parts.size()
                          ? t0 + duration
                          : t0 + (duration * length) / total_length;

        segments.push_back(
            text_tools::segment_t{.n = first_n + i,
                                  .t0 = seg_t0,
                                  .t1 = seg_t1,
                                  .text = std::move(parts[i].first)});
    }

    return segments;
}

void qwen3asr_engine::reset_impl() { m_speech_buf.clear(); }

void qwen3asr_engine::stop_processing_impl() {
    LOGD("qwen3asr cancel");
    m_cancel_requested = true;
}

void qwen3asr_engine::start_processing_impl() {
    m_cancel_requested = false;
    create_model();
}

void qwen3asr_engine::create_model() {
    if (m_model) return;

    LOGD("creating qwen3asr model");

    auto task = py_executor::instance()->execute([&]() {
        try {
            const auto& libs_availability =
                py_executor::instance()->libs_availability;

            auto use_cuda =
                m_config.use_gpu && libs_availability &&
                (libs_availability->torch_cuda || libs_availability->torch_hip);

            auto device_str =
                use_cuda ? fmt::format("cuda:{}", m_config.gpu_device.id)
                         : std::string{"cpu"};

            auto torch = py::module_::import("torch");

            // fp16 is never used
            auto use_bf16 =
                use_cuda ? gpu_has_native_bf16(torch, m_config.gpu_device.id)
                         : cpu_tools::cpu_has_native_bf16();
            const auto* dtype_str = use_bf16 ? "bfloat16" : "float32";

            LOGD("using device: " << device_str << ", dtype: " << dtype_str);

            py_tools::disable_transformers_progress_bar();

            auto qwen_asr = py::module_::import("qwen_asr");

            m_model.emplace(
                qwen_asr.attr("Qwen3ASRModel")
                    .attr("from_pretrained")(
                        m_config.model_files.model_file,
                        "dtype"_a = torch.attr(dtype_str),
                        "device_map"_a = device_str,
                        "max_inference_batch_size"_a = 1,
                        "max_new_tokens"_a = m_max_new_tokens_base));

            // cancellation: forward pre-hook is called before every
            // generation step
            auto hook = py::cpp_function{
                [this]([[maybe_unused]] const py::args& args,
                       [[maybe_unused]] const py::kwargs& kwargs) {
                    if (m_cancel_requested) {
                        throw std::runtime_error{"engine shutdown"};
                    }
                }};
            m_model->attr("model").attr("thinker").attr(
                "register_forward_pre_hook")(hook);
        } catch (const std::exception& err) {
            LOGE("py error: " << err.what());
            m_model.reset();
            return false;
        }

        return true;
    });

    if (!task || !std::any_cast<bool>(task->get())) {
        LOGE("failed to create qwen3asr model");
        throw std::runtime_error{"failed to create qwen3asr model"};
    }

    LOGD("qwen3asr model created");
}

stt_engine::samples_process_result_t qwen3asr_engine::process_buff() {
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
            push_buf_to_whisper_buf(vad_buf, m_speech_buf);
        else
            push_buf_to_whisper_buf(m_in_buf.buf.data(), m_in_buf.size,
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

void qwen3asr_engine::decode_speech(const whisper_buf_t& buf) {
    LOGD("speech decoding started");

    create_model();

    auto decoding_start = std::chrono::steady_clock::now();

    using result_t = std::pair<std::string, std::string>;

    auto task = py_executor::instance()->execute([&]() {
        try {
            // audio shorter than 0.5s is padded with silence
            auto size = std::max(buf.size(), m_speech_min_size);
            py::array_t<float> array(static_cast<py::ssize_t>(size));
            auto r = array.mutable_unchecked<1>();
            for (py::ssize_t i = 0; i < r.shape(0); ++i)
                r(i) = static_cast<size_t>(i) < buf.size() ? buf[i] : 0.0F;

            // transcribe() reads max_new_tokens from the model on every call
            m_model->attr("max_new_tokens") =
                py::int_(m_max_new_tokens_base +
                         m_max_new_tokens_per_sec * (size / m_sample_rate));

            auto results = m_model->attr("transcribe")(
                "audio"_a = py::make_tuple(array, m_sample_rate),
                "context"_a =
                    m_config.whisper_config.has_value()
                        ? make_context(m_config.whisper_config->initial_prompt)
                        : std::string{},
                "language"_a =
                    m_lang_name.empty()
                        ? static_cast<py::object>(py::none())
                        : static_cast<py::object>(py::str(m_lang_name)));

            auto results_list = results.cast<py::list>();
            if (results_list.empty()) return result_t{};

            py::object result = results_list[0];

            auto text = result.attr("text").cast<std::string>();
            rtrim(text);
            ltrim(text);

            std::string auto_lang = [&] {
                if (!m_auto_lang) return m_config.lang;
                auto qwen_lang = result.attr("language").cast<std::string>();
                auto lang = qwen_name_to_lang(qwen_lang);
                LOGD("auto lang: " << qwen_lang << " => " << lang);
                if (lang.empty()) return m_config.lang;
                return lang;
            }();

            if (text.empty()) return result_t{std::string{}, auto_lang};

#ifdef DEBUG
            LOGD("segment: " << text);
#endif

            std::ostringstream os;

            bool subrip = m_config.text_format == text_format_t::subrip;
            bool inline_ts =
                m_config.text_format == text_format_t::inline_timestamp;

            if (subrip || inline_ts) {
                auto segments = make_segments(
                    text, auto_lang, 1 + m_segment_offset,
                    m_segment_time_offset, (1000 * buf.size()) / m_sample_rate);
                m_segment_offset += segments.size();

                if (subrip) {
                    for (auto& segment : segments) {
                        text_tools::break_segment_to_multiline(
                            m_config.sub_config.min_line_length,
                            m_config.sub_config.max_line_length, segment);
                        text_tools::segment_to_subrip_text(segment, os);
                    }
                } else {
                    os << text_tools::format_segments_inline(
                        segments, m_config.inline_timestamp_template,
                        m_config.inline_timestamp_min_interval,
                        m_last_inline_timestamp_t0);
                }
            } else {
                os << text;
            }

            return result_t{os.str(), std::move(auto_lang)};
        } catch (const std::exception& err) {
            if (m_cancel_requested) {
                LOGD("qwen3asr decoding cancelled");
            } else {
                LOGE("qwen3asr py error: " << err.what());
            }
            return result_t{};
        }
    });

    if (!task) return;

    auto [text, auto_lang] = std::any_cast<result_t>(task->get());

    if (m_thread_exit_requested || m_cancel_requested) return;

    auto stats = report_stats(
        buf.size(), m_sample_rate,
        static_cast<size_t>(std::max(
            0L, static_cast<long int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - decoding_start)
                        .count()))));

    auto result = merge_texts(m_intermediate_text.value_or(std::string{}),
                              std::move(text));

    if (m_config.insert_stats) result.append(" " + stats);

#ifdef DEBUG
    LOGD("speech decoded: text=" << result);
#endif

    if (!m_intermediate_text || m_intermediate_text != result)
        set_intermediate_text(result, auto_lang);
}

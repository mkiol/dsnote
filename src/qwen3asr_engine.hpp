/* Copyright (C) 2026 James David Clarke <james@jamesdavidclarke.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef QWEN3ASR_ENGINE_H
#define QWEN3ASR_ENGINE_H

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pytypes.h>
#define slots Q_SLOTS

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include "stt_engine.hpp"
#include "text_tools.hpp"

namespace py = pybind11;

class qwen3asr_engine : public stt_engine {
   public:
    qwen3asr_engine(config_t config, callbacks_t call_backs);
    ~qwen3asr_engine() override;

   private:
    using whisper_buf_t = std::vector<float>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s
    // shorter audio is padded with silence (0.5s)
    inline static const size_t m_speech_min_size = m_sample_rate / 2;
    // max number of generated tokens depends on audio length (some languages
    // need ~15 tokens per second of speech)
    inline static const size_t m_max_new_tokens_base = 256;
    inline static const size_t m_max_new_tokens_per_sec = 20;
    inline static const size_t m_context_max_size = 1000;  // unicode chars

    std::optional<py::object> m_model;
    whisper_buf_t m_speech_buf;
    bool m_auto_lang = false;
    // qwen language name or empty when auto-detection should be used
    std::string m_lang_name;
    std::atomic_bool m_cancel_requested = false;

    void create_model();
    samples_process_result_t process_buff() override;
    void decode_speech(const whisper_buf_t& buf);
    static void push_buf_to_whisper_buf(
        const std::vector<in_buf_t::buf_t::value_type>& buf,
        whisper_buf_t& whisper_buf);
    static void push_buf_to_whisper_buf(in_buf_t::buf_t::value_type* data,
                                        in_buf_t::buf_t::size_type size,
                                        whisper_buf_t& whisper_buf);
    static std::string lang_to_qwen_name(const std::string& lang,
                                         const std::string& lang_code);
    static std::string qwen_name_to_lang(const std::string& qwen_langs_str);
    static std::string make_context(const std::string& prompt);
    static std::vector<text_tools::segment_t> make_segments(
        const std::string& text, const std::string& lang, size_t first_n,
        size_t t0, size_t duration);
    static bool cpu_has_native_bf16();
    static bool gpu_has_native_bf16(const py::module_& torch, int device_id);

    void reset_impl() override;
    void stop_processing_impl() override;
    void start_processing_impl() override;
    void stop();
};

#endif  // QWEN3ASR_ENGINE_H

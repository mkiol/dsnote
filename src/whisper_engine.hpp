/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef WHISPER_ENGINE_H
#define WHISPER_ENGINE_H

#include <memory>
#include <string>
#include <vector>

#include "stt_engine.hpp"

// do not include 'whisper.h' as it contains SIMD-dependent code,
// which is problematic when compiling for AMR32

/*** copied from 'whisper.h' START ***/

extern "C" {
typedef int32_t whisper_token;

enum whisper_sampling_strategy {
    WHISPER_SAMPLING_GREEDY,
    WHISPER_SAMPLING_BEAM_SEARCH
};

typedef void (*whisper_new_segment_callback)(void* ctx, void* state, int n_new,
                                             void* user_data);
typedef void (*whisper_progress_callback)(void* ctx, void* state, int progress,
                                          void* user_data);
typedef bool (*whisper_encoder_begin_callback)(void* ctx, void* state,
                                               void* user_data);
typedef bool (*whisper_abort_callback)(void* user_data);

typedef void (*whisper_logits_filter_callback)(void* ctx, void* state,
                                               const void* tokens, int n_tokens,
                                               float* logits, void* user_data);

struct whisper_full_params {
    enum whisper_sampling_strategy strategy;

    int n_threads;
    int n_max_text_ctx;  // max tokens to use from past text as prompt for the
                         // decoder
    int offset_ms;       // start offset in ms
    int duration_ms;     // audio duration to process in ms

    bool translate;
    bool no_context;      // do not use past transcription (if any) as initial
                          // prompt for the decoder
    bool no_timestamps;   // do not generate timestamps
    bool single_segment;  // force single segment output (useful for streaming)
    bool
        print_special;  // print special tokens (e.g. <SOT>, <EOT>, <BEG>, etc.)
    bool print_progress;    // print progress information
    bool print_realtime;    // print results from within whisper.cpp (avoid it,
                            // use callback instead)
    bool print_timestamps;  // print timestamps for each text segment when
                            // printing realtime

    // [EXPERIMENTAL] token-level timestamps
    bool token_timestamps;  // enable token-level timestamps
    float thold_pt;         // timestamp token probability threshold (~0.01)
    float thold_ptsum;      // timestamp token sum probability threshold (~0.01)
    int max_len;            // max segment length in characters
    bool split_on_word;  // split on word rather than on token (when used with
                         // max_len)
    int max_tokens;      // max tokens per segment (0 = no limit)

    // [EXPERIMENTAL] speed-up techniques
    // note: these can significantly reduce the quality of the output
    bool speed_up;  // speed-up the audio by 2x using Phase Vocoder
    bool
        debug_mode;  // enable debug_mode provides extra info (eg. Dump log_mel)
    int audio_ctx;   // overwrite the audio context size (0 = use default)

    // [EXPERIMENTAL] [TDRZ] tinydiarize
    bool tdrz_enable;  // enable tinydiarize speaker turn detection

    const char* initial_prompt;
    const whisper_token* prompt_tokens;
    int prompt_n_tokens;

    const char* language;
    bool detect_language;

    bool suppress_blank;
    bool suppress_non_speech_tokens;

    float temperature;
    float max_initial_ts;
    float length_penalty;

    float temperature_inc;
    float entropy_thold;
    float logprob_thold;
    float no_speech_thold;

    struct {
        int best_of;
    } greedy;

    struct {
        int beam_size;
        float patience;
    } beam_search;

    whisper_new_segment_callback new_segment_callback;
    void* new_segment_callback_user_data;

    whisper_progress_callback progress_callback;
    void* progress_callback_user_data;

    whisper_encoder_begin_callback encoder_begin_callback;
    void* encoder_begin_callback_user_data;

    whisper_abort_callback abort_callback;
    void* abort_callback_user_data;

    whisper_logits_filter_callback logits_filter_callback;
    void* logits_filter_callback_user_data;

    const void** grammar_rules;
    size_t n_grammar_rules;
    size_t i_start_rule;
    float grammar_penalty;
};
}

/*** copied from 'whisper.h' END ***/

class whisper_engine : public stt_engine {
   public:
    static bool has_cuda();
    static bool has_opencl();
    static bool has_hip();

    whisper_engine(config_t config, callbacks_t call_backs);
    ~whisper_engine() override;

   private:
    using whisper_buf_t = std::vector<float>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s
    inline static const int m_threads = 5;

    struct whisper_api {
        void* (*whisper_init_from_file)(const char* path_model) = nullptr;
        const char* (*whisper_print_system_info)() = nullptr;
        int (*whisper_full)(void* ctx, whisper_full_params params,
                            const float* samples, int n_samples) = nullptr;
        int (*whisper_full_n_segments)(void* ctx) = nullptr;
        const char* (*whisper_full_get_segment_text)(void* ctx,
                                                     int i_segment) = nullptr;
        int64_t (*whisper_full_get_segment_t0)(void* ctx,
                                               int i_segment) = nullptr;
        int64_t (*whisper_full_get_segment_t1)(void* ctx,
                                               int i_segment) = nullptr;
        void (*whisper_free)(void* ctx) = nullptr;
        whisper_full_params (*whisper_full_default_params)(
            whisper_sampling_strategy strategy) = nullptr;
        inline auto ok() const {
            return whisper_init_from_file && whisper_print_system_info &&
                   whisper_full && whisper_full_n_segments &&
                   whisper_full_get_segment_text &&
                   whisper_full_get_segment_t0 && whisper_full_get_segment_t1 &&
                   whisper_free && whisper_full_default_params;
        }
    };

    whisper_buf_t m_speech_buf;
    whisper_api m_whisper_api;
    void* m_whisperlib_handle = nullptr;
    void* m_whisper_ctx = nullptr;
    whisper_full_params m_wparams{};

    void open_whisper_lib();
    void create_model();
    samples_process_result_t process_buff() override;
    void decode_speech(const whisper_buf_t& buf);
    static void push_buf_to_whisper_buf(
        const std::vector<in_buf_t::buf_t::value_type>& buf,
        whisper_buf_t& whisper_buf);
    static void push_buf_to_whisper_buf(in_buf_t::buf_t::value_type* data,
                                        in_buf_t::buf_t::size_type size,
                                        whisper_buf_t& whisper_buf);
    whisper_full_params make_wparams();
    void reset_impl() override;
    void stop_processing_impl() override;
    void start_processing_impl() override;
};

#endif  // WHISPER_ENGINE_H

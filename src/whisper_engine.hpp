/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef WHISPER_ENGINE_H
#define WHISPER_ENGINE_H

#include <vector>

#include "stt_engine.hpp"

// do not include 'whisper.h' as it contains SIMD-dependent code,
// which is problematic when compiling for AMR32

/*** copied from 'whisper.h' START ***/

extern "C" {
typedef int32_t whisper_pos;
typedef int32_t whisper_token;
typedef int32_t whisper_seq_id;

enum whisper_alignment_heads_preset {
    WHISPER_AHEADS_NONE,
    WHISPER_AHEADS_N_TOP_MOST,  // All heads from the N-top-most text-layers
    WHISPER_AHEADS_CUSTOM,
    WHISPER_AHEADS_TINY_EN,
    WHISPER_AHEADS_TINY,
    WHISPER_AHEADS_BASE_EN,
    WHISPER_AHEADS_BASE,
    WHISPER_AHEADS_SMALL_EN,
    WHISPER_AHEADS_SMALL,
    WHISPER_AHEADS_MEDIUM_EN,
    WHISPER_AHEADS_MEDIUM,
    WHISPER_AHEADS_LARGE_V1,
    WHISPER_AHEADS_LARGE_V2,
    WHISPER_AHEADS_LARGE_V3,
};

typedef struct whisper_ahead {
    int n_text_layer;
    int n_head;
} whisper_ahead;

typedef struct whisper_aheads {
    size_t n_heads;
    const whisper_ahead* heads;
} whisper_aheads;

struct whisper_context_params {
    bool use_gpu;
    bool flash_attn;
    int gpu_device;  // CUDA device

    // [EXPERIMENTAL] Token-level timestamps with DTW
    bool dtw_token_timestamps;
    enum whisper_alignment_heads_preset dtw_aheads_preset;

    int dtw_n_top;
    struct whisper_aheads dtw_aheads;

    size_t dtw_mem_size;  // TODO: remove
};

typedef struct whisper_token_data {
    whisper_token id;   // token id
    whisper_token tid;  // forced timestamp token id

    float p;      // probability of the token
    float plog;   // log probability of the token
    float pt;     // probability of the timestamp token
    float ptsum;  // sum of probabilities of all timestamp tokens

    // token-level timestamp data
    // do not use if you haven't computed token-level timestamps
    int64_t t0;  // start time of the token
    int64_t t1;  //   end time of the token

    // [EXPERIMENTAL] Token-level timestamps with DTW
    // do not use if you haven't computed token-level timestamps with dtw
    // Roughly corresponds to the moment in audio in which the token was output
    int64_t t_dtw;

    float vlen;  // voice length of the token
} whisper_token_data;

typedef struct whisper_model_loader {
    void* context;

    size_t (*read)(void* ctx, void* output, size_t read_size);
    bool (*eof)(void* ctx);
    void (*close)(void* ctx);
} whisper_model_loader;

// grammar element type
enum whisper_gretype {
    // end of rule definition
    WHISPER_GRETYPE_END = 0,

    // start of alternate definition for rule
    WHISPER_GRETYPE_ALT = 1,

    // non-terminal element: reference to rule
    WHISPER_GRETYPE_RULE_REF = 2,

    // terminal element: character (code point)
    WHISPER_GRETYPE_CHAR = 3,

    // inverse char(s) ([^a], [^a-b] [^abc])
    WHISPER_GRETYPE_CHAR_NOT = 4,

    // modifies a preceding WHISPER_GRETYPE_CHAR or LLAMA_GRETYPE_CHAR_ALT to
    // be an inclusive range ([a-z])
    WHISPER_GRETYPE_CHAR_RNG_UPPER = 5,

    // modifies a preceding WHISPER_GRETYPE_CHAR or
    // WHISPER_GRETYPE_CHAR_RNG_UPPER to add an alternate char to match ([ab],
    // [a-zA])
    WHISPER_GRETYPE_CHAR_ALT = 6,
};

typedef struct whisper_grammar_element {
    enum whisper_gretype type;
    uint32_t value;  // Unicode code point or rule ID
} whisper_grammar_element;

// Available sampling strategies
enum whisper_sampling_strategy {
    WHISPER_SAMPLING_GREEDY,       // similar to OpenAI's GreedyDecoder
    WHISPER_SAMPLING_BEAM_SEARCH,  // similar to OpenAI's BeamSearchDecoder
};

// Text segment callback
// Called on every newly generated text segment
// Use the whisper_full_...() functions to obtain the text segments
typedef void (*whisper_new_segment_callback)(void* ctx, void* state, int n_new,
                                             void* user_data);

// Progress callback
typedef void (*whisper_progress_callback)(void* ctx, void* state, int progress,
                                          void* user_data);

// Encoder begin callback
// If not NULL, called before the encoder starts
// If it returns false, the computation is aborted
typedef bool (*whisper_encoder_begin_callback)(void* ctx, void* state,
                                               void* user_data);

// Abort callback
// If not NULL, called before ggml computation
// If it returns true, the computation is aborted
typedef bool (*ggml_abort_callback)(void* data);

// Parameters for the whisper_full() function
// If you change the order or add new parameters, make sure to update the
// default values in whisper.cpp: whisper_full_default_params()
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

    // A regular expression that matches tokens to suppress
    const char* suppress_regex;

    // tokens to provide to the whisper decoder as initial prompt
    // these are prepended to any existing text context from a previous call
    // use whisper_tokenize() to convert text to tokens
    // maximum of whisper_n_text_ctx()/2 tokens are used (typically 224)
    const char* initial_prompt;
    const whisper_token* prompt_tokens;
    int prompt_n_tokens;

    // for auto-detection, set to nullptr, "" or "auto"
    const char* language;
    bool detect_language;

    // common decoding parameters:
    bool
        suppress_blank;  // ref:
                         // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L89
    bool
        suppress_non_speech_tokens;  // ref:
                                     // https://github.com/openai/whisper/blob/7858aa9c08d98f75575035ecd6481f462d66ca27/whisper/tokenizer.py#L224-L253

    float temperature;  // initial decoding temperature, ref:
                        // https://ai.stackexchange.com/a/32478
    float
        max_initial_ts;  // ref:
                         // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
    float
        length_penalty;  // ref:
                         // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L267

    // fallback parameters
    // ref:
    // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
    float temperature_inc;
    float entropy_thold;  // similar to OpenAI's "compression_ratio_threshold"
    float logprob_thold;
    float no_speech_thold;  // TODO: not implemented

    struct {
        int best_of;  // ref:
                      // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
    } greedy;

    struct {
        int beam_size;  // ref:
                        // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265

        float patience;  // TODO: not implemented, ref:
                         // https://arxiv.org/pdf/2204.05424.pdf
    } beam_search;

    // called for every newly generated text segment
    whisper_new_segment_callback new_segment_callback;
    void* new_segment_callback_user_data;

    // called on each progress update
    whisper_progress_callback progress_callback;
    void* progress_callback_user_data;

    // called each time before the encoder starts
    whisper_encoder_begin_callback encoder_begin_callback;
    void* encoder_begin_callback_user_data;

    // called each time before ggml computation starts
    ggml_abort_callback abort_callback;
    void* abort_callback_user_data;

    // called by each decoder to filter obtained logits
    void* logits_filter_callback;
    void* logits_filter_callback_user_data;

    const whisper_grammar_element** grammar_rules;
    size_t n_grammar_rules;
    size_t i_start_rule;
    float grammar_penalty;
};
}

/*** copied from 'whisper.h' END ***/

class whisper_engine : public stt_engine {
   public:
    static bool has_cuda();
    static bool has_hip();
    static bool has_openvino();
    static bool has_opencl();

    whisper_engine(config_t config, callbacks_t call_backs);
    ~whisper_engine() override;

   private:
    using whisper_buf_t = std::vector<float>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s

    struct whisper_api {
        void* (*whisper_init_from_file_with_params)(
            const char* path_model, whisper_context_params params) = nullptr;
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
        whisper_context_params (*whisper_context_default_params)() = nullptr;
        int (*whisper_ctx_init_openvino_encoder)(
            void* ctx, const char* model_path, const char* device,
            const char* cache_dir) = nullptr;
        int (*whisper_full_lang_id)(void* ctx) = nullptr;
        const char* (*whisper_lang_str)(int id) = nullptr;
        inline auto ok() const {
            return whisper_init_from_file_with_params &&
                   whisper_print_system_info && whisper_full &&
                   whisper_full_n_segments && whisper_full_get_segment_text &&
                   whisper_full_get_segment_t0 && whisper_full_get_segment_t1 &&
                   whisper_free && whisper_full_default_params &&
                   whisper_context_default_params &&
                   whisper_ctx_init_openvino_encoder && whisper_full_lang_id &&
                   whisper_lang_str;
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
    bool use_openvino() const;
    bool use_gpu() const;
};

#endif  // WHISPER_ENGINE_H

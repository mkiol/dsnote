/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef PARAKEET_ENGINE_H
#define PARAKEET_ENGINE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "ggml_tools.hpp"

#include "stt_engine.hpp"

// do not include 'parakeet.h' as it contains SIMD-dependent code,
// which is problematic when compiling for AMR32

// clang-format off
/*** copied from 'parakeet.h' START ***/

extern "C" {
typedef int32_t parakeet_pos;
typedef int32_t parakeet_token;
typedef int32_t parakeet_seq_id;

struct parakeet_context_params {
    bool  use_gpu;
    int   gpu_device;  // CUDA device
};

typedef struct parakeet_token_data {
    parakeet_token id;  // the BPE subword ID (0-8191)

    int duration_idx;   // index into the models durations array
    int duration_value; // actual duration value
    int frame_index;

    float p;
    float plog;

    int64_t t0;
    int64_t t1;

    bool is_word_start;
} parakeet_token_data;

typedef struct parakeet_model_loader {
    void * context;

    size_t (*read)(void * ctx, void * output, size_t read_size);
    bool    (*eof)(void * ctx);
    void  (*close)(void * ctx);
} parakeet_model_loader;

// Available sampling strategies
enum parakeet_sampling_strategy {
    PARAKEET_SAMPLING_GREEDY,
};

// Token callback.
// Called for each new predicted token.
// Use the parakeet_full_...() functions to obtain the text segments
typedef void (*parakeet_new_token_callback)(
        struct parakeet_context * ctx,
            struct parakeet_state * state,
        const parakeet_token_data * token_data,
                            void * user_data);

// Text segment callback
// Called on every newly generated text segment
// Use the parakeet_full_...() functions to obtain the text segments
typedef void (*parakeet_new_segment_callback)(struct parakeet_context * ctx, struct parakeet_state * state, int n_new, void * user_data);

// Progress callback
typedef void (*parakeet_progress_callback)(struct parakeet_context * ctx, struct parakeet_state * state, int progress, void * user_data);

// Encoder begin callback
// If not NULL, called before the encoder starts
// If it returns false, the computation is aborted
typedef bool (*parakeet_encoder_begin_callback)(void * ctx, void * state, void * user_data);

// Parameters for the parakeet_full() function
// If you change the order or add new parameters, make sure to update the default values in parakeet.cpp:
// parakeet_full_default_params()
struct parakeet_full_params {
    enum parakeet_sampling_strategy strategy;

    int n_threads;
    int offset_ms;          // start offset in ms
    int duration_ms;        // audio duration to process in ms

    bool no_context;        // do not use past transcription (if any) as context

    int  audio_ctx;         // overwrite the audio context size (0 = use default)

    // called for every newly generated text segment
    parakeet_new_segment_callback new_segment_callback;
    void * new_segment_callback_user_data;

    // called for every newly generated token
    parakeet_new_token_callback new_token_callback;
    void * new_token_callback_user_data;

    // called on each progress update
    parakeet_progress_callback progress_callback;
    void * progress_callback_user_data;

    // called each time before the encoder starts
    parakeet_encoder_begin_callback encoder_begin_callback;
    void * encoder_begin_callback_user_data;

    // called each time before ggml computation starts
    ggml_abort_callback abort_callback;
    void * abort_callback_user_data;
};
}

/*** copied from 'parakeet.h' END ***/
// clang-format on

class parakeet_engine : public stt_engine {
   public:
    static bool available();
    static bool has_cuda();
    static bool has_hip();
    static bool has_opencl();
    static bool has_vulkan();

    parakeet_engine(config_t config, callbacks_t call_backs);
    ~parakeet_engine() override;

   private:
    using parakeet_buf_t = std::vector<float>;

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s

    struct parakeet_api {
        void* (*parakeet_init_from_file_with_params)(
            const char* path_model, parakeet_context_params params) = nullptr;
        const char* (*parakeet_print_system_info)() = nullptr;
        int (*parakeet_full)(void* ctx, parakeet_full_params params,
                            const float* samples, int n_samples) = nullptr;
        int (*parakeet_full_n_segments)(void* ctx) = nullptr;
        const char* (*parakeet_full_get_segment_text)(void* ctx,
                                                     int i_segment) = nullptr;
        int64_t (*parakeet_full_get_segment_t0)(void* ctx,
                                               int i_segment) = nullptr;
        int64_t (*parakeet_full_get_segment_t1)(void* ctx,
                                               int i_segment) = nullptr;
        void (*parakeet_free)(void* ctx) = nullptr;
        parakeet_full_params (*parakeet_full_default_params)(
            parakeet_sampling_strategy strategy) = nullptr;
        parakeet_context_params (*parakeet_context_default_params)() = nullptr;
        const char* (*parakeet_version)() = nullptr;
        void (*parakeet_log_set)(ggml_log_callback log_callback,
                                void* user_data) = nullptr;
        void (*ggml_backend_load_all)() = nullptr;
        void* (*ggml_backend_load_best_ex)(const char* name) = nullptr;
        void* (*ggml_backend_unload)(void* reg) = nullptr;
        void (*ggml_log_set)(ggml_log_callback log_callback,
                             void* user_data) = nullptr;
        size_t (*ggml_backend_reg_dev_count)(void* reg) = nullptr;
        auto ok() const {
            return parakeet_init_from_file_with_params &&
                   parakeet_print_system_info && parakeet_full &&
                   parakeet_full_n_segments && parakeet_full_get_segment_text &&
                   parakeet_full_get_segment_t0 && parakeet_full_get_segment_t1 &&
                   parakeet_free && parakeet_full_default_params &&
                   parakeet_context_default_params && parakeet_version && parakeet_log_set &&
                   ggml_backend_load_all && ggml_backend_load_best_ex &&
                   ggml_backend_unload && ggml_log_set &&
                   ggml_backend_reg_dev_count;
        }
    };

    parakeet_buf_t m_speech_buf;
    parakeet_api m_parakeet_api;
    void* m_parakeetlib_handle = nullptr;
    void* m_ggmllib_handle = nullptr;
    void* m_parakeet_ctx = nullptr;
    parakeet_full_params m_wparams{};
    std::unordered_map<std::string, void*> m_backend_regs;

    void open_parakeet_lib();
    void create_model();
    samples_process_result_t process_buff() override;
    void decode_speech(const parakeet_buf_t& buf);
    static void push_buf_to_parakeet_buf(
        const std::vector<in_buf_t::buf_t::value_type>& buf,
        parakeet_buf_t& parakeet_buf);
    static void push_buf_to_parakeet_buf(in_buf_t::buf_t::value_type* data,
                                        in_buf_t::buf_t::size_type size,
                                        parakeet_buf_t& parakeet_buf);
    parakeet_full_params make_wparams();
    void reset_impl() override;
    void stop_processing_impl() override;
    void start_processing_impl() override;
    bool use_gpu() const;
    void set_visible_devices();
    bool load_backend(const std::string& name);
    void unload_all_backends();
};

#endif  // PARAKEET_ENGINE_H

/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DS_ENGINE_H
#define DS_ENGINE_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "stt_engine.hpp"
#include "text_tools.hpp"

typedef struct TokenMetadata {
    /** The text corresponding to this token */
    const char* const text;

    /** Position of the token in units of 20ms */
    const unsigned int timestep;

    /** Position of the token in seconds */
    const float start_time;
} TokenMetadata;

/**
 * @brief A single transcript computed by the model, including a confidence
 *        value and the metadata for its constituent tokens.
 */
typedef struct CandidateTranscript {
    /** Array of TokenMetadata objects */
    const TokenMetadata* const tokens;
    /** Size of the tokens array */
    const unsigned int num_tokens;
    /** Approximated confidence value for this transcript. This is roughly the
     * sum of the acoustic model logit values for each timestep/character that
     * contributed to the creation of this transcript.
     */
    const double confidence;

} CandidateTranscript;

/**
 * @brief  An structure to contain emissions (the softmax output of individual
 *         timesteps) from the acoustic model.
 *
 * @member The layout of the emissions member is time major, thus to access the
 *         probability of symbol j at timestep i you would use
 *         emissions[i * num_symbols + j]
 */
typedef struct AcousticModelEmissions {
    /** number of symbols in the alphabet, including CTC blank */
    int num_symbols;
    /** num_symbols long array of NUL-terminated strings */
    const char** symbols;
    /** total number of timesteps */
    int num_timesteps;
    /** num_timesteps long array, each pointer is a num_symbols long array */
    const double* emissions;
} AcousticModelEmissions;

/**
 * @brief An array of CandidateTranscript objects computed by the model.
 */
typedef struct Metadata {
    /** Array of CandidateTranscript objects */
    const CandidateTranscript* const transcripts;
    /** Size of the transcripts array */
    const unsigned int num_transcripts;
    /** Logits and information to decode them **/
    const AcousticModelEmissions* const emissions;
} Metadata;

struct ModelState;
struct StreamingState;

class ds_engine : public stt_engine {
   public:
    ds_engine(config_t config, callbacks_t call_backs);
    ~ds_engine() override;

   private:
    using ds_buf_t = std::vector<int16_t>;

    struct ds_api {
        int (*STT_CreateModel)(const char* aModelPath,
                               ModelState** retval) = nullptr;
        void (*STT_FreeModel)(ModelState* ctx) = nullptr;
        int (*STT_EnableExternalScorer)(ModelState* aCtx,
                                        const char* aScorerPath) = nullptr;
        int (*STT_CreateStream)(ModelState* aCtx,
                                StreamingState** retval) = nullptr;
        void (*STT_FreeStream)(StreamingState* aSctx) = nullptr;
        char* (*STT_FinishStream)(StreamingState* aSctx) = nullptr;
        Metadata* (*STT_FinishStreamWithMetadata)(
            StreamingState* aSctx, unsigned int aNumResults) = nullptr;
        char* (*STT_IntermediateDecode)(const StreamingState* aSctx) = nullptr;
        void (*STT_FeedAudioContent)(StreamingState* aSctx,
                                     const short* aBuffer,
                                     unsigned int aBufferSize) = nullptr;
        void (*STT_FreeString)(char* str) = nullptr;
        void (*STT_FreeMetadata)(Metadata* m) = nullptr;

        inline auto ok() const {
            return STT_CreateModel && STT_FreeModel &&
                   STT_EnableExternalScorer && STT_CreateStream &&
                   STT_FreeStream && STT_FinishStream &&
                   STT_FinishStreamWithMetadata && STT_IntermediateDecode &&
                   STT_FeedAudioContent && STT_FreeString && STT_FreeMetadata;
        }
    };

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s

    ds_buf_t m_speech_buf;
    ds_api m_ds_api;
    void* m_dslib_handle = nullptr;
    ModelState* m_ds_model = nullptr;
    StreamingState* m_ds_stream = nullptr;
    size_t m_decoded_samples = 0;
    size_t m_decoding_duration = 0;

    void open_ds_lib();
    void create_ds_model();
    void create_ds_stream();
    void free_ds_stream();
    samples_process_result_t process_buff() override;
    void decode_speech(const ds_buf_t& buf, bool eof);
    void reset_impl() override;
    void start_processing_impl() override;
    std::pair<std::string, std::vector<text_tools::segment_t>>
    segments_from_meta(const Metadata* meta);
};

#endif  // DS_ENGINE_H

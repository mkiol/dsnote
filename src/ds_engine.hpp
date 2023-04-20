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

#include "punctuator.hpp"
#include "stt_engine.hpp"

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
        char* (*STT_IntermediateDecode)(const StreamingState* aSctx) = nullptr;
        void (*STT_FeedAudioContent)(StreamingState* aSctx,
                                     const short* aBuffer,
                                     unsigned int aBufferSize) = nullptr;
        void (*STT_FreeString)(char* str) = nullptr;

        inline auto ok() const {
            return STT_CreateModel && STT_FreeModel &&
                   STT_EnableExternalScorer && STT_CreateStream &&
                   STT_FreeStream && STT_FinishStream &&
                   STT_IntermediateDecode && STT_FeedAudioContent &&
                   STT_FreeString;
        }
    };

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s

    ds_buf_t m_speech_buf;
    ds_api m_ds_api;
    void* m_dslib_handle = nullptr;
    ModelState* m_ds_model = nullptr;
    StreamingState* m_ds_stream = nullptr;
    std::optional<punctuator> m_punctuator;

    void open_ds_lib();
    void create_ds_model();
    void create_ds_stream();
    void free_ds_stream();
    void create_punctuator();
    samples_process_result_t process_buff() override;
    void decode_speech(const ds_buf_t& buf, bool eof);
    void reset_impl() override;
    void start_processing_impl() override;
};

#endif  // DS_ENGINE_H

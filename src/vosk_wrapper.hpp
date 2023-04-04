/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef VOSK_SPEECH_WRAPPER_H
#define VOSK_SPEECH_WRAPPER_H

#include <memory>
#include <string>
#include <vector>

#include "engine_wrapper.hpp"

struct VoskModel;
struct VoskRecognizer;

class vosk_wrapper : public engine_wrapper {
   public:
    vosk_wrapper(config_t config, callbacks_t call_backs);
    ~vosk_wrapper() override;

   private:
    using vosk_buf_t = std::vector<int16_t>;

    struct vosk_api {
        VoskModel* (*vosk_model_new)(const char* model_path) = nullptr;
        void (*vosk_model_free)(VoskModel* model) = nullptr;
        VoskRecognizer* (*vosk_recognizer_new)(VoskModel* model,
                                               float sample_rate) = nullptr;
        void (*vosk_recognizer_reset)(VoskRecognizer* recognizer) = nullptr;
        void (*vosk_recognizer_free)(VoskRecognizer* recognizer) = nullptr;
        int (*vosk_recognizer_accept_waveform_s)(VoskRecognizer* recognizer,
                                                 const short* data,
                                                 int length) = nullptr;
        const char* (*vosk_recognizer_partial_result)(
            VoskRecognizer* recognizer) = nullptr;
        const char* (*vosk_recognizer_final_result)(
            VoskRecognizer* recognizer) = nullptr;

        inline auto ok() const {
            return vosk_model_new && vosk_model_free && vosk_recognizer_new &&
                   vosk_recognizer_reset && vosk_recognizer_free &&
                   vosk_recognizer_accept_waveform_s &&
                   vosk_recognizer_partial_result &&
                   vosk_recognizer_final_result;
        }
    };

    inline static const size_t m_speech_max_size = m_sample_rate * 60;  // 60s

    vosk_buf_t m_speech_buf;
    vosk_api m_vosk_api;
    void* m_vosklib_handle = nullptr;
    VoskModel* m_vosk_model = nullptr;
    VoskRecognizer* m_vosk_recognizer = nullptr;

    void open_vosk_lib();
    void create_vosk_model();
    samples_process_result_t process_buff() override;
    void decode_speech(const vosk_buf_t& buf, bool eof);
    void reset_impl() override;
    void start_processing_impl() override;
    void push_inbuf_to_samples();
};

#endif  // VOSK_SPEECH_WRAPPER_H

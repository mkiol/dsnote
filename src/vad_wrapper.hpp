/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef VAD_WRAPPER_H
#define VAD_WRAPPER_H

#include <webrtc_vad.h>

#include <cstddef>
#include <optional>
#include <vector>

class vad_wrapper {
   public:
    using buf_t = std::vector<int16_t>;

    struct voice_active_result {
        std::optional<size_t> start;
        std::optional<size_t> stop;
    };

    vad_wrapper();
    ~vad_wrapper();
    void reset();
    void restart();
    const buf_t& remove_silence(const buf_t::value_type* frame, size_t frame_size);
    bool is_speech(const buf_t::value_type* frame, size_t frame_size);

   private:
    inline static const size_t m_chunk_size = 480;
    inline static const size_t m_dup_max_size = 10 * m_chunk_size;
    inline static const size_t m_chunks_in_frame = 25;

    VadInst* m_handle = nullptr;
    int m_mode = 3;
    int m_fs = 16000;
    buf_t m_input_samples;
    buf_t m_output_samples;
    size_t m_dup_size = 0;

    std::vector<bool> vad_process(const buf_t& samples) const;
    static void shift_left(std::vector<int16_t>& vec, size_t distance);
};

#endif  // VAD_WRAPPER_H

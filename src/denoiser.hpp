/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DENOISER_HPP
#define DENOISER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct DenoiseState;

class denoiser {
   public:
    enum task_flags : unsigned int {
        task_none = 0U,
        task_denoise = 1U << 0U,
        task_denoise_hard = 1U << 1U,
        task_normalize = 1U << 2U,
        task_normalize_two_pass = 1U << 3U,
        task_probs = 1U << 4U
    };

    inline static const size_t frame_size = 480;
    using sample_t = int16_t;

    explicit denoiser(int sample_rate, unsigned int tasks,
                      uint64_t full_size = 0);
    ~denoiser();
    void process(sample_t* buf, size_t size);
    void process_char(char* buf, size_t size);
    void normalize_second_pass(sample_t* buf, size_t size);
    void normalize_second_pass_char(char* buf, size_t size);
    std::vector<float> speech_probs();

   private:
    using frame_t = std::array<float, frame_size>;

    inline static const float m_prob_seg_threshold = 0.1;
    inline static const float m_prob_seg_count = 100;
    float m_prob_acum = 0.0;

    unsigned int m_prob_seg_num = 0;
    unsigned int m_prob_seg_size = 0;

    DenoiseState* m_state = nullptr;
    std::vector<float> m_speech_probs;
    unsigned int m_task_flags = task_flags::task_none;
    uint64_t m_full_size = 0;
    int m_normalize_peek = 1;

    void normalize_audio(sample_t* audio, size_t size, bool second_pass);
};

#endif // DENOISER_HPP

/* Copyright (C) 2024-2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "denoiser.hpp"

extern "C" {
#include <rnnoise.h>
}

#include <algorithm>
#include <cmath>
#include <stdexcept>

denoiser::denoiser(int sample_rate, unsigned int tasks, uint64_t full_size)
    : m_sample_rate{sample_rate}, m_task_flags{tasks}, m_full_size{full_size} {
    if ((m_task_flags & task_denoise) || (m_task_flags & task_denoise_hard) ||
        (m_task_flags & task_probs)) {
        m_state = rnnoise_create(nullptr);
        if (m_state == nullptr)
            throw std::runtime_error("failed to create rnnoise");
    }

    m_prob_seg_size =
        m_full_size > 0 ? m_full_size / frame_size / m_prob_seg_count : 0;
}

denoiser::~denoiser() {
    if (m_state) rnnoise_destroy(m_state);
    m_state = nullptr;
}

std::vector<float> denoiser::speech_probs() {
    if (m_prob_seg_num < m_prob_seg_size && m_prob_seg_size > 0) {
        auto av_probe = m_prob_acum / m_prob_seg_size;
        if (av_probe < m_prob_seg_threshold) av_probe = 0.0;
        m_prob_seg_num = 0;
        m_prob_acum = 0;
        m_speech_probs.push_back(av_probe);
    }

    return m_speech_probs;
}

// TO-DO: refactor with simd
void denoiser::normalize_audio(sample_t* __restrict__ audio, size_t size,
                               bool second_pass) {
    constexpr int max = std::numeric_limits<sample_t>::max();
    constexpr int min = std::numeric_limits<sample_t>::min();
    constexpr int target_gain = max * 3 / 4;

    const bool peek_pass =
        !second_pass &&
        (m_task_flags & (task_normalize | task_normalize_two_pass)) != 0;
    const bool gain_pass = (m_task_flags & task_normalize) != 0 || second_pass;

    int peek = m_normalize_peek;

    if (peek_pass) {
        int local_peek = 0;
        for (size_t i = 0; i < size; ++i) {
            int v = std::abs(static_cast<int>(audio[i]));
            local_peek = v > local_peek ? v : local_peek;
        }
        peek = std::max(peek, local_peek);
        m_normalize_peek = peek;
    }

    if (gain_pass && peek > 0) {
        int gain = (1 << 10) * target_gain / peek;
        gain = std::clamp(gain, 1 << 10, 32 << 10);
        if ((peek * gain >> 10) > max) gain = (max << 10) / peek;

        for (size_t i = 0; i < size; ++i) {
            int v = (audio[i] * gain) >> 10;
            audio[i] =
                static_cast<sample_t>(v < min ? min : (v > max ? max : v));
        }

        if (m_task_flags & task_normalize) {
            m_normalize_peek = 1;
            peek = 1;
        }
    }
}

void denoiser::normalize_second_pass_char(char* buf, size_t size) {
    normalize_second_pass(reinterpret_cast<sample_t*>(buf),
                          size / sizeof(sample_t));
}

void denoiser::normalize_second_pass(sample_t* buf, size_t size) {
    normalize_audio(buf, size, true);
}

void denoiser::process_char(char* buf, size_t size) {
    denoiser::process(reinterpret_cast<sample_t*>(buf),
                      size / sizeof(sample_t));
}

size_t denoiser::size_after_resample(size_t src_len, int src_rate,
                                     int dst_rate) {
    return static_cast<size_t>(
        std::llround(static_cast<double>(src_len) * dst_rate / src_rate));
}

size_t denoiser::resample(const sample_t* src, size_t src_len, int src_rate,
                          int dst_rate, sample_t* dst) {
    auto dst_size = size_after_resample(src_len, src_rate, dst_rate);
    double src_step = static_cast<double>(src_rate) / dst_rate;

    for (size_t i = 0; i < dst_size; ++i) {
        auto pos = i * src_step;
        auto k = static_cast<size_t>(pos);
        double frac = pos - static_cast<double>(k);
        int32_t s0 = src[k];
        int32_t s1 = (k + 1 < src_len) ? src[k + 1] : src[k];
        dst[i] = static_cast<int16_t>(std::lrint(s0 + ((s1 - s0) * frac)));
    }

    return dst_size;
}

void denoiser::process(sample_t* buf, size_t size) {
    if ((m_task_flags & task_denoise) || (m_task_flags & task_denoise_hard) ||
        (m_task_flags & task_probs)) {
        // resample to 48000
        auto size48 = size_after_resample(size, m_sample_rate, rnn_sample_rate);
        std::vector<int16_t> buf48(size48);
        resample(buf, size, m_sample_rate, rnn_sample_rate, buf48.data());

        frame_t frame;

        auto* cur = buf48.data();
        const auto* end = buf48.data() + size48;

        while (cur < end) {
            size_t samples = end - cur;

            if (samples >= frame.size()) {
                for (size_t i = 0; i < frame.size(); ++i) frame[i] = cur[i];
                samples = frame.size();
            } else {
                for (size_t i = 0; i < samples; ++i) frame[i] = cur[i];
                for (size_t i = 0; i < frame.size() - samples; ++i)
                    frame[i] = 0.0;
            }

            auto prob =
                rnnoise_process_frame(m_state, frame.data(), frame.data());

            if ((m_task_flags & task_probs) && m_prob_seg_size > 0) {
                m_prob_acum += prob;
                if (++m_prob_seg_num >= m_prob_seg_size) {
                    auto av_probe = m_prob_acum / m_prob_seg_size;
                    if (av_probe < m_prob_seg_threshold) av_probe = 0.0;
                    m_speech_probs.push_back(av_probe);
                    m_prob_acum = 0.0;
                    m_prob_seg_num = 0;
                }
            }

            if (m_task_flags & task_denoise_hard) {
                if (prob < 0.1)
                    for (size_t i = 0; i < samples; ++i) cur[i] = 0;
                else
                    for (size_t i = 0; i < samples; ++i) cur[i] = frame[i];
            } else if (m_task_flags & task_denoise) {
                for (size_t i = 0; i < samples; ++i) cur[i] = frame[i];
            }

            cur += samples;
        }

        resample(buf48.data(), size48, rnn_sample_rate, m_sample_rate, buf);
    }

    if (m_task_flags & task_normalize ||
        m_task_flags & task_normalize_two_pass) {
        normalize_audio(buf, size, false);
    }
}

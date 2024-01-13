/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "denoiser.hpp"

extern "C" {
#include <rnnoise-nu.h>
}

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "logger.hpp"

denoiser::denoiser(int sample_rate, int tasks, uint64_t full_size)
    : m_task_flags{tasks}, m_full_size{full_size} {
    if ((m_task_flags & task_denoise) || (m_task_flags & task_probs)) {
        auto* model = rnnoise_get_model("orig");
        if (model == nullptr) LOGE("rnnoise model not found");

        m_state = rnnoise_create(model);
        if (m_state == nullptr)
            throw std::runtime_error("failed to create rnnoise");

        rnnoise_set_param(m_state, RNNOISE_PARAM_MAX_ATTENUATION, 10.0);
        rnnoise_set_param(m_state, RNNOISE_PARAM_SAMPLE_RATE, sample_rate);
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
void denoiser::normalize_audio(sample_t* audio, size_t size, bool second_pass) {
    int max = std::numeric_limits<sample_t>::max();
    int min = std::numeric_limits<sample_t>::min();
    int target_gain = max * 0.75;

    if (m_task_flags & task_normalize ||
        m_task_flags & task_normalize_two_pass) {
        for (size_t i = 0; i < size; ++i) {
            int val = std::abs(audio[i]);
            if (val > m_normalize_peek) m_normalize_peek = val;
        }
    }

    if (m_task_flags & task_normalize || second_pass) {
        auto new_gain = [=]() {
            int max_ampl = 32;

            int new_gain = (1 << 10) * target_gain / m_normalize_peek;
            if (new_gain > (max_ampl << 10)) new_gain = max_ampl << 10;
            if (new_gain < (1 << 10)) new_gain = 1 << 10;
            if ((m_normalize_peek * new_gain >> 10) > max)
                new_gain = (max << 10) / m_normalize_peek;

            return new_gain;
        }();

        for (size_t i = 0; i < size; ++i)
            audio[i] = static_cast<sample_t>(
                std::clamp(audio[i] * new_gain >> 10, min, max));

        if (m_task_flags & task_normalize) m_normalize_peek = 1;
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

void denoiser::process(sample_t* buf, size_t size) {
    if ((m_task_flags & task_denoise) || (m_task_flags & task_probs)) {
        frame_t frame;

        auto* cur = buf;
        const auto* end = buf + size;

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

            if (m_task_flags & task_denoise) {
                for (size_t i = 0; i < samples; ++i) cur[i] = frame[i];
            }

            cur += samples;
        }
    }

    if (m_task_flags & task_normalize || m_task_flags & task_normalize_two_pass)
        normalize_audio(buf, size, false);
}

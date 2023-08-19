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

denoiser::denoiser(int sample_rate) {
    auto* model = rnnoise_get_model("orig");
    if (model == nullptr) LOGE("rnnoise model not found");

    m_state = rnnoise_create(model);
    if (m_state == nullptr)
        throw std::runtime_error("failed to create rnnoise");

    rnnoise_set_param(m_state, RNNOISE_PARAM_MAX_ATTENUATION, 10.0);
    rnnoise_set_param(m_state, RNNOISE_PARAM_SAMPLE_RATE, sample_rate);
}

denoiser::~denoiser() {
    if (m_state) rnnoise_destroy(m_state);
    m_state = nullptr;
}

void denoiser::normalize_audio(sample_t* audio, size_t size) {
    // inspired by https://github.com/fluffy-critter/AudioCompress

    int max = std::numeric_limits<sample_t>::max();
    int min = std::numeric_limits<sample_t>::min();
    int target_gain = max * 0.75;

    auto peak = [=] {
        int peak = 1;

        for (size_t i = 0; i < size; ++i) {
            int val = audio[i];
            if (val < 0) val = -val;
            if (val > peak) peak = val;
        }

        return peak;
    }();

    auto new_gain = [=]() {
        int max_ampl = 32;

        int new_gain = (1 << 10) * target_gain / peak;
        if (new_gain > (max_ampl << 10)) new_gain = max_ampl << 10;
        if (new_gain < (1 << 10)) new_gain = 1 << 10;
        if ((peak * new_gain >> 10) > max) new_gain = (max << 10) / peak;

        return new_gain;
    }();

    for (size_t i = 0; i < size; ++i)
        audio[i] = static_cast<sample_t>(
            std::clamp(audio[i] * new_gain >> 10, min, max));
}

void denoiser::process(sample_t* buf, size_t size) {
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
            for (size_t i = 0; i < frame.size() - samples; ++i) frame[i] = 0.0;
        }

        auto prob = rnnoise_process_frame(m_state, frame.data(), frame.data());

        LOGT("prob: " << prob);

        if (prob < 0.1)
            for (size_t i = 0; i < samples; ++i) cur[i] = 0;
        else
            for (size_t i = 0; i < samples; ++i) cur[i] = frame[i];

        cur += samples;
    }

    normalize_audio(buf, size);
}

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "vad_wrapper.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "logger.hpp"

vad_wrapper::vad_wrapper() { restart(); }

void vad_wrapper::restart() {
    if (m_handle) WebRtcVad_Free(m_handle);

    m_handle = WebRtcVad_Create();

    if (m_handle == nullptr) throw std::runtime_error("vad create error");

    if (WebRtcVad_Init(m_handle) != 0)
        throw std::runtime_error("vad init error");

    if (WebRtcVad_set_mode(m_handle, m_mode) != 0)
        throw std::runtime_error("set mode error");

    reset();
}

void vad_wrapper::reset() {
    m_output_samples.clear();
    m_input_samples.clear();
    m_dup_size = 0;
    m_end_adjacent = false;
}

vad_wrapper::~vad_wrapper() { WebRtcVad_Free(m_handle); }

void vad_wrapper::shift_left(std::vector<int16_t>& vec, size_t distance) {
    if (distance >= vec.size()) {
        vec.clear();
        return;
    }

    auto beg = vec.cbegin();
    std::advance(beg, distance);

    std::copy(beg, vec.cend(), vec.begin());

    vec.resize(vec.size() - distance);
}

[[maybe_unused]] static void log_vec(const std::vector<bool>& vec) {
    std::ostringstream os;
    std::for_each(vec.cbegin(), vec.cend(), [&os](auto value) { os << value; });
    LOGD("results: " << os.str());
}

static void insert_to_vec(const std::vector<int16_t>& input, size_t start,
                          size_t stop, std::vector<int16_t>& output) {
    auto beg = input.begin();
    std::advance(beg, start);

    auto end = input.begin();
    std::advance(end, stop);

    output.insert(output.end(), beg, end);
}

std::vector<bool> vad_wrapper::vad_process(const buf_t& samples) const {
    const auto chunks = samples.size() / m_chunk_size;

    std::vector<bool> results;
    results.reserve(chunks);

    for (size_t chunk = 0; chunk < chunks; ++chunk) {
        auto result = WebRtcVad_Process(m_handle, m_fs,
                                        samples.data() + (chunk * m_chunk_size),
                                        m_chunk_size);

        if (result < 0) throw std::runtime_error("process error");

        results.push_back(result == 1);
    }

    return results;
}

const vad_wrapper::buf_t& vad_wrapper::process(const buf_t::value_type* frame,
                                               size_t frame_size) {
    m_output_samples.clear();

    auto old_end_adjacent = m_end_adjacent;

    for (size_t i = 0; i < frame_size; ++i) m_input_samples.push_back(frame[i]);

    if (m_input_samples.size() < m_chunk_size) return m_output_samples;

    auto vad_results = vad_process(m_input_samples);

    if (vad_results.size() < m_chunks_in_frame) return m_output_samples;

    std::optional<size_t> cut_start;
    std::optional<size_t> cut_stop;

    for (size_t chunk = 0; chunk <= (vad_results.size() - m_chunks_in_frame);
         ++chunk) {
        auto beg = vad_results.cbegin();
        std::advance(beg, chunk);

        auto end = beg;
        std::advance(end, m_chunks_in_frame);

        auto acc = std::accumulate(beg, end, static_cast<size_t>(0),
                                   [](size_t sum, bool value) {
                                       if (value) ++sum;
                                       return sum;
                                   });

        auto vad_active = 2 * acc > m_chunks_in_frame;

        if (vad_active && !cut_start) {
            auto pos = std::min(chunk * m_chunk_size, m_input_samples.size());
            LOGT("cut start: pos=" << pos << ", m_dup_size=" << m_dup_size);

            if (pos == 0) {
                pos = std::min(pos + m_dup_size, m_input_samples.size());
                m_dup_size = 0;
            }

            cut_start.emplace(pos);

            LOGT("cut start: " << *cut_start << ", chunk=" << chunk
                               << ", m_chunk_size=" << m_chunk_size);
        }

        if (!vad_active && cut_start) {
            auto pos = std::min((chunk + m_chunks_in_frame) * m_chunk_size,
                                m_input_samples.size());
            LOGT("cut stop: pos=" << pos);

            if (pos > *cut_start) {
                cut_stop.emplace(pos);

                LOGT("cut stop: " << *cut_stop << ", chunk=" << chunk
                                  << ", m_chunk_size=" << m_chunk_size);

                insert_to_vec(m_input_samples, *cut_start, *cut_stop,
                              m_output_samples);

                cut_start.reset();
                chunk += m_chunks_in_frame;
            }
        }
    }

    m_dup_size = 0;

    auto dup_size = m_input_samples.size() <= m_dup_max_size
                        ? m_dup_max_size - m_input_samples.size()
                        : m_dup_max_size;

    if (cut_start) {
        cut_stop.emplace(m_input_samples.size());
        m_end_adjacent = true;

        LOGT("cut stop: " << *cut_stop);

        insert_to_vec(m_input_samples, *cut_start, *cut_stop, m_output_samples);
    } else {
        m_end_adjacent = false;
    }

    if (cut_stop) {
        auto not_cut = m_input_samples.size() - *cut_stop;
        if (not_cut < dup_size) m_dup_size = dup_size - not_cut;
    }

    LOGT("dup size=" << dup_size << ", m_size=" << m_dup_size);

    shift_left(m_input_samples, m_input_samples.size() - dup_size);

    if (m_output_samples.empty() && old_end_adjacent) {
        LOGD("adding extra samples because of adjacent end");

        auto end = m_input_samples.cbegin();
        std::advance(end, std::min(2 * m_chunk_size, m_input_samples.size()));
        m_output_samples.insert(m_output_samples.begin(),
                                m_input_samples.cbegin(), end);
    }

    return m_output_samples;
}

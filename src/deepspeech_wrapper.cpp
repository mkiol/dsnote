/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "deepspeech_wrapper.h"

#include <QDebug>

#include <algorithm>
#include <numeric>
#include <fstream>
#include <cmath>
#include <chrono>
#include <cstring>

using namespace std::chrono_literals;

deepspeech_wrapper::deepspeech_wrapper(const std::string& model_file,
                                       const std::string& scorer_file,
                                       const callbacks_type& call_backs,
                                       speech_mode_type speech_mode) :
    call_backs{call_backs},
    processing_thread{&deepspeech_wrapper::start_processing, this},
    speech_mode_value{speech_mode}
{
    create_model(model_file, scorer_file);
}

deepspeech_wrapper::~deepspeech_wrapper()
{
    {
        std::lock_guard lock{processing_mtx};
        thread_exit_requested = true;
    }
    processing_cv.notify_all();
    processing_thread.join();
    speech_detected_value = false;
    call_backs.speech_status_changed(false);
}

void deepspeech_wrapper::start_processing()
{
    qDebug() << "processing thread started";

    thread_exit_requested = false;

    while (true) {
        std::unique_lock lock{processing_mtx};
        if (thread_exit_requested) break;
        if (restart_requested) {
            restart_requested = false;
            flush(true);
        }
        if (!process_buff()) processing_cv.wait(lock);
    }

    flush();

    qDebug() << "processing thread ended";
}

std::string deepspeech_wrapper::error_msg(int status)
{
    auto error_mgs_raw = DS_ErrorCodeToErrorMessage(status);
    std::string error_mgs{error_mgs_raw};
    delete error_mgs_raw;
    return error_mgs;
}

bool deepspeech_wrapper::ok() const
{
    return model ? true : false;
}

void deepspeech_wrapper::create_model(const std::string& model_file, const std::string& scorer_file)
{
    ModelState* state;
    int status = DS_CreateModel(model_file.c_str(), &state);

    if (status == 0) {
        model = model_ptr{state, [](ModelState* state){
            DS_FreeModel(state);
        }};
        if (!scorer_file.empty()) {
            DS_EnableExternalScorer(model.get(), scorer_file.c_str());
        }
    } else {
        qDebug() << "could not create model:" << QString::fromStdString(error_msg(status));
    }
}

void deepspeech_wrapper::create_stream()
{
    int status = DS_CreateStream(model.get(), &stream);

    if (status != 0) {
        qDebug() << "could not create stream:" << QString::fromStdString(error_msg(status));
        free_stream();
    }
}

void deepspeech_wrapper::free_stream()
{
    if (stream) {
        DS_FreeStream(stream);
        stream = nullptr;
    }
}

int deepspeech_wrapper::sample_rate() const
{
    return DS_GetModelSampleRate(model.get());
}

unsigned int deepspeech_wrapper::accumulate_abs(buff_type::const_iterator begin,
                                                buff_type::const_iterator end) const {
    unsigned int sum = 0;

    std::for_each(begin, end, [&](const auto& v) {
        sum += std::abs(v);
    });

    return sum;
}

bool deepspeech_wrapper::lock_buff(lock_type desired_lock)
{
    lock_type expected_lock = lock_type::free;
    return buff_struct.lock.compare_exchange_strong(expected_lock, desired_lock);
}

void deepspeech_wrapper::free_buff(lock_type lock)
{
    lock_type expected_lock = lock;
    buff_struct.lock.compare_exchange_strong(expected_lock, lock_type::free);
}

void deepspeech_wrapper::free_buff()
{
    buff_struct.lock.store(lock_type::free);
}

std::pair<char*, int64_t> deepspeech_wrapper::borrow_buff()
{
    std::pair<char*, int64_t> c_buf{nullptr, 0};

    if (!lock_buff(lock_type::borrowed))
        return c_buf;

    if (buff_struct.full()) {
        //qWarning() << "cannot borrow, buff is full";
        free_buff();
        return c_buf;
    }

    c_buf.first = reinterpret_cast<char*>(&buff_struct.buff.at(buff_struct.size));
    c_buf.second = (buff_struct.buff.size() - buff_struct.size) * sizeof (buff_type::value_type);

    return c_buf;
}

void deepspeech_wrapper::return_buff(char* c_buff, int64_t size)
{
    if (buff_struct.lock != lock_type::borrowed) {
        //qWarning() << "cannot return, buff not borrowed";
        return;
    }

    buff_struct.size = (c_buff - reinterpret_cast<char*>(&buff_struct.buff.at(0)) + size) /
            sizeof (buff_type::value_type);

    free_buff();
    processing_cv.notify_one();
}

bool deepspeech_wrapper::lock_buff_for_processing()
{
    if (!lock_buff(lock_type::processed))
        return false;

    if (buff_struct.size < frame_size) {
        free_buff();
        return false;
    }

    return true;
}

bool deepspeech_wrapper::process_buff()
{
    if (!lock_buff_for_processing())
        return false;

    if (speech_mode_value == speech_mode_type::manual) {
        if (last_frame_done) {
            flush();
            buff_struct.size = 0;
            free_buff();
            return false;
        } else if (!speech_detected_value) {
            last_frame_done = true;
        }
    }

    //qDebug() << "process_buff start:" << buff_struct.size;

    auto begin = buff_struct.buff.begin(), end = begin + frame_size;
    auto max_end = begin + buff_struct.size;

    do {
        process_buff(begin, end);
        begin += frame_size;
        end += frame_size;
    } while (end <= max_end);

    trim_buff(end - frame_size);

    //qDebug() << "process_buff end";

    free_buff();

    if (speech_mode_value == speech_mode_type::manual)
        return !last_frame_done;
    return true;
}

void deepspeech_wrapper::process_buff(buff_type::const_iterator begin,
                                      buff_type::const_iterator end)
{
    if (!stream)
        create_stream();

    DS_FeedAudioContent(stream, &(*begin), std::distance(begin, end));

    auto result = DS_IntermediateDecode(stream);

    if (speech_mode_value == speech_mode_type::automatic &&
            intermediate_text && intermediate_text == result)
        frames_without_change++;

    if (frames_without_change < silent_level) {
        //qDebug("result: %d %s %s", frames_without_change, intermediate_text ? intermediate_text->c_str() : "null", result);
        if (speech_mode_value == speech_mode_type::automatic) {
            if (intermediate_text || std::strlen(result) != 0)
                set_intermediate_text(result);
            if (intermediate_text && !intermediate_text->empty())
                set_speech_detected(true);
        } else {
            set_intermediate_text(result);
        }
    } else {
        //qDebug("flush: %d %s %s", frames_without_change, intermediate_text ? intermediate_text->c_str() : "null", result);
        flush();
        if (speech_mode_value == speech_mode_type::automatic)
            set_speech_detected(false);
    }

    DS_FreeString(result);
}

void deepspeech_wrapper::set_intermediate_text(const char* text)
{
    if (intermediate_text != text) {
        intermediate_text = text;
        if (intermediate_text->size() >= min_text_size)
            call_backs.intermediate_text_decoded(intermediate_text.value());
    }
}

void deepspeech_wrapper::trim_buff(buff_type::const_iterator begin)
{
    buff_struct.size -= std::distance(buff_struct.buff.cbegin(), begin);
    std::copy(begin, begin + buff_struct.size, buff_struct.buff.begin());
}

void deepspeech_wrapper::flush(bool clear_buff)
{
    free_stream();

    frames_without_change = 0;

    set_speech_detected(false);

    if (clear_buff) {
        if (lock_buff_for_processing())
            buff_struct.size = 0;
    }

    if (intermediate_text && !intermediate_text->empty()) {
        if (intermediate_text->size() >= min_text_size)
            call_backs.text_decoded(intermediate_text.value());
        set_intermediate_text("");
    }

    intermediate_text.reset();
}

void deepspeech_wrapper::set_speech_status(bool started)
{
    if (speech_mode_value == speech_mode_type::manual) {
        set_speech_detected(started);
    } else {
        qWarning() << "speech status can be changed only in manual mode";
    }
}

void deepspeech_wrapper::set_speech_detected(bool detected) {
    if (speech_detected_value != detected) {
        last_frame_done = false;
        speech_detected_value = detected;
        call_backs.speech_status_changed(speech_detected_value);
    }
}

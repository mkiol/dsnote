/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "media_compressor.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include "logger.hpp"
#include "text_tools.hpp"

extern "C" {
#include <libavutil/error.h>
}

[[maybe_unused]] static std::ostream& operator<<(
    std::ostream& os, const AVChannelLayout* layout) {
    if (layout == nullptr) {
        os << "null";
    } else {
        std::array<char, 512> s{};
        av_channel_layout_describe(layout, s.data(), s.size());
        os << s.data();
    }
    return os;
}

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os,
                                                 AVRational r) {
    os << r.num << "/" << r.den;
    return os;
}

static bool time_base_equal(AVRational l, AVRational r) {
    return l.den == r.den && l.num == r.num;
}

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os,
                                                 AVSampleFormat fmt) {
    const auto* name = av_get_sample_fmt_name(fmt);
    if (name == nullptr)
        os << "unknown";
    else
        os << name;
    return os;
}

[[maybe_unused]] static auto codec_name(AVCodecID codec) {
    const auto* desc = avcodec_descriptor_get(codec);
    if (desc == nullptr) return "unknown";
    return desc->name;
}

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os,
                                                 AVCodecID codec) {
    os << codec_name(codec);
    return os;
}

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os,
                                                 const AVPacket* pkt) {
    os << "pts=" << pkt->pts << ", dts=" << pkt->dts
       << ", duration=" << pkt->duration << ", pos=" << pkt->pos
       << ", sidx=" << pkt->stream_index << ", tb=" << pkt->time_base
       << ", size=" << pkt->size;
    return os;
}

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os,
                                                 AVMediaType media_type) {
    switch (media_type) {
        case AVMEDIA_TYPE_VIDEO:
            os << "video";
            break;
        case AVMEDIA_TYPE_AUDIO:
            os << "audio";
            break;
        case AVMEDIA_TYPE_DATA:
            os << "data";
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            os << "subtitle";
            break;
        case AVMEDIA_TYPE_ATTACHMENT:
            os << "attachment";
            break;
        case AVMEDIA_TYPE_NB:
            os << "nb";
            break;
        case AVMEDIA_TYPE_UNKNOWN:
            os << "unknown";
            break;
    }

    return os;
}

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os,
                                                 const AVDictionary* dict) {
    if (dict) {
        AVDictionaryEntry* t = nullptr;
        while ((t = av_dict_get(dict, "", t, AV_DICT_IGNORE_SUFFIX))) {
            os << "[" << t->key << "=" << t->value << "],";
        }
    }

    return os;
}

static char* value_from_av_dict(const char* key, const AVDictionary* dict) {
    if (dict) {
        AVDictionaryEntry* t = nullptr;
        while ((t = av_dict_get(dict, "", t, AV_DICT_IGNORE_SUFFIX))) {
            if (strcmp(key, t->key) == 0) return t->value;
        }
    }

    return nullptr;
}

static const char* str_from_av_error(int av_err) {
    static std::array<char, 1024> buf;

    return av_make_error_string(buf.data(), buf.size(), av_err);
}

static AVSampleFormat best_sample_format(const AVCodec* encoder,
                                         AVSampleFormat decoder_sample_fmt) {
    if (encoder->sample_fmts == nullptr)
        throw std::runtime_error(
            "audio encoder does not support any sample fmts");

    AVSampleFormat best_fmt = AV_SAMPLE_FMT_NONE;

    for (int i = 0; encoder->sample_fmts[i] != AV_SAMPLE_FMT_NONE; ++i) {
        best_fmt = encoder->sample_fmts[i];
        if (best_fmt == decoder_sample_fmt) {
            LOGD("sample fmt exact match");
            break;
        }
    }

    return best_fmt;
}

static int best_sample_rate(const AVCodec* encoder, int decoder_sample_rate) {
    if (encoder->supported_samplerates == nullptr) return decoder_sample_rate;

    int best_rate = 0;

    for (int i = 0; encoder->supported_samplerates[i] != 0; ++i) {
        if (best_rate == 0) best_rate = encoder->supported_samplerates[i];

        if (encoder->supported_samplerates[i] < decoder_sample_rate) break;

        best_rate = encoder->supported_samplerates[i];

        if (best_rate == decoder_sample_rate) {
            LOGD("sample rate exact match");
            break;
        }
    }

    return best_rate;
}

[[maybe_unused]] static void clean_av_opts(AVDictionary** opts) {
    if (*opts != nullptr) {
        LOGW("rejected av options: " << *opts);
        av_dict_free(opts);
    }
}

std::ostream& operator<<(std::ostream& os, media_compressor::format_t format) {
    switch (format) {
        case media_compressor::format_t::unknown:
            os << "unknown";
            break;
        case media_compressor::format_t::wav:
            os << "wav";
            break;
        case media_compressor::format_t::mp3:
            os << "mp3";
            break;
        case media_compressor::format_t::ogg_vorbis:
            os << "ogg-vorbis";
            break;
        case media_compressor::format_t::ogg_opus:
            os << "ogg-opus";
            break;
        case media_compressor::format_t::flac:
            os << "flac";
            break;
        case media_compressor::format_t::srt:
            os << "srt";
            break;
        case media_compressor::format_t::ass:
            os << "ass";
            break;
        case media_compressor::format_t::vtt:
            os << "vtt";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         media_compressor::quality_t quality) {
    switch (quality) {
        case media_compressor::quality_t::vbr_high:
            os << "vbr-high";
            break;
        case media_compressor::quality_t::vbr_medium:
            os << "vbr-medium";
            break;
        case media_compressor::quality_t::vbr_low:
            os << "vbr-low";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         media_compressor::media_type_t media_type) {
    switch (media_type) {
        case media_compressor::media_type_t::audio:
            os << "audio";
            break;
        case media_compressor::media_type_t::video:
            os << "video";
            break;
        case media_compressor::media_type_t::subtitles:
            os << "subtitles";
            break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, media_compressor::stream_t stream) {
    os << "index=" << stream.index << ", media-type=" << stream.media_type
       << ", title=" << stream.title << ", lang=" << stream.language;

    return os;
}

std::ostream& operator<<(std::ostream& os,
                         media_compressor::media_info_t media_info) {
    os << "audio=[";
    for (const auto& stream : media_info.audio_streams)
        os << "[" << stream << "], ";

    os << "], video=";
    for (const auto& stream : media_info.video_streams)
        os << "[" << stream << "], ";

    os << "], subtitles=";
    for (const auto& stream : media_info.subtitles_streams)
        os << "[" << stream << "], ";

    return os;
}

media_compressor::~media_compressor() {
    cancel();
    clean_av();
}

media_compressor::format_t media_compressor::format_from_filename(
    const std::string& filename) {
    auto idx = filename.find_last_of('.');

    if (idx == std::string::npos || idx == filename.size() - 1)
        return format_t::unknown;

    auto ext = filename.substr(idx + 1);
    text_tools::to_lower_case(ext);

    if (ext == "mp3") return format_t::mp3;
    if (ext == "ogg" || ext == "oga" || ext == "ogx")
        return format_t::ogg_vorbis;
    if (ext == "opus") return format_t::ogg_opus;
    if (ext == "flac") return format_t::flac;

    return format_t::unknown;
}

void media_compressor::cancel() {
    m_shutdown = true;

    m_cv.notify_all();

    if (m_async_thread.joinable()) m_async_thread.join();
}

void media_compressor::clean_av_in_format() {
    if (m_in_av_format_ctx) avformat_close_input(&m_in_av_format_ctx);
}

void media_compressor::clean_av() {
    if (m_av_filter_ctx.in != nullptr) avfilter_inout_free(&m_av_filter_ctx.in);
    if (m_av_filter_ctx.out != nullptr)
        avfilter_inout_free(&m_av_filter_ctx.out);
    if (m_av_filter_ctx.graph != nullptr)
        avfilter_graph_free(&m_av_filter_ctx.graph);

    if (m_av_fifo) {
        av_audio_fifo_free(m_av_fifo);
        m_av_fifo = nullptr;
    }

    if (m_out_av_format_ctx) {
        if (m_out_av_format_ctx->pb) {
            if (m_out_av_format_ctx->flags & AVFMT_FLAG_CUSTOM_IO) {
                if (m_out_av_format_ctx->pb) {
                    if (m_out_av_format_ctx->pb->buffer)
                        av_freep(&m_out_av_format_ctx->pb->buffer);
                    avio_context_free(&m_out_av_format_ctx->pb);
                }
            } else {
                avio_closep(&m_out_av_format_ctx->pb);
            }
        }
        avformat_free_context(m_out_av_format_ctx);
        m_out_av_format_ctx = nullptr;
    }
    if (m_out_av_ctx) avcodec_free_context(&m_out_av_ctx);

    if (m_in_av_ctx) avcodec_free_context(&m_in_av_ctx);

    clean_av_in_format();
}

void media_compressor::init_av_filter(const char* arg) {
    m_av_filter_ctx.in = avfilter_inout_alloc();
    m_av_filter_ctx.out = avfilter_inout_alloc();
    m_av_filter_ctx.graph = avfilter_graph_alloc();
    if (!m_av_filter_ctx.in || !m_av_filter_ctx.out || !m_av_filter_ctx.graph) {
        clean_av();
        throw std::runtime_error("failed to allocate av filter");
    }

    const auto* buffersrc = avfilter_get_by_name("abuffer");
    if (!buffersrc) throw std::runtime_error("no abuffer filter");

    if (m_in_av_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
        av_channel_layout_default(&m_in_av_ctx->ch_layout,
                                  m_in_av_ctx->ch_layout.nb_channels);

    std::array<char, 512> src_args{};
    auto s =
        snprintf(src_args.data(), src_args.size(),
                 "sample_rate=%d:sample_fmt=%s:time_base=%d/%d:channel_layout=",
                 m_in_av_ctx->sample_rate,
                 av_get_sample_fmt_name(m_in_av_ctx->sample_fmt),
                 m_in_av_ctx->time_base.num, m_in_av_ctx->time_base.den);
    av_channel_layout_describe(&m_in_av_ctx->ch_layout, &src_args.at(s),
                               src_args.size() - s);
    // LOGD("filter bufsrc: " << src_args.data());

    if (avfilter_graph_create_filter(&m_av_filter_ctx.src_ctx, buffersrc, "in",
                                     src_args.data(), nullptr,
                                     m_av_filter_ctx.graph) < 0) {
        clean_av();
        throw std::runtime_error("avfilter_graph_create_filter error");
    }

    const auto* buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersink) throw std::runtime_error("no abuffersink filter");

    if (avfilter_graph_create_filter(&m_av_filter_ctx.sink_ctx, buffersink,
                                     "out", nullptr, nullptr,
                                     m_av_filter_ctx.graph) < 0) {
        clean_av();
        throw std::runtime_error("avfilter_graph_create_filter error");
    }

    const AVSampleFormat out_sample_fmts[] = {m_out_av_ctx->sample_fmt,
                                              AV_SAMPLE_FMT_NONE};
    if (av_opt_set_int_list(m_av_filter_ctx.sink_ctx, "sample_fmts",
                            out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN) < 0) {
        clean_av();
        throw std::runtime_error("av_opt_set_int_list error");
    }

    std::array<char, 512> chname{};
    av_channel_layout_describe(&m_out_av_ctx->ch_layout, chname.data(),
                               chname.size());
    if (av_opt_set(m_av_filter_ctx.sink_ctx, "ch_layouts", chname.data(),
                   AV_OPT_SEARCH_CHILDREN) < 0) {
        clean_av();
        throw std::runtime_error("av_opt_set error");
    }

    const int out_sample_rates[] = {m_out_av_ctx->sample_rate, -1};
    if (av_opt_set_int_list(m_av_filter_ctx.sink_ctx, "sample_rates",
                            out_sample_rates, -1, AV_OPT_SEARCH_CHILDREN) < 0) {
        clean_av();
        throw std::runtime_error("av_opt_set_int_list error");
    }

    m_av_filter_ctx.out->name = av_strdup("in");
    m_av_filter_ctx.out->filter_ctx = m_av_filter_ctx.src_ctx;
    m_av_filter_ctx.out->pad_idx = 0;
    m_av_filter_ctx.out->next = nullptr;

    m_av_filter_ctx.in->name = av_strdup("out");
    m_av_filter_ctx.in->filter_ctx = m_av_filter_ctx.sink_ctx;
    m_av_filter_ctx.in->pad_idx = 0;
    m_av_filter_ctx.in->next = nullptr;

    if (avfilter_graph_parse_ptr(m_av_filter_ctx.graph, arg,
                                 &m_av_filter_ctx.in, &m_av_filter_ctx.out,
                                 nullptr) < 0) {
        clean_av();
        throw std::runtime_error("audio avfilter_graph_parse_ptr error");
    }

    if (avfilter_graph_config(m_av_filter_ctx.graph, nullptr) < 0) {
        clean_av();
        throw std::runtime_error("audio avfilter_graph_config error");
    }
}

void media_compressor::init_av_in_format(const std::string& input_file,
                                         bool skip_stream_discovery) {
    clean_av_in_format();

    LOGD("opening file: " << input_file);

    if (auto ret = avformat_open_input(&m_in_av_format_ctx, input_file.c_str(),
                                       nullptr, nullptr);
        ret < 0) {
        LOGE("avformat_open_input error: " << str_from_av_error(ret));
        throw std::runtime_error("avformat_open_input error");
    }

    if (!m_in_av_format_ctx) {
        throw std::runtime_error("in_av_format_ctx is null");
    }

    if (avformat_find_stream_info(m_in_av_format_ctx, nullptr) < 0) {
        clean_av();
        throw std::runtime_error("avformat_find_stream_info error");
    }

    m_in_stream_idx = -1;

    if (skip_stream_discovery) return;

    if (!m_options.stream || m_options.stream->index < 0) {
        AVMediaType type = AVMEDIA_TYPE_AUDIO;
        if (m_options.stream) {
            switch (m_options.stream->media_type) {
                case media_type_t::video:
                    type = AVMEDIA_TYPE_VIDEO;
                    break;
                case media_type_t::subtitles:
                    type = AVMEDIA_TYPE_SUBTITLE;
                    break;
                case media_type_t::audio:
                    break;
            }
        }

        m_in_stream_idx =
            av_find_best_stream(m_in_av_format_ctx, type, -1, -1, nullptr, 0);
        if (m_in_stream_idx < 0) {
            clean_av();
            throw std::runtime_error("no stream found in input file");
        }
    } else {
        for (auto i = 0u; i < m_in_av_format_ctx->nb_streams; ++i) {
            if (m_in_av_format_ctx->streams[i]->index ==
                    m_options.stream->index &&
                ((m_in_av_format_ctx->streams[i]->codecpar->codec_type ==
                      AVMEDIA_TYPE_AUDIO &&
                  m_options.stream->media_type == media_type_t::audio) ||
                 (m_in_av_format_ctx->streams[i]->codecpar->codec_type ==
                      AVMEDIA_TYPE_SUBTITLE &&
                  m_options.stream->media_type == media_type_t::subtitles))) {
                m_in_stream_idx = i;
            }
        }

        if (m_in_stream_idx < 0) {
            clean_av();
            throw std::runtime_error(
                "no stream with requested index and type found in input file");
        }
    }

    // const auto* in_stream = m_in_av_format_ctx->streams[m_in_stream_idx];

    // LOGD("in stream: codec="
    //      << in_stream->codecpar->codec_id << ", sample-rate="
    //      << in_stream->codecpar->sample_rate << ", tb=" <<
    //      in_stream->time_base
    //      << ", frame-size=" << in_stream->codecpar->frame_size
    //      << ", sample-format="
    //      << static_cast<AVSampleFormat>(in_stream->codecpar->format));
}

static uint64_t time_ms_to_pcm_bytes(uint64_t time_ms, int sample_rate,
                                     int channels) {
    return (time_ms * 2 * sample_rate * channels) / 1000.0;
}

void media_compressor::init_av(task_t task) {
    init_av_in_format(m_input_files.front(), false);
    m_input_files.pop();

    const auto* in_stream = m_in_av_format_ctx->streams[m_in_stream_idx];

    LOGD("input codec: " << in_stream->codecpar->codec_id << " ("
                         << static_cast<int>(in_stream->codecpar->codec_id)
                         << ")");

    m_no_decode = [&]() {
        if (m_options.mono || m_options.sample_rate_16) return false;

        switch (task) {
            case task_t::compress_to_file:
                switch (m_format) {
                    case format_t::wav:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_PCM_S16LE;
                    case format_t::mp3:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_MP3;
                    case format_t::flac:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_FLAC;
                    case format_t::srt:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_SUBRIP;
                    case format_t::ass:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_SSA;
                    case format_t::vtt:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_WEBVTT;
                    case format_t::ogg_vorbis:
                    case format_t::ogg_opus:
                        return false;
                    case format_t::unknown:
                        break;
                }
                break;
            case task_t::decompress_to_file:
            case task_t::decompress_to_file_async:
            case task_t::decompress_to_data_raw_async:
            case task_t::decompress_to_data_format_async:
                break;
        }
        return false;
    }();

    if (in_stream->codecpar->codec_id == AV_CODEC_ID_PCM_S16LE &&
        m_clip_info.valid_time()) {
        m_clip_info.start_bytes = time_ms_to_pcm_bytes(
            m_clip_info.start_time_ms, in_stream->codecpar->sample_rate,
            in_stream->codecpar->ch_layout.nb_channels);
        m_clip_info.stop_bytes = time_ms_to_pcm_bytes(
            m_clip_info.stop_time_ms, in_stream->codecpar->sample_rate,
            in_stream->codecpar->ch_layout.nb_channels);
    }

    if (m_no_decode) {
        LOGD("no decode");
    } else {
        const auto* decoder =
            avcodec_find_decoder(in_stream->codecpar->codec_id);
        if (!decoder) {
            clean_av();
            throw std::runtime_error("avcodec_find_decoder error");
        }

        //    LOGD("sample fmts supported by decoder:");
        //    for (int i = 0; decoder->sample_fmts[i] != AV_SAMPLE_FMT_NONE;
        //    ++i) {
        //        LOGD("[" << i << "]: " << decoder->sample_fmts[i]);
        //    }

        m_in_av_ctx = avcodec_alloc_context3(decoder);
        if (!m_in_av_ctx) {
            clean_av();
            throw std::runtime_error("avcodec_alloc_context3 error");
        }

        if (avcodec_parameters_to_context(m_in_av_ctx, in_stream->codecpar) <
            0) {
            clean_av();
            throw std::runtime_error("avcodec_parameters_to_context error");
        }

        m_in_av_ctx->time_base = in_stream->time_base;

        if (auto ret = avcodec_open2(m_in_av_ctx, nullptr, nullptr); ret != 0) {
            clean_av();
            LOGE("avcodec_open2 error: " << str_from_av_error(ret));
            throw std::runtime_error("avcodec_open2 error");
        }

        //        LOGD("in codec: codec="
        //             << m_in_av_audio_ctx->codec_id
        //             << ", channels=" <<
        //             m_in_av_audio_ctx->ch_layout.nb_channels
        //             << ", tb=" << m_in_av_audio_ctx->time_base
        //             << ", frame-size=" << m_in_av_audio_ctx->frame_size
        //             << ", sample-rate=" << m_in_av_audio_ctx->sample_rate
        //             << ", sample-format=" << m_in_av_audio_ctx->sample_fmt);

        auto encoder_name = [&]() {
            switch (task) {
                case task_t::compress_to_file:
                    switch (m_format) {
                        case format_t::mp3:
                            return "libmp3lame";
                        case format_t::ogg_vorbis:
                            return "libvorbis";
                        case format_t::ogg_opus:
                            return "libopus";
                        case format_t::flac:
                            return "flac";
                        case format_t::srt:
                            return "subrip";
                        case format_t::ass:
                            return "ssa";
                        case format_t::vtt:
                            return "webvtt";
                        case format_t::wav:
                        case format_t::unknown:
                            break;
                    }
                    break;
                case task_t::decompress_to_file:
                case task_t::decompress_to_file_async:
                case task_t::decompress_to_data_raw_async:
                case task_t::decompress_to_data_format_async:
                    if (in_stream->codecpar->codec_type ==
                        AVMEDIA_TYPE_SUBTITLE)
                        return "subrip";
                    break;
            }

            return "pcm_s16le";
        }();

        const auto* encoder = avcodec_find_encoder_by_name(encoder_name);
        if (!encoder) {
            clean_av();
            throw std::runtime_error("no encoder");
        }

        //    LOGD("sample fmts supported by encoder:");
        //    for (int i = 0; encoder->sample_fmts[i] != AV_SAMPLE_FMT_NONE;
        //    ++i) {
        //        LOGD("[" << i << "]: " << encoder->sample_fmts[i]);
        //    }

        m_out_av_ctx = avcodec_alloc_context3(encoder);
        if (!m_out_av_ctx) {
            clean_av();
            throw std::runtime_error("avcodec_alloc_context3 error");
        }

        AVDictionary* opts = nullptr;

        if (task == task_t::compress_to_file) {
            if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_out_av_ctx->sample_fmt =
                    best_sample_format(encoder, m_in_av_ctx->sample_fmt);
                av_channel_layout_default(
                    &m_out_av_ctx->ch_layout,
                    m_in_av_ctx->ch_layout.nb_channels == 1 ? 1 : 2);
                m_out_av_ctx->sample_rate =
                    best_sample_rate(encoder, m_in_av_ctx->sample_rate);

                m_out_av_ctx->time_base = m_in_av_ctx->time_base;

                if (m_format == format_t::ogg_opus) {
                    av_dict_set(&opts, "application", "voip", 0);
                    av_dict_set(&opts, "b",
                                m_options.quality == quality_t::vbr_high ? "48k"
                                : m_options.quality == quality_t::vbr_low
                                    ? "10k"
                                    : "24k",
                                0);
                } else if (m_format == format_t::flac) {
                    av_dict_set_int(&opts, "compression_level",
                                    m_options.quality == quality_t::vbr_high ? 0
                                    : m_options.quality == quality_t::vbr_low
                                        ? 12
                                        : 5,
                                    0);
                } else {
                    m_out_av_ctx->flags |= AV_CODEC_FLAG_QSCALE;

                    switch (m_options.quality) {
                        case quality_t::vbr_high:
                            m_out_av_ctx->global_quality =
                                FF_QP2LAMBDA *
                                (m_format == format_t::mp3 ? 0 : 10);
                            break;
                        case quality_t::vbr_medium:
                            m_out_av_ctx->global_quality =
                                FF_QP2LAMBDA *
                                (m_format == format_t::mp3 ? 4 : 3);
                            break;
                        case quality_t::vbr_low:
                            m_out_av_ctx->global_quality =
                                FF_QP2LAMBDA *
                                (m_format == format_t::mp3 ? 9 : 0);
                            break;
                    }
                }
            } else if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                m_out_av_ctx->time_base = AV_TIME_BASE_Q;

                m_out_av_ctx->subtitle_header = static_cast<uint8_t*>(
                    av_mallocz(m_in_av_ctx->subtitle_header_size + 1));
                if (!m_out_av_ctx->subtitle_header)
                    throw std::runtime_error("can't allocate subtitle header");

                m_out_av_ctx->subtitle_header_size =
                    m_in_av_ctx->subtitle_header_size;
            }
        } else {
            if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_out_av_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
                m_out_av_ctx->time_base = m_in_av_ctx->time_base;
                m_out_av_ctx->sample_rate = m_in_av_ctx->sample_rate;

                if (m_options.sample_rate_16 || m_options.mono) {
                    // STT engines support only mono 16000Hz
                    if (m_options.sample_rate_16) {
                        m_out_av_ctx->sample_rate = 16000;
                        m_out_av_ctx->time_base = {1, 16000};
                    }

                    if (m_options.mono)
                        av_channel_layout_default(&m_out_av_ctx->ch_layout, 1);
                } else {
                    m_out_av_ctx->sample_rate = m_in_av_ctx->sample_rate;
                    av_channel_layout_default(
                        &m_out_av_ctx->ch_layout,
                        m_in_av_ctx->ch_layout.nb_channels == 1 ? 1 : 2);
                    m_out_av_ctx->time_base = m_in_av_ctx->time_base;
                }
            } else if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                m_out_av_ctx->time_base = AV_TIME_BASE_Q;

                m_out_av_ctx->subtitle_header = static_cast<uint8_t*>(
                    av_mallocz(m_in_av_ctx->subtitle_header_size + 1));
                if (!m_out_av_ctx->subtitle_header)
                    throw std::runtime_error("can't allocate subtitle header");

                m_out_av_ctx->subtitle_header_size =
                    m_in_av_ctx->subtitle_header_size;
            } else {
                throw std::runtime_error("unsupported media type");
            }
        }

        if (int ret = avcodec_open2(m_out_av_ctx, encoder, &opts); ret < 0) {
            clean_av();
            LOGE("avcodec_open2 error: " << str_from_av_error(ret));
            throw std::runtime_error("avcodec_open2 error");
        }

        clean_av_opts(&opts);

        //        LOGD("out codec: codec="
        //             << m_out_av_audio_ctx->codec_id
        //             << ", channels=" <<
        //             m_out_av_audio_ctx->ch_layout.nb_channels
        //             << ", tb=" << m_out_av_audio_ctx->time_base
        //             << ", frame-size=" << m_out_av_audio_ctx->frame_size
        //             << ", sample-rate=" << m_out_av_audio_ctx->sample_rate
        //             << ", sample-format=" << m_out_av_audio_ctx->sample_fmt);

        if (!time_base_equal(m_out_av_ctx->time_base, m_in_av_ctx->time_base)) {
            LOGD("time-base change: " << m_out_av_ctx->time_base << " => "
                                      << m_in_av_ctx->time_base);
        }

        if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (m_out_av_ctx->sample_fmt != m_in_av_ctx->sample_fmt) {
                LOGD("sample-format change: " << m_in_av_ctx->sample_fmt
                                              << " => "
                                              << m_out_av_ctx->sample_fmt);
            }

            if (m_out_av_ctx->sample_rate != m_in_av_ctx->sample_rate) {
                LOGD("sample-rate change: " << m_in_av_ctx->sample_rate
                                            << " => "
                                            << m_out_av_ctx->sample_rate);
            }

            m_av_fifo =
                av_audio_fifo_alloc(m_out_av_ctx->sample_fmt,
                                    m_out_av_ctx->ch_layout.nb_channels, 1);
            if (!m_av_fifo) {
                clean_av();
                throw std::runtime_error("av_audio_fifo_alloc error");
            }

            init_av_filter("anull");
        }
    }

    auto* format_name = [&]() {
        switch (task) {
            case task_t::compress_to_file:
                switch (m_format) {
                    case format_t::mp3:
                        return "mp3";
                    case format_t::ogg_vorbis:
                    case format_t::ogg_opus:
                        return "ogg";
                    case format_t::flac:
                        return "flac";
                    case format_t::srt:
                        return "srt";
                    case format_t::ass:
                        return "ass";
                    case format_t::vtt:
                        return "webvtt";
                    case format_t::wav:
                    case format_t::unknown:
                        break;
                }
                break;
            case task_t::decompress_to_data_raw_async:
                return "";
            case task_t::decompress_to_file:
            case task_t::decompress_to_file_async:
            case task_t::decompress_to_data_format_async:
                if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
                    return "srt";
                break;
        }
        return "wav";
    }();

    if (task != task_t::decompress_to_data_raw_async) {
        if (avformat_alloc_output_context2(&m_out_av_format_ctx, nullptr,
                                           format_name, nullptr) < 0) {
            clean_av();
            throw std::runtime_error("avformat_alloc_output_context2 error");
        }

        m_out_av_format_ctx->flags |= AVFMT_FLAG_BITEXACT;

        auto* out_stream = avformat_new_stream(m_out_av_format_ctx, nullptr);
        if (!out_stream) {
            clean_av();
            throw std::runtime_error("avformat_new_stream error");
        }

        out_stream->id = 0;

        if (m_no_decode) {
            if (avcodec_parameters_copy(out_stream->codecpar,
                                        in_stream->codecpar) < 0) {
                clean_av();
                throw std::runtime_error("avcodec_parameters_copy error");
            }
            out_stream->time_base = in_stream->time_base;
        } else {
            if (avcodec_parameters_from_context(out_stream->codecpar,
                                                m_out_av_ctx) < 0) {
                clean_av();
                throw std::runtime_error(
                    "avcodec_parameters_from_context error");
            }
            out_stream->time_base = m_out_av_ctx->time_base;
        }

        // av_dump_format(m_out_av_format_ctx, out_stream->id, "", 1);

        if (task == task_t::decompress_to_data_format_async) {
            auto* out_buf = static_cast<uint8_t*>(av_malloc(BUF_MAX_SIZE));
            if (!out_buf) {
                av_freep(&out_buf);
                throw std::runtime_error("unable to allocate out av buf");
            }

            m_out_av_format_ctx->pb =
                avio_alloc_context(out_buf, BUF_MAX_SIZE, 1, this, nullptr,
                                   write_packet_callback, nullptr);
            if (!m_out_av_format_ctx->pb)
                throw std::runtime_error("avio_alloc_context error");

            m_out_av_format_ctx->flags |= AVFMT_FLAG_NOBUFFER |
                                          AVFMT_FLAG_FLUSH_PACKETS |
                                          AVFMT_FLAG_CUSTOM_IO;
        } else {
            if (avio_open(&m_out_av_format_ctx->pb, m_output_file.c_str(),
                          AVIO_FLAG_WRITE) < 0) {
                clean_av();
                throw std::runtime_error("avio_open error");
            }
        }

        //        LOGD("out stream: codec="
        //             << out_stream->codecpar->codec_id
        //             << ", sample-rate=" << out_stream->codecpar->sample_rate
        //             << ", tb=" << out_stream->time_base << ", frame-size="
        //             << out_stream->codecpar->frame_size << ", sample-format="
        //             <<
        //             static_cast<AVSampleFormat>(out_stream->codecpar->format));
    }
}

void media_compressor::write_to_buf(const char* data, int size) {
    if (size > BUF_MAX_SIZE)
        throw std::runtime_error("data size too large: " +
                                 std::to_string(size));

    std::unique_lock lock{m_mtx};
    m_cv.wait(lock, [&]() {
        if (m_shutdown) return true;
        if (m_buf.size() + size <= BUF_MAX_SIZE) return true;

        if (m_data_ready_callback) m_data_ready_callback();

        return false;
    });

    if (!m_shutdown) {
        auto old_size = m_buf.size();
        m_buf.resize(m_buf.size() + size);
        memcpy(std::next(m_buf.data(), old_size), data, size);
    }

    lock.unlock();
}

int media_compressor::write_packet_callback(void* opaque, uint8_t* buf,
                                            int buf_size) {
    static_cast<media_compressor*>(opaque)->write_to_buf(
        reinterpret_cast<char*>(buf), buf_size);

    return buf_size;
}

void media_compressor::setup_input_files(
    std::vector<std::string>&& input_files) {
    if (input_files.empty()) throw std::runtime_error("empty input file list");

    m_data_info = data_info_t{};
    std::for_each(input_files.begin(), input_files.end(), [&](auto& file) {
        struct stat st;
        if (stat(file.c_str(), &st) == 0) m_data_info.total += st.st_size;

        m_input_files.push(std::move(file));
    });
}

std::optional<media_compressor::media_info_t> media_compressor::media_info(
    const std::string& input_file) {
    media_info_t media_info_data{};

    try {
        init_av_in_format(input_file, true);

        for (auto i = 0u; i < m_in_av_format_ctx->nb_streams; ++i) {
            // LOGD("stream: idx="
            //      << m_in_av_format_ctx->streams[i]->index << ", type="
            //      << m_in_av_format_ctx->streams[i]->codecpar->codec_type
            //      << ", metadata=[" <<
            //      m_in_av_format_ctx->streams[i]->metadata
            //      << "]");

            media_type_t media_type = media_type_t::audio;

            switch (m_in_av_format_ctx->streams[i]->codecpar->codec_type) {
                case AVMEDIA_TYPE_VIDEO:
                    media_type = media_type_t::video;
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    media_type = media_type_t::audio;
                    break;
                case AVMEDIA_TYPE_SUBTITLE:
                    media_type = media_type_t::subtitles;
                    break;
                case AVMEDIA_TYPE_DATA:
                case AVMEDIA_TYPE_ATTACHMENT:
                case AVMEDIA_TYPE_NB:
                case AVMEDIA_TYPE_UNKNOWN:
                    continue;
            }

            stream_t stream;
            stream.index = m_in_av_format_ctx->streams[i]->index;
            stream.media_type = media_type;
            if (const auto* value = value_from_av_dict(
                    "title", m_in_av_format_ctx->streams[i]->metadata))
                stream.title = value;
            if (const auto* value = value_from_av_dict(
                    "language", m_in_av_format_ctx->streams[i]->metadata))
                stream.language = value;

            switch (stream.media_type) {
                case media_type_t::audio:
                    media_info_data.audio_streams.push_back(std::move(stream));
                    break;
                case media_type_t::video:
                    media_info_data.video_streams.push_back(std::move(stream));
                    break;
                case media_type_t::subtitles:
                    media_info_data.subtitles_streams.push_back(
                        std::move(stream));
                    break;
            }
        }
    } catch (const std::runtime_error& err) {
        LOGD("can't get media info: " << err.what());
        return std::nullopt;
    }

    return media_info_data;
}

size_t media_compressor::duration(const std::string& input_file) {
    try {
        init_av_in_format(input_file, false);

        const auto* in_stream = m_in_av_format_ctx->streams[m_in_stream_idx];

        return std::max<size_t>(
            0, av_rescale_q(in_stream->duration, in_stream->time_base,
                            AVRational{1, 1000}));
    } catch (const std::runtime_error& err) {
        LOGE("can't get duration: " << err.what());
    }

    return 0;
}

void media_compressor::decompress_to_file(std::vector<std::string> input_files,
                                          std::string output_file,
                                          options_t options) {
    LOGD("task decompress to file");

    setup_input_files(std::move(input_files));

    m_output_file = std::move(output_file);
    m_options = std::move(options);

    init_av(task_t::decompress_to_file);

    process();
}

void media_compressor::decompress_to_file_async(
    std::vector<std::string> input_files, std::string output_file,
    options_t options, task_finished_callback_t task_finished_callback) {
    LOGD("task decompress to file async");

    decompress_async_internal(task_t::decompress_to_file_async,
                              std::move(input_files), std::move(output_file),
                              std::move(options), {},
                              std::move(task_finished_callback));
}

void media_compressor::decompress_async_internal(
    task_t task, std::vector<std::string>&& input_files,
    std::string output_file, options_t options,
    data_ready_callback_t&& data_ready_callback,
    task_finished_callback_t&& task_finished_callback) {
    setup_input_files(std::move(input_files));

    m_output_file = std::move(output_file);
    m_options = std::move(options);

    init_av(task);

    if (m_async_thread.joinable()) m_async_thread.join();
    m_data_ready_callback = std::move(data_ready_callback);

    m_async_thread =
        std::thread([this, callback = std::move(task_finished_callback)]() {
            try {
                m_error = false;

                LOGD("process started");
                process();
                LOGD("process finished");
            } catch (const std::runtime_error& err) {
                LOGE("exception in process: " << err.what());
                m_error = true;
            }

            if (m_data_ready_callback && m_buf.size() > 0)
                m_data_ready_callback();
            if (callback) callback();
        });
}

void media_compressor::decompress_to_data_raw_async(
    std::vector<std::string> input_files, options_t options,
    data_ready_callback_t data_ready_callback,
    task_finished_callback_t task_finished_callback) {
    LOGD("task decompress to data raw async");

    decompress_async_internal(task_t::decompress_to_data_raw_async,
                              std::move(input_files), {}, std::move(options),
                              std::move(data_ready_callback),
                              std::move(task_finished_callback));
}

void media_compressor::decompress_to_data_format_async(
    std::vector<std::string> input_files, options_t options,
    data_ready_callback_t data_ready_callback,
    task_finished_callback_t task_finished_callback) {
    LOGD("task decompress to data format async");

    decompress_async_internal(task_t::decompress_to_data_format_async,
                              std::move(input_files), {}, options,
                              std::move(data_ready_callback),
                              std::move(task_finished_callback));
}

void media_compressor::compress_to_file_async(
    std::vector<std::string> input_files, std::string output_file,
    format_t format, options_t options,
    task_finished_callback_t task_finished_callback) {
    if (!task_finished_callback)
        throw std::runtime_error{"callback not provided"};

    compress_internal(std::move(input_files), std::move(output_file), format,
                      std::move(options), std::move(task_finished_callback));
}

void media_compressor::compress_to_file(std::vector<std::string> input_files,
                                        std::string output_file,
                                        format_t format, options_t options,
                                        clip_info_t clip_info) {
    m_clip_info = clip_info;

    auto start = std::chrono::steady_clock::now();

    compress_internal(std::move(input_files), std::move(output_file), format,
                      std::move(options), task_finished_callback_t{});

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start)
                   .count();

    LOGD("compress stats: format=" << format << ", duration=" << dur << "ms");
}

void media_compressor::compress_internal(
    std::vector<std::string> input_files, std::string output_file,
    format_t format, options_t options,
    task_finished_callback_t task_finished_callback) {
    LOGD("task compress to file: format="
         << format << ", quality=" << options.quality
         << ", async=" << static_cast<bool>(task_finished_callback));

    if (input_files.empty()) throw std::runtime_error{"empty input file list"};

    std::for_each(input_files.begin(), input_files.end(),
                  [&](auto& file) { m_input_files.push(std::move(file)); });

    m_format = format;
    if (format == format_t::unknown)
        m_format = format_from_filename(m_output_file);
    if (m_format == format_t::unknown)
        throw std::runtime_error("unknown format requested");
    m_options = std::move(options);

    m_output_file = std::move(output_file);

    init_av(task_t::compress_to_file);

    if (task_finished_callback) {
        if (m_async_thread.joinable()) m_async_thread.join();

        m_async_thread =
            std::thread([this, callback = std::move(task_finished_callback)]() {
                try {
                    m_error = false;

                    LOGD("process started");
                    process();
                    LOGD("process finished");
                } catch (const std::runtime_error& err) {
                    LOGE("exception in process: " << err.what());
                    m_error = true;
                }

                if (callback) callback();
            });

        LOGD("task compress started");
    } else {
        m_error = false;

        process();

        LOGD("task compress finished");
    }
}

void media_compressor::process() {
    if (m_out_av_format_ctx) {
        if (avformat_write_header(m_out_av_format_ctx, nullptr) < 0)
            throw std::runtime_error("avformat_write_header error");
    }

    auto* pkt = av_packet_alloc();
    if (!pkt) throw std::runtime_error("av_packet_alloc error");

    auto* frame_in = av_frame_alloc();
    if (!frame_in) {
        av_packet_free(&pkt);
        throw std::runtime_error("av_frame_alloc error");
    }
    auto* frame_out = av_frame_alloc();
    if (!frame_out) {
        av_packet_free(&pkt);
        throw std::runtime_error("av_frame_alloc error");
    }

    AVSubtitle subtitle{};

    int64_t next_ts = 0;
    m_data_info.eof = false;

    m_in_bytes_read = 0;

    while (!m_shutdown) {
        if (m_no_decode) {
            if (!read_frame(pkt)) break;
        } else {
            auto* frame{frame_out};

            if (!decode_frame(pkt, frame_in, frame, &subtitle)) frame = nullptr;

            if (m_shutdown) break;

            if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                if (m_data_info.eof) break;
                if (!encode_subtitle(pkt, &subtitle)) {
                    if (frame) continue;
                    break;
                }
            } else {
                if (!encode_frame(frame, pkt)) {
                    if (frame) continue;
                    break;
                }
            }
        }

        if (m_out_av_format_ctx) {
            if (m_out_av_format_ctx->streams[0]->codecpar->codec_type !=
                AVMEDIA_TYPE_SUBTITLE) {
                auto in_tb =
                    m_in_av_format_ctx->streams[m_in_stream_idx]->time_base;
                auto out_tb = m_out_av_format_ctx->streams[0]->time_base;

                pkt->duration = av_rescale_q(pkt->duration, in_tb, out_tb);
                pkt->stream_index = 0;
                pkt->pts = next_ts;
                pkt->dts = next_ts;
                pkt->time_base = out_tb;
                pkt->pos = -1;

                next_ts += pkt->duration;
            }

            if (auto ret = av_write_frame(m_out_av_format_ctx, pkt); ret < 0)
                throw std::runtime_error("av_write_frame error");
        } else {
            write_to_buf(reinterpret_cast<char*>(pkt->data), pkt->size);

            if (m_shutdown) break;
        }

        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_frame_free(&frame_in);
    av_frame_free(&frame_out);
    avsubtitle_free(&subtitle);

    if (m_out_av_format_ctx) {
        if (av_write_trailer(m_out_av_format_ctx) < 0)
            LOGW("av_write_trailer error");

        if (m_out_av_format_ctx->flags & AVFMT_FLAG_CUSTOM_IO) {
            if (m_out_av_format_ctx->pb) {
                if (m_out_av_format_ctx->pb->buffer)
                    av_freep(&m_out_av_format_ctx->pb->buffer);
                avio_context_free(&m_out_av_format_ctx->pb);
            }
        } else {
            avio_closep(&m_out_av_format_ctx->pb);

            if (m_shutdown) unlink(m_output_file.c_str());
        }
    }
}

bool media_compressor::read_frame(AVPacket* pkt) {
    bool do_clip = m_clip_info.valid_bytes();

    while (true) {
        if (do_clip && m_in_bytes_read >= m_clip_info.stop_bytes) {
            LOGD("demuxer clip stop");
            m_data_info.eof = true;
            break;
        }

        if (auto ret = av_read_frame(m_in_av_format_ctx, pkt); ret != 0) {
            if (ret == AVERROR_EOF) {
                if (m_input_files.empty()) {
                    LOGD("demuxer eof");
                    m_data_info.eof = true;
                    return false;
                } else {
                    init_av_in_format(m_input_files.front(), false);
                    m_input_files.pop();
                    continue;
                }
            }

            throw std::runtime_error("av_read_frame error");
        }

        if (pkt->flags & AV_PKT_FLAG_CORRUPT) LOGD("corrupt pkt");

        if (pkt->stream_index != m_in_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }

        m_in_bytes_read += pkt->size;

        if (do_clip && m_in_bytes_read < m_clip_info.start_bytes) {
            av_packet_unref(pkt);
            continue;
        }

        return true;
    }

    return false;
}

bool media_compressor::decode_frame(AVPacket* pkt, AVFrame* frame_in,
                                    AVFrame* frame_out, AVSubtitle* subtitle) {
    bool do_clip = m_clip_info.valid_bytes() &&
                   m_out_av_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE;

    if (m_out_av_ctx->frame_size > 0) {  // decoder needs fix frame size
        while (!m_shutdown &&
               av_audio_fifo_size(m_av_fifo) < m_out_av_ctx->frame_size &&
               !m_data_info.eof) {
            if (do_clip && m_in_bytes_read >= m_clip_info.stop_bytes) {
                LOGD("demuxer clip stop");
                m_data_info.eof = true;
                break;
            } else if (auto ret = av_read_frame(m_in_av_format_ctx, pkt);
                       ret != 0) {
                if (ret == AVERROR_EOF) {
                    if (m_input_files.empty()) {
                        LOGD("demuxer eof");
                        m_data_info.eof = true;
                    } else {
                        init_av_in_format(m_input_files.front(), false);
                        m_input_files.pop();
                        continue;
                    }
                } else {
                    throw std::runtime_error("av_read_frame error");
                }
            }

            if (m_data_info.eof) {
                if (!filter_frame(nullptr, frame_out)) continue;
            } else {
                if (pkt->stream_index != m_in_stream_idx) {
                    av_packet_unref(pkt);
                    continue;
                }

                m_in_bytes_read += pkt->size;

                if (do_clip && m_in_bytes_read < m_clip_info.start_bytes) {
                    av_packet_unref(pkt);
                    continue;
                }

                if (auto ret = avcodec_send_packet(m_in_av_ctx, pkt);
                    ret != 0 && ret != AVERROR(EAGAIN)) {
                    av_packet_unref(pkt);
                    LOGW("audio decoding error: " << ret << " "
                                                  << str_from_av_error(ret));
                    continue;
                }

                av_packet_unref(pkt);

                if (auto ret = avcodec_receive_frame(m_in_av_ctx, frame_in);
                    ret != 0) {
                    if (ret == AVERROR(EAGAIN)) continue;

                    throw std::runtime_error("avcodec_receive_frame error");
                }

                if (!filter_frame(frame_in, frame_out)) continue;
            }

            if (av_audio_fifo_realloc(m_av_fifo, av_audio_fifo_size(m_av_fifo) +
                                                     frame_out->nb_samples) < 0)
                throw std::runtime_error("av_audio_fifo_realloc error");

            if (av_audio_fifo_write(
                    m_av_fifo, reinterpret_cast<void**>(frame_out->data),
                    frame_out->nb_samples) < frame_out->nb_samples)
                throw std::runtime_error("av_audio_fifo_write error");

            av_frame_unref(frame_out);
        }

        auto size_to_read =
            std::min(av_audio_fifo_size(m_av_fifo), m_out_av_ctx->frame_size);

        if (size_to_read == 0) return false;

        frame_out->nb_samples = size_to_read;
        av_channel_layout_copy(&frame_out->ch_layout, &m_in_av_ctx->ch_layout);
        frame_out->format = m_out_av_ctx->sample_fmt;
        frame_out->sample_rate = m_out_av_ctx->sample_rate;

        if (av_frame_get_buffer(frame_out, 0) != 0)
            throw std::runtime_error("av_frame_get_buffer error");

        if (av_audio_fifo_read(m_av_fifo,
                               reinterpret_cast<void**>(frame_out->data),
                               size_to_read) < size_to_read) {
            throw std::runtime_error("av_audio_fifo_read error");
        }
    } else {
        while (!m_shutdown) {
            if (do_clip && m_in_bytes_read >= m_clip_info.stop_bytes) {
                LOGD("demuxer clip stop");
                m_data_info.eof = true;
                break;
            } else if (auto ret = av_read_frame(m_in_av_format_ctx, pkt);
                       ret != 0) {
                if (ret == AVERROR_EOF) {
                    if (m_input_files.empty()) {
                        LOGD("demuxer eof");
                        m_data_info.eof = true;
                    } else {
                        init_av_in_format(m_input_files.front(), false);
                        m_input_files.pop();
                        continue;
                    }
                } else {
                    throw std::runtime_error("av_read_frame error");
                }
            }

            pkt->time_base =
                m_in_av_format_ctx->streams[m_in_stream_idx]->time_base;

            if (m_data_info.eof) {
                if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                    return false;
                } else {
                    return filter_frame(nullptr, frame_out);
                }
            }

            if (pkt->stream_index != m_in_stream_idx) {
                av_packet_unref(pkt);
                continue;
            }

            m_in_bytes_read += pkt->size;

            if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                int got_output = 0;
                if (avcodec_decode_subtitle2(m_in_av_ctx, subtitle, &got_output,
                                             pkt) < 0) {
                    throw std::runtime_error("avcodec_decode_subtitle2 error");
                }

                if (got_output == 0) {
                    LOGW("no subtitle output");
                    av_packet_unref(pkt);
                    m_data_info.eof = true;
                    return false;
                    continue;
                }

                auto pts = pkt->pts;
                if (pts == AV_NOPTS_VALUE) pts = pkt->dts;
                if (pts == AV_NOPTS_VALUE) pts = 0;

                // subtitle->pts = av_rescale_q(pts, pkt->time_base, {1, 1000});
                subtitle->pts = pts;
                subtitle->end_display_time =
                    av_rescale_q(pkt->duration, pkt->time_base, {1, 1000});

                av_packet_unref(pkt);
            } else {
                if (do_clip && m_in_bytes_read < m_clip_info.start_bytes) {
                    av_packet_unref(pkt);
                    continue;
                }

                if (auto ret = avcodec_send_packet(m_in_av_ctx, pkt);
                    ret != 0 && ret != AVERROR(EAGAIN)) {
                    av_packet_unref(pkt);
                    LOGW("audio frame decoding error: "
                         << ret << " " << str_from_av_error(ret));
                    continue;
                }

                av_packet_unref(pkt);

                if (auto ret = avcodec_receive_frame(m_in_av_ctx, frame_in);
                    ret != 0) {
                    if (ret == AVERROR(EAGAIN)) continue;

                    throw std::runtime_error("avcodec_receive_frame error");
                }

                if (!filter_frame(frame_in, frame_out)) continue;
            }

            break;
        }
    }

    return true;
}

bool media_compressor::filter_frame(AVFrame* frame_in, AVFrame* frame_out) {
    if (av_buffersrc_add_frame_flags(m_av_filter_ctx.src_ctx, frame_in,
                                     AV_BUFFERSRC_FLAG_PUSH) < 0) {
        av_frame_unref(frame_in);
        throw std::runtime_error("av_buffersrc_add_frame_flags error");
    }

    av_frame_unref(frame_in);

    auto ret = av_buffersink_get_frame(m_av_filter_ctx.sink_ctx, frame_out);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return false;
    if (ret < 0) {
        av_frame_unref(frame_out);
        throw std::runtime_error("audio av_buffersink_get_frame error");
    }

    return true;
}

bool media_compressor::encode_subtitle(AVPacket* pkt, AVSubtitle* subtitle) {
    if (av_new_packet(pkt, 1024 * 1024) < 0)
        throw std::runtime_error("av_new_packet for subtitle error");

    auto in_tb = m_in_av_format_ctx->streams[m_in_stream_idx]->time_base;
    subtitle->pts = av_rescale_q(subtitle->pts, in_tb, AV_TIME_BASE_Q);
    subtitle->start_display_time = 0;

    auto subtitle_out_size =
        avcodec_encode_subtitle(m_out_av_ctx, pkt->data, pkt->size, subtitle);

    if (subtitle_out_size < 0)
        throw std::runtime_error("avcodec_encode_subtitle error");

    av_shrink_packet(pkt, subtitle_out_size);

    pkt->time_base = m_out_av_format_ctx
                         ? m_out_av_format_ctx->streams[0]->time_base
                         : AV_TIME_BASE_Q;

    pkt->pts = av_rescale_q(subtitle->pts, AV_TIME_BASE_Q, pkt->time_base);
    pkt->duration =
        av_rescale_q(subtitle->end_display_time, {1, 1000}, pkt->time_base);
    pkt->dts = pkt->pts;

    return true;
}

bool media_compressor::encode_frame(AVFrame* frame, AVPacket* pkt) {
    if (auto ret = avcodec_send_frame(m_out_av_ctx, frame);
        ret != 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        av_frame_unref(frame);
        LOGW("avcodec_send_frame error: " << ret << " "
                                          << str_from_av_error(ret));
        return false;
    }

    av_frame_unref(frame);

    if (auto ret = avcodec_receive_packet(m_out_av_ctx, pkt); ret != 0) {
        if (ret == AVERROR(EAGAIN)) return false;
        if (ret == AVERROR_EOF) {
            LOGD("encoder eof");
            return false;
        }

        throw std::runtime_error("audio avcodec_receive_packet error");
    }

    return true;
}

size_t media_compressor::data_size() const { return m_buf.size(); }

media_compressor::data_info_t media_compressor::get_data(char* data,
                                                         size_t max_size) {
    std::unique_lock lock{m_mtx};

    m_data_info.size = std::min(max_size, m_buf.size());

    if (m_data_info.size > 0) {
        memcpy(data, m_buf.data(), m_data_info.size);
        memmove(m_buf.data(), m_buf.data() + m_data_info.size,
                m_buf.size() - m_data_info.size);
        m_buf.resize(m_buf.size() - m_data_info.size);
    }

    lock.unlock();
    m_cv.notify_all();

    if (m_in_av_format_ctx && m_in_av_format_ctx->pb) {
        m_data_info.bytes_read = m_in_av_format_ctx->pb->bytes_read;
    } else {
        m_data_info.bytes_read = m_data_info.total;
    }

    return m_data_info;
}

std::pair<media_compressor::data_info_t, std::string>
media_compressor::get_all_data() {
    std::pair<media_compressor::data_info_t, std::string> result;

    auto size = data_size();
    result.second.resize(size);

    result.first = get_data(result.second.data(), size);

    return result;
}

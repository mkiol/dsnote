/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "media_compressor.hpp"

#include <fmt/format.h>
#include <locale.h>
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

[[maybe_unused]] static bool time_base_equal(AVRational l, AVRational r) {
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

    if (strcmp(encoder->name, "libopus") == 0) return 48000;

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
        case media_compressor::format_t::audio_wav:
            os << "audio-wav";
            break;
        case media_compressor::format_t::audio_mp3:
            os << "audio-mp3";
            break;
        case media_compressor::format_t::audio_ogg_vorbis:
            os << "audio-ogg-vorbis";
            break;
        case media_compressor::format_t::audio_ogg_opus:
            os << "audio-ogg-opus";
            break;
        case media_compressor::format_t::audio_flac:
            os << "audio-flac";
            break;
        case media_compressor::format_t::sub_srt:
            os << "sub-srt";
            break;
        case media_compressor::format_t::sub_ass:
            os << "sub-ass";
            break;
        case media_compressor::format_t::sub_vtt:
            os << "sub-vtt";
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
        case media_compressor::media_type_t::unknown:
            os << "unknown";
            break;
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

    if (ext == "mp3") return format_t::audio_mp3;
    if (ext == "ogg" || ext == "oga" || ext == "ogx")
        return format_t::audio_ogg_vorbis;
    if (ext == "opus") return format_t::audio_ogg_opus;
    if (ext == "flac") return format_t::audio_flac;
    if (ext == "srt") return format_t::sub_srt;
    if (ext == "aas" || ext == "ssa") return format_t::sub_ass;
    if (ext == "vtt") return format_t::sub_vtt;

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

    if (m_in_main_av_ctx) avcodec_free_context(&m_in_main_av_ctx);

    if (m_in_main_av_format_ctx) avformat_close_input(&m_in_main_av_format_ctx);
}

void media_compressor::init_av_filter() {
    m_av_filter_ctx.graph = avfilter_graph_alloc();
    if (!m_av_filter_ctx.graph)
        throw std::runtime_error("failed to allocate av filter graph");

    const auto* buffersrc = avfilter_get_by_name("abuffer");
    if (!buffersrc) throw std::runtime_error("no abuffer filter");

    if (m_in_av_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
        av_channel_layout_default(&m_in_av_ctx->ch_layout,
                                  m_in_av_ctx->ch_layout.nb_channels);

    if (m_in_main_av_ctx) {
        if (m_in_main_av_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
            av_channel_layout_default(&m_in_main_av_ctx->ch_layout,
                                      m_in_main_av_ctx->ch_layout.nb_channels);
    }

    auto make_src_args = [](const AVCodecContext* codec_ctx) {
        std::string args(512, '\0');
        auto s = snprintf(
            args.data(), args.size(),
            "sample_rate=%d:sample_fmt=%s:time_base=%d/%d:channel_layout=",
            codec_ctx->sample_rate,
            av_get_sample_fmt_name(codec_ctx->sample_fmt),
            codec_ctx->time_base.num, codec_ctx->time_base.den);
        auto lsize = av_channel_layout_describe(&codec_ctx->ch_layout,
                                                &args.at(s), args.size() - s);
        args.resize(s + lsize - 1);
        return args;
    };

    AVFilterContext* filter_mix_ctx = nullptr;
    AVFilterContext* filter_volume_ctx = nullptr;
    AVFilterContext* filter_rubberband_ctx = nullptr;

    auto args = make_src_args(m_in_av_ctx);
    LOGD("filter src args: " << args);

    if (avfilter_graph_create_filter(&m_av_filter_ctx.src_ctx, buffersrc, "src",
                                     args.data(), nullptr,
                                     m_av_filter_ctx.graph) < 0) {
        clean_av();
        throw std::runtime_error("src avfilter_graph_create_filter error");
    }

    if (m_in_main_av_ctx) {
        args = make_src_args(m_in_main_av_ctx);
        LOGD("main filter src args: " << args);

        if (avfilter_graph_create_filter(&m_av_filter_ctx.src_main_ctx,
                                         buffersrc, "src_main", args.data(),
                                         nullptr, m_av_filter_ctx.graph) < 0) {
            clean_av();
            throw std::runtime_error(
                "main src avfilter_graph_create_filter error");
        }

        const auto* amix = avfilter_get_by_name("amix");
        if (!amix) throw std::runtime_error("no amix filter");

        if (avfilter_graph_create_filter(&filter_mix_ctx, amix, "mix",
                                         "inputs=2:duration=first", nullptr,
                                         m_av_filter_ctx.graph) < 0) {
            clean_av();
            throw std::runtime_error("mix avfilter_graph_create_filter error");
        }

        bool change_volume = m_options && m_options->stream &&
                             m_options->stream->volume_change != 0;
        if (change_volume) {
            const auto* volume = avfilter_get_by_name("volume");
            if (!volume) throw std::runtime_error("no volume filter");
            args = fmt::format(
                "volume={}dB",
                std::clamp(m_options->stream->volume_change, -30, 30));
            LOGD("volume filter args: " << args);
            if (avfilter_graph_create_filter(&filter_volume_ctx, volume,
                                             "volume", args.c_str(), nullptr,
                                             m_av_filter_ctx.graph) < 0) {
                clean_av();
                throw std::runtime_error(
                    "volume avfilter_graph_create_filter error");
            }
        }
    } else if (m_options &&
               (m_options->flags & flags_t::flag_change_speed) != 0) {
        const auto* rubberband = avfilter_get_by_name("rubberband");
        if (!rubberband) throw std::runtime_error("no rubberband filter");

        args = fmt::format("tempo={:.2}:transients=smooth:window=long",
                           m_options->speed);
        LOGD("rubberband filter args: " << args);

        const char* old_locale = setlocale(LC_NUMERIC, "C");
        if (avfilter_graph_create_filter(&filter_rubberband_ctx, rubberband,
                                         "rubberband", args.c_str(), nullptr,
                                         m_av_filter_ctx.graph) < 0) {
            setlocale(LC_NUMERIC, old_locale);
            clean_av();
            throw std::runtime_error(
                "rubberband avfilter_graph_create_filter error");
        }
        setlocale(LC_NUMERIC, old_locale);
    }

    const auto* buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersink) throw std::runtime_error("no abuffersink filter");

    if (avfilter_graph_create_filter(&m_av_filter_ctx.sink_ctx, buffersink,
                                     "sink", nullptr, nullptr,
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

    if (m_in_main_av_ctx) {
        if (filter_volume_ctx) {
            if (avfilter_link(m_av_filter_ctx.src_main_ctx, 0,
                              filter_volume_ctx, 0) < 0)
                throw std::runtime_error("src_main_ctx avfilter_link error");
            if (avfilter_link(filter_volume_ctx, 0, filter_mix_ctx, 0) < 0)
                throw std::runtime_error("volume_ctx avfilter_link error");
        } else {
            if (avfilter_link(m_av_filter_ctx.src_main_ctx, 0, filter_mix_ctx,
                              0) < 0)
                throw std::runtime_error("src_main_ctx avfilter_link error");
        }

        if (avfilter_link(m_av_filter_ctx.src_ctx, 0, filter_mix_ctx, 1) < 0)
            throw std::runtime_error("src_ctx avfilter_link error");
        if (avfilter_link(filter_mix_ctx, 0, m_av_filter_ctx.sink_ctx, 0) < 0)
            throw std::runtime_error("mix_ctx avfilter_link error");
    } else if (filter_rubberband_ctx) {
        if (avfilter_link(m_av_filter_ctx.src_ctx, 0, filter_rubberband_ctx,
                          0) < 0)
            throw std::runtime_error("src_ctx avfilter_link error");
        if (avfilter_link(filter_rubberband_ctx, 0, m_av_filter_ctx.sink_ctx,
                          0) < 0)
            throw std::runtime_error(
                "filter_rubberband_ctx avfilter_link error");
    } else {
        if (avfilter_link(m_av_filter_ctx.src_ctx, 0, m_av_filter_ctx.sink_ctx,
                          0) < 0)
            throw std::runtime_error("src_ctx avfilter_link error");
    }

    if (avfilter_graph_config(m_av_filter_ctx.graph, nullptr) < 0) {
        clean_av();
        throw std::runtime_error("audio avfilter_graph_config error");
    }

#ifdef DEBUG
    auto* graph_str = avfilter_graph_dump(m_av_filter_ctx.graph, nullptr);
    if (graph_str) {
        LOGD("filter graph:\n" << graph_str);
        av_free(graph_str);
    }
#endif
}

void media_compressor::init_av_in_main_format(const std::string& input_file) {
    LOGD("opening main file: " << input_file);

    if (auto ret = avformat_open_input(&m_in_main_av_format_ctx,
                                       input_file.c_str(), nullptr, nullptr);
        ret < 0) {
        LOGE("main avformat_open_input error: " << str_from_av_error(ret));
        throw std::runtime_error("main avformat_open_input error");
    }

    if (!m_in_main_av_format_ctx) {
        throw std::runtime_error("main in_av_main_format_ctx is null");
    }

    if (avformat_find_stream_info(m_in_main_av_format_ctx, nullptr) < 0) {
        clean_av();
        throw std::runtime_error("main avformat_find_stream_info error");
    }

    if (m_in_main_av_format_ctx->nb_streams == 0) {
        clean_av();
        throw std::runtime_error("no main stream");
    }

    m_in_main_stream_idx = -1;

    if (m_options && m_options->stream && m_options->stream->index >= 0) {
        if (m_options->stream->media_type != media_type_t::audio)
            throw std::runtime_error{
                "invalid media type, only audio is supported"};
        m_in_main_stream_idx = m_options->stream->index;
        LOGD("stream requested => selecting audio stream: "
             << m_in_main_stream_idx);

    } else {
        LOGD("no stream requested => auto selecting audio stream");
    }

    m_in_main_stream_idx =
        av_find_best_stream(m_in_main_av_format_ctx, AVMEDIA_TYPE_AUDIO,
                            m_in_main_stream_idx, -1, nullptr, 0);
    if (m_in_main_stream_idx < 0) {
        clean_av();
        throw std::runtime_error(
            "no stream with requested index found in input file");
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

    if (m_in_av_format_ctx->nb_streams == 0) {
        clean_av();
        throw std::runtime_error("no stream");
    }

    m_in_stream_idx = -1;

    if (skip_stream_discovery) return;

    if (m_file_progress_callback && m_total_files_to_process > 0) {
        m_file_progress_callback(
            m_total_files_to_process - m_input_files.size(),
            m_total_files_to_process);
    }

    if (!m_options || !m_options->stream) {
        LOGD("no stream type requested => selecting first stream");
        m_in_stream_idx = 0;
        return;
    }

    if (m_in_main_av_format_ctx || m_options->stream->index < 0) {
        AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
        if (m_options->stream) {
            switch (m_options->stream->media_type) {
                case media_type_t::video:
                    type = AVMEDIA_TYPE_VIDEO;
                    break;
                case media_type_t::subtitles:
                    type = AVMEDIA_TYPE_SUBTITLE;
                    break;
                case media_type_t::audio:
                    type = AVMEDIA_TYPE_AUDIO;
                    break;
                case media_type_t::unknown:
                    break;
            }
        }

        if (type == AVMEDIA_TYPE_UNKNOWN) {
            LOGD("stream unknown type requested => selecting first stream");
            m_in_stream_idx = 0;
            return;
        }

        m_in_stream_idx =
            av_find_best_stream(m_in_av_format_ctx, type, -1, -1, nullptr, 0);
        if (m_in_stream_idx < 0) {
            clean_av();
            throw std::runtime_error(
                "no stream with requested type found in input file");
        }

        LOGD("stream type requested => selecting stream: " << m_in_stream_idx);

        return;
    }

    for (auto i = 0u; i < m_in_av_format_ctx->nb_streams; ++i) {
        if (m_in_av_format_ctx->streams[i]->index == m_options->stream->index &&
            ((m_in_av_format_ctx->streams[i]->codecpar->codec_type ==
                  AVMEDIA_TYPE_AUDIO &&
              m_options->stream->media_type == media_type_t::audio) ||
             (m_in_av_format_ctx->streams[i]->codecpar->codec_type ==
                  AVMEDIA_TYPE_SUBTITLE &&
              m_options->stream->media_type == media_type_t::subtitles))) {
            m_in_stream_idx = i;
        }
    }

    if (m_in_stream_idx < 0) {
        clean_av();
        throw std::runtime_error(
            "no stream with requested index and type found in input file");
    }

    LOGD("stream index requested => selecting stream: " << m_in_stream_idx);
}

static uint64_t time_ms_to_pcm_bytes(uint64_t time_ms, int sample_rate,
                                     int channels) {
    return (time_ms * 2 * sample_rate * channels) / 1000.0;
}

void media_compressor::setup_decoder(const AVStream* in_stream,
                                     AVCodecContext** in_av_ctx) {
    const auto* decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
    if (!decoder) {
        clean_av();
        throw std::runtime_error("avcodec_find_decoder error");
    }

    *in_av_ctx = avcodec_alloc_context3(decoder);
    if (!*in_av_ctx) {
        clean_av();
        throw std::runtime_error("avcodec_alloc_context3 error");
    }

    auto* ctx = *in_av_ctx;

    if (avcodec_parameters_to_context(ctx, in_stream->codecpar) < 0) {
        clean_av();
        throw std::runtime_error("avcodec_parameters_to_context error");
    }

    if (auto ret = avcodec_open2(ctx, nullptr, nullptr); ret != 0) {
        clean_av();
        LOGE("avcodec_open2 error: " << str_from_av_error(ret));
        throw std::runtime_error("avcodec_open2 error");
    }
}

void media_compressor::init_av(task_t task) {
    m_in_av_ctx = nullptr;
    m_in_main_av_ctx = nullptr;
    m_in_av_format_ctx = nullptr;
    m_in_main_av_format_ctx = nullptr;

    if (task == task_t::compress_mix_to_file) {
        init_av_in_main_format(m_main_input_file);

        const auto* in_main_stream =
            m_in_main_av_format_ctx->streams[m_in_main_stream_idx];
        LOGD("input main codec: "
             << in_main_stream->codecpar->codec_id << " ("
             << static_cast<int>(in_main_stream->codecpar->codec_id) << ")");
    }

    init_av_in_format(m_input_files.front(), false);
    m_input_files.pop();

    const auto* in_stream = m_in_av_format_ctx->streams[m_in_stream_idx];
    LOGD("input codec: " << in_stream->codecpar->codec_id << " ("
                         << static_cast<int>(in_stream->codecpar->codec_id)
                         << ")");

    LOGD("requested out format: " << m_format);

    m_no_decode = [&]() {
        if (m_options &&
            ((m_options->flags & flag_force_mono_output) ||
             (m_options->flags & flag_force_16k_sample_rate_output) ||
             (m_options->flags & flag_change_speed)))
            return false;

        switch (task) {
            case task_t::compress_to_file:
                switch (m_format) {
                    case format_t::audio_wav:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_PCM_S16LE;
                    case format_t::audio_mp3:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_MP3;
                    case format_t::audio_flac:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_FLAC;
                    case format_t::sub_srt:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_SUBRIP;
                    case format_t::sub_ass:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_SSA;
                    case format_t::sub_vtt:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_WEBVTT;
                    case format_t::audio_ogg_vorbis:
                    case format_t::audio_ogg_opus:
                        return false;
                    case format_t::unknown:
                        break;
                }
                break;
            case task_t::compress_mix_to_file:
                return false;
            case task_t::decompress_to_file:
            case task_t::decompress_to_file_async:
            case task_t::decompress_to_data_raw_async:
            case task_t::decompress_to_data_format_async:
                break;
        }
        return false;
    }();

    if (m_options && m_options->clip_info) {
        if (in_stream->codecpar->codec_id == AV_CODEC_ID_PCM_S16LE &&
            m_options->clip_info && m_options->clip_info->valid_time()) {
            m_options->clip_info->start_bytes = time_ms_to_pcm_bytes(
                m_options->clip_info->start_time_ms,
                in_stream->codecpar->sample_rate,
                in_stream->codecpar->ch_layout.nb_channels);
            m_options->clip_info->stop_bytes = time_ms_to_pcm_bytes(
                m_options->clip_info->stop_time_ms,
                in_stream->codecpar->sample_rate,
                in_stream->codecpar->ch_layout.nb_channels);
        } else {
            m_options->clip_info.reset();
        }
    }

    if (in_stream->codecpar->codec_id == AV_CODEC_ID_PCM_S16LE && m_options &&
        m_options->clip_info && m_options->clip_info->valid_time()) {
        m_options->clip_info->start_bytes =
            time_ms_to_pcm_bytes(m_options->clip_info->start_time_ms,
                                 in_stream->codecpar->sample_rate,
                                 in_stream->codecpar->ch_layout.nb_channels);
        m_options->clip_info->stop_bytes =
            time_ms_to_pcm_bytes(m_options->clip_info->stop_time_ms,
                                 in_stream->codecpar->sample_rate,
                                 in_stream->codecpar->ch_layout.nb_channels);
    }

    if (m_no_decode) {
        LOGD("no decode");
    } else {
        setup_decoder(in_stream, &m_in_av_ctx);

        if (task == task_t::compress_mix_to_file) {
            const auto* in_main_stream =
                m_in_main_av_format_ctx->streams[m_in_main_stream_idx];
            setup_decoder(in_main_stream, &m_in_main_av_ctx);
        }

        auto encoder_name = [&]() {
            switch (task) {
                case task_t::compress_to_file:
                case task_t::compress_mix_to_file:
                    switch (m_format) {
                        case format_t::audio_mp3:
                            return "libmp3lame";
                        case format_t::audio_ogg_vorbis:
                            return "libvorbis";
                        case format_t::audio_ogg_opus:
                            return "libopus";
                        case format_t::audio_flac:
                            return "flac";
                        case format_t::sub_srt:
                            return "subrip";
                        case format_t::sub_ass:
                            return "ass";
                        case format_t::sub_vtt:
                            return "webvtt";
                        case format_t::audio_wav:
                        case format_t::unknown:
                            return "pcm_s16le";
                    }
                    break;
                case task_t::decompress_to_file:
                case task_t::decompress_to_file_async:
                case task_t::decompress_to_data_raw_async:
                case task_t::decompress_to_data_format_async:
                    return in_stream->codecpar->codec_type ==
                                   AVMEDIA_TYPE_SUBTITLE
                               ? "subrip"
                               : "pcm_s16le";
            }

            throw std::runtime_error{"invalid encoder requested"};
        }();

        LOGD("encoder name: " << encoder_name);

        const auto* encoder = avcodec_find_encoder_by_name(encoder_name);
        if (!encoder) {
            clean_av();
            throw std::runtime_error("no encoder");
        }

        m_out_av_ctx = avcodec_alloc_context3(encoder);
        if (!m_out_av_ctx) {
            clean_av();
            throw std::runtime_error("avcodec_alloc_context3 error");
        }

        AVDictionary* opts = nullptr;

        if (task == task_t::compress_to_file ||
            task == task_t::compress_mix_to_file) {
            if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                if (m_in_main_av_ctx) {
                    av_channel_layout_default(
                        &m_out_av_ctx->ch_layout,
                        std::max(
                            m_in_main_av_ctx->ch_layout.nb_channels == 1 ? 1
                                                                         : 2,
                            m_in_av_ctx->ch_layout.nb_channels == 1 ? 1 : 2));
                    m_out_av_ctx->sample_rate = best_sample_rate(
                        encoder, std::max(m_in_main_av_ctx->sample_rate,
                                          m_in_av_ctx->sample_rate));
                    m_out_av_ctx->sample_fmt = best_sample_format(
                        encoder, AVSampleFormat::AV_SAMPLE_FMT_FLT);
                } else {
                    av_channel_layout_default(
                        &m_out_av_ctx->ch_layout,
                        m_in_av_ctx->ch_layout.nb_channels == 1 ? 1 : 2);
                    m_out_av_ctx->sample_rate =
                        best_sample_rate(encoder, m_in_av_ctx->sample_rate);
                    m_out_av_ctx->sample_fmt =
                        best_sample_format(encoder, m_in_av_ctx->sample_fmt);
                }

                m_out_av_ctx->time_base = {1, m_out_av_ctx->sample_rate};

                if (m_format == format_t::audio_ogg_opus) {
                    const auto* q = [&] {
                        switch (m_options ? m_options->quality
                                          : quality_t::vbr_medium) {
                            case quality_t::vbr_high:
                                return "48k";
                            case quality_t::vbr_low:
                                return "10k";
                            case quality_t::vbr_medium:
                                break;
                        }
                        return "24k";
                    }();

                    av_dict_set(&opts, "application", "voip", 0);
                    av_dict_set(&opts, "b", q, 0);
                } else if (m_format == format_t::audio_flac) {
                    auto q = [&] {
                        switch (m_options ? m_options->quality
                                          : quality_t::vbr_medium) {
                            case quality_t::vbr_high:
                                return 0;
                            case quality_t::vbr_low:
                                return 12;
                            case quality_t::vbr_medium:
                                break;
                        }
                        return 5;
                    }();

                    av_dict_set_int(&opts, "compression_level", q, 0);
                } else {
                    auto q = [&] {
                        switch (m_options ? m_options->quality
                                          : quality_t::vbr_medium) {
                            case quality_t::vbr_high:
                                return FF_QP2LAMBDA *
                                       (m_format == format_t::audio_mp3 ? 0
                                                                        : 10);
                            case quality_t::vbr_low:
                                return FF_QP2LAMBDA *
                                       (m_format == format_t::audio_mp3 ? 9
                                                                        : 0);
                            case quality_t::vbr_medium:
                                break;
                        }
                        return FF_QP2LAMBDA *
                               (m_format == format_t::audio_mp3 ? 4 : 3);
                    }();

                    m_out_av_ctx->flags |= AV_CODEC_FLAG_QSCALE;
                    m_out_av_ctx->global_quality = q;
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
                throw std::runtime_error{"unsupported codec type"};
            }
        } else {
            if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_out_av_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
                m_out_av_ctx->time_base = m_in_av_ctx->time_base;
                m_out_av_ctx->sample_rate = m_in_av_ctx->sample_rate;

                if (m_options &&
                    ((m_options->flags & flag_force_mono_output) ||
                     (m_options->flags & flag_force_16k_sample_rate_output))) {
                    // STT engines support only mono 16000Hz
                    if (m_options->flags & flag_force_16k_sample_rate_output) {
                        m_out_av_ctx->sample_rate = 16000;
                        m_out_av_ctx->time_base = {1, 16000};
                    } else {
                        m_out_av_ctx->sample_rate = m_in_av_ctx->sample_rate;
                    }

                    if (m_options->flags & flag_force_mono_output) {
                        av_channel_layout_default(&m_out_av_ctx->ch_layout, 1);
                    } else {
                        av_channel_layout_default(
                            &m_out_av_ctx->ch_layout,
                            m_in_av_ctx->ch_layout.nb_channels == 1 ? 1 : 2);
                    }
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
                throw std::runtime_error("unsupported codec type");
            }
        }

        if (int ret = avcodec_open2(m_out_av_ctx, encoder, &opts); ret < 0) {
            clean_av();
            LOGE("avcodec_open2 error: " << str_from_av_error(ret));
            throw std::runtime_error("avcodec_open2 error");
        }

        clean_av_opts(&opts);

        LOGD("decoder frame-size: " << m_in_av_ctx->frame_size);
        if (m_in_main_av_ctx)
            LOGD("main decoder frame-size: " << m_in_main_av_ctx->frame_size);
        LOGD("encoder frame-size: " << m_out_av_ctx->frame_size);

        LOGD("time-base change: " << m_in_av_ctx->time_base << " => "
                                  << m_out_av_ctx->time_base);
        if (m_in_main_av_ctx) {
            LOGD("main time-base change: " << m_in_main_av_ctx->time_base
                                           << " => "
                                           << m_out_av_ctx->time_base);
        }

        if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGD("sample-format change: " << m_in_av_ctx->sample_fmt << " => "
                                          << m_out_av_ctx->sample_fmt);
            LOGD("sample-rate change: " << m_in_av_ctx->sample_rate << " => "
                                        << m_out_av_ctx->sample_rate);

            m_av_fifo =
                av_audio_fifo_alloc(m_out_av_ctx->sample_fmt,
                                    m_out_av_ctx->ch_layout.nb_channels, 1);
            if (!m_av_fifo) {
                clean_av();
                throw std::runtime_error("av_audio_fifo_alloc error");
            }

            if (m_in_main_av_ctx) {
                LOGD("main sample-format change: "
                     << m_in_main_av_ctx->sample_fmt << " => "
                     << m_out_av_ctx->sample_fmt);
                LOGD("sample-rate change: " << m_in_main_av_ctx->sample_rate
                                            << " => "
                                            << m_out_av_ctx->sample_rate);
            }

            init_av_filter();
        }
    }

    auto* format_name = [&]() {
        switch (task) {
            case task_t::compress_mix_to_file:
            case task_t::compress_to_file:
                switch (m_format) {
                    case format_t::audio_mp3:
                        return "mp3";
                    case format_t::audio_ogg_vorbis:
                    case format_t::audio_ogg_opus:
                        return "ogg";
                    case format_t::audio_flac:
                        return "flac";
                    case format_t::sub_srt:
                        return "srt";
                    case format_t::sub_ass:
                        return "ass";
                    case format_t::sub_vtt:
                        return "webvtt";
                    case format_t::audio_wav:
                    case format_t::unknown:
                        return "wav";
                }
                break;
            case task_t::decompress_to_data_raw_async:
                return "";
            case task_t::decompress_to_file:
            case task_t::decompress_to_file_async:
            case task_t::decompress_to_data_format_async:
                return in_stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE
                           ? "srt"
                           : "wav";
        }
        throw std::runtime_error{"invalid output format name"};
    }();

    LOGD("output format: " << format_name);

    if (task != task_t::decompress_to_data_raw_async) {
        if (avformat_alloc_output_context2(&m_out_av_format_ctx, nullptr,
                                           format_name, nullptr) < 0) {
            clean_av();
            throw std::runtime_error("avformat_alloc_output_context2 error");
        }

        if (strncmp(format_name, "wav", 3) == 0)
            m_out_av_format_ctx->flags |= AVFMT_FLAG_BITEXACT;

        auto allocate_out_stream = [&](const AVStream* in_stream) {
            auto* out_stream =
                avformat_new_stream(m_out_av_format_ctx, nullptr);
            if (!out_stream) {
                clean_av();
                throw std::runtime_error("avformat_new_stream error");
            }

            if (in_stream) {
                if (avcodec_parameters_copy(out_stream->codecpar,
                                            in_stream->codecpar) < 0) {
                    clean_av();
                    throw std::runtime_error("avcodec_parameters_copy error");
                }

                out_stream->time_base = in_stream->time_base;
                av_dict_copy(&out_stream->metadata, in_stream->metadata, 0);
            } else if (m_out_av_ctx) {
                if (avcodec_parameters_from_context(out_stream->codecpar,
                                                    m_out_av_ctx) < 0) {
                    clean_av();
                    throw std::runtime_error(
                        "avcodec_parameters_from_context error");
                }

                out_stream->time_base = m_out_av_ctx->time_base;
            } else {
                throw std::runtime_error{"can't allocate out stream"};
            }

            return out_stream->index;
        };

        m_out_stream_idx =
            allocate_out_stream(m_out_av_ctx ? nullptr : in_stream);

        LOGD("out stream idx: " << m_out_stream_idx);

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

int media_compressor::write_packet_callback(void* opaque, ff_buf_type buf,
                                            int buf_size) {
    static_cast<media_compressor*>(opaque)->write_to_buf(
        reinterpret_cast<const char*>(buf), buf_size);

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
                case media_type_t::unknown:
                    // this should not happen
                    return std::nullopt;
            }
        }
    } catch (const std::runtime_error& err) {
        LOGD("can't get media info: " << err.what());
        return std::nullopt;
    }

    return media_info_data;
}

std::pair<size_t, unsigned int> media_compressor::duration_and_rate(
    const std::string& input_file) {
    try {
        init_av_in_format(input_file, false);

        const auto* in_stream = m_in_av_format_ctx->streams[m_in_stream_idx];

        return {std::max<size_t>(
                    0, av_rescale_q(in_stream->duration, in_stream->time_base,
                                    AVRational{1, 1000})),
                in_stream->codecpar->sample_rate};
    } catch (const std::runtime_error& err) {
        LOGE("can't get duration and rate: " << err.what());
    }

    return {0, 0};
}

void media_compressor::decompress_to_file(std::vector<std::string> input_files,
                                          std::string output_file,
                                          std::optional<options_t> options) {
    LOGD("task decompress to file");

    setup_input_files(std::move(input_files));

    m_output_file = std::move(output_file);
    m_options = std::move(options);

    init_av(task_t::decompress_to_file);

    m_error = false;

    process();

    if (m_error) unlink(m_output_file.c_str());
}

void media_compressor::decompress_to_file_async(
    std::vector<std::string> input_files, std::string output_file,
    std::optional<options_t> options,
    task_finished_callback_t task_finished_callback) {
    LOGD("task decompress to file async");

    decompress_async_internal(task_t::decompress_to_file_async,
                              std::move(input_files), std::move(output_file),
                              std::move(options), {},
                              std::move(task_finished_callback));
}

void media_compressor::decompress_async_internal(
    task_t task, std::vector<std::string>&& input_files,
    std::string output_file, std::optional<options_t> options,
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
                unlink(m_output_file.c_str());
            }

            if (m_data_ready_callback && m_buf.size() > 0)
                m_data_ready_callback();

            if (callback) callback();
        });
}

void media_compressor::decompress_to_data_raw_async(
    std::vector<std::string> input_files, std::optional<options_t> options,
    data_ready_callback_t data_ready_callback,
    task_finished_callback_t task_finished_callback) {
    LOGD("task decompress to data raw async");

    decompress_async_internal(task_t::decompress_to_data_raw_async,
                              std::move(input_files), {}, std::move(options),
                              std::move(data_ready_callback),
                              std::move(task_finished_callback));
}

void media_compressor::decompress_to_data_format_async(
    std::vector<std::string> input_files, std::optional<options_t> options,
    data_ready_callback_t data_ready_callback,
    task_finished_callback_t task_finished_callback) {
    LOGD("task decompress to data format async");

    decompress_async_internal(task_t::decompress_to_data_format_async,
                              std::move(input_files), {}, std::move(options),
                              std::move(data_ready_callback),
                              std::move(task_finished_callback));
}

void media_compressor::compress_to_file_async(
    std::vector<std::string> input_files, std::string output_file,
    format_t format, std::optional<options_t> options,
    file_progress_callback_t file_progress_callback,
    task_finished_callback_t task_finished_callback) {
    LOGD("task compress to file async");

    if (!task_finished_callback)
        throw std::runtime_error{"callback not provided"};

    compress_internal(task_t::compress_to_file, {}, std::move(input_files),
                      std::move(output_file), format, std::move(options),
                      std::move(file_progress_callback),
                      std::move(task_finished_callback));
}

void media_compressor::compress_to_file(std::vector<std::string> input_files,
                                        std::string output_file,
                                        format_t format,
                                        std::optional<options_t> options) {
    LOGD("task compress to file");

    compress_internal(task_t::compress_to_file, {}, std::move(input_files),
                      std::move(output_file), format, std::move(options),
                      file_progress_callback_t{}, task_finished_callback_t{});
}

void media_compressor::compress_mix_to_file(
    std::string main_input_file, std::vector<std::string> input_files,
    std::string output_file, format_t format,
    std::optional<options_t> options) {
    LOGD("task compress mix to file");

    if (main_input_file.empty())
        throw std::runtime_error{"empty main input file"};

    compress_internal(task_t::compress_mix_to_file, std::move(main_input_file),
                      std::move(input_files), std::move(output_file), format,
                      std::move(options), file_progress_callback_t{},
                      task_finished_callback_t{});
}

void media_compressor::compress_mix_to_file_async(
    std::string main_input_file, std::vector<std::string> input_files,
    std::string output_file, format_t format, std::optional<options_t> options,
    file_progress_callback_t file_progress_callback,
    task_finished_callback_t task_finished_callback) {
    LOGD("task compress mix to file async");

    if (!task_finished_callback)
        throw std::runtime_error{"callback not provided"};

    if (main_input_file.empty())
        throw std::runtime_error{"empty main input file"};

    compress_internal(task_t::compress_mix_to_file, std::move(main_input_file),
                      std::move(input_files), std::move(output_file), format,
                      std::move(options), std::move(file_progress_callback),
                      std::move(task_finished_callback));
}

void media_compressor::compress_internal(
    task_t task, std::string main_input_file,
    std::vector<std::string> input_files, std::string output_file,
    format_t format, std::optional<options_t> options,
    file_progress_callback_t file_progress_callback,
    task_finished_callback_t task_finished_callback) {
    if (input_files.empty()) throw std::runtime_error{"empty input file list"};
    if (output_file.empty()) throw std::runtime_error{"empty output file"};
    if (format == format_t::unknown)
        format = format_from_filename(m_output_file);
    if (format == format_t::unknown)
        throw std::runtime_error("unknown format requested");

    std::for_each(input_files.begin(), input_files.end(),
                  [&](auto& file) { m_input_files.push(std::move(file)); });
    m_output_file = std::move(output_file);
    m_main_input_file = std::move(main_input_file);
    m_options = std::move(options);
    m_format = format;
    m_file_progress_callback = std::move(file_progress_callback);
    m_total_files_to_process = m_input_files.size();

    init_av(task);

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
                    unlink(m_output_file.c_str());
                }

                if (callback) callback();
            });

        LOGD("task compress started");
    } else {
        m_error = false;

        process();

        if (m_error) unlink(m_output_file.c_str());

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

    int next_ts = 0;

    m_data_info.in_eof = false;
    m_data_info.main_in_eof = false;
    m_data_info.filter_eof = false;
    m_data_info.eof = false;

    m_in_bytes_read = 0;

    bool processing_subtitles =
        (m_out_av_format_ctx &&
         m_out_av_format_ctx->streams[m_out_stream_idx]->codecpar->codec_type ==
             AVMEDIA_TYPE_SUBTITLE) ||
        (m_in_av_format_ctx &&
         m_in_av_format_ctx->streams[m_in_stream_idx]->codecpar->codec_type ==
             AVMEDIA_TYPE_SUBTITLE);

    while (!m_shutdown && !m_data_info.eof) {
        if (m_no_decode) {
            if (!read_frame(pkt))
                if (!m_data_info.eof) continue;
        } else {
            bool do_flush = false;

            if (!m_data_info.filter_eof) {
                if (!decode_frame(pkt, frame_in, frame_out, &subtitle)) {
                    if (!m_data_info.filter_eof) continue;
                    do_flush = true;
                }
            } else {
                do_flush = true;
            }

            if (processing_subtitles) {
                if (!encode_subtitle(pkt, do_flush ? nullptr : &subtitle))
                    if (!m_data_info.eof) continue;
            } else if (!encode_frame(do_flush ? nullptr : frame_out, pkt)) {
                if (!m_data_info.eof) continue;
            }
        }

        if (!m_data_info.eof) {
            if (m_out_av_format_ctx) {
                if (!processing_subtitles) {
                    pkt->pts = next_ts;
                    pkt->dts = next_ts;
                    next_ts += pkt->duration;
                }

                LOGT("pkt: " << pkt);

                if (auto ret = av_write_frame(m_out_av_format_ctx, pkt);
                    ret < 0)
                    throw std::runtime_error("av_write_frame error");
            } else {
                write_to_buf(reinterpret_cast<char*>(pkt->data), pkt->size);

                if (m_shutdown) break;
            }

            av_packet_unref(pkt);
        } else if (m_out_av_format_ctx) {
            LOGD("muxer flush");
            av_write_frame(m_out_av_format_ctx, nullptr);
        }
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
    while (true) {
        if (m_options && m_options->clip_info &&
            m_in_bytes_read >= m_options->clip_info->stop_bytes) {
            LOGD("demuxer clip stop");
            m_data_info.in_eof = true;
            m_data_info.filter_eof = true;
            m_data_info.eof = true;
            break;
        }

        if (auto ret = av_read_frame(m_in_av_format_ctx, pkt); ret != 0) {
            if (ret == AVERROR_EOF) {
                if (m_input_files.empty()) {
                    LOGD("demuxer eof");
                    m_data_info.in_eof = true;
                    m_data_info.filter_eof = true;
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

        if (m_options && m_options->clip_info &&
            m_in_bytes_read < m_options->clip_info->start_bytes) {
            av_packet_unref(pkt);
            continue;
        }

        auto in_tb = m_in_av_format_ctx->streams[m_in_stream_idx]->time_base;
        auto out_tb = m_out_av_format_ctx->streams[m_out_stream_idx]->time_base;
        pkt->duration = av_rescale_q(pkt->duration, in_tb, out_tb);
        pkt->stream_index = m_out_stream_idx;
        pkt->time_base = out_tb;
        pkt->pos = -1;

        return true;
    }

    return false;
}

bool media_compressor::decode_frame_fix_size(AVPacket* pkt, AVFrame* frame_in,
                                             AVFrame* frame_out) {
    const bool decode_main = m_in_main_av_ctx != nullptr;

    while (!m_shutdown &&
           av_audio_fifo_size(m_av_fifo) < m_out_av_ctx->frame_size &&
           !m_data_info.filter_eof) {
        if (m_options && m_options->clip_info &&
            m_in_bytes_read >= m_options->clip_info->stop_bytes) {
            LOGD("demuxer clip stop");
            m_data_info.in_eof = true;
            m_data_info.filter_eof = true;
            break;
        }

        bool got_frame = false;

        while (!m_data_info.in_eof) {
            if (auto ret = av_read_frame(m_in_av_format_ctx, pkt); ret != 0) {
                if (ret == AVERROR_EOF) {
                    if (m_input_files.empty()) {
                        LOGD("demuxer eof");
                        m_data_info.in_eof = true;
                    } else {
                        init_av_in_format(m_input_files.front(), false);
                        m_input_files.pop();
                        continue;
                    }
                } else {
                    throw std::runtime_error("av_read_frame error");
                }
            }

            if (m_data_info.in_eof) break;

            if (pkt->stream_index != m_in_stream_idx) {
                av_packet_unref(pkt);
                continue;
            }

            m_in_bytes_read += pkt->size;

            if (m_options && m_options->clip_info &&
                m_in_bytes_read < m_options->clip_info->start_bytes) {
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

            frame_in->time_base = m_in_av_ctx->time_base;

            got_frame =
                filter_frame(m_av_filter_ctx.src_ctx, frame_in, frame_out);

            break;
        }

        if (!got_frame && m_data_info.in_eof && !m_data_info.filter_eof) {
            got_frame =
                filter_frame(m_av_filter_ctx.src_ctx, nullptr, frame_out);
        }

        if (!got_frame && decode_main) {
            while (!m_data_info.main_in_eof) {
                if (auto ret = av_read_frame(m_in_main_av_format_ctx, pkt);
                    ret != 0) {
                    if (ret == AVERROR_EOF) {
                        LOGD("main demuxer eof");
                        m_data_info.main_in_eof = true;
                    } else {
                        throw std::runtime_error("main av_read_frame error");
                    }
                }

                if (m_data_info.main_in_eof) break;

                if (pkt->stream_index != m_in_main_stream_idx) {
                    av_packet_unref(pkt);
                    continue;
                }

                if (auto ret = avcodec_send_packet(m_in_main_av_ctx, pkt);
                    ret != 0 && ret != AVERROR(EAGAIN)) {
                    av_packet_unref(pkt);
                    LOGW("main audio decoding error: "
                         << ret << " " << str_from_av_error(ret));
                    continue;
                }

                av_packet_unref(pkt);

                if (auto ret =
                        avcodec_receive_frame(m_in_main_av_ctx, frame_in);
                    ret != 0) {
                    if (ret == AVERROR(EAGAIN)) continue;

                    throw std::runtime_error("avcodec_receive_frame error");
                }

                got_frame = filter_frame(m_av_filter_ctx.src_main_ctx, frame_in,
                                         frame_out);
                break;
            }

            if (!got_frame && !m_data_info.filter_eof && m_data_info.in_eof &&
                m_data_info.main_in_eof) {
                LOGD("sending eof to filter");
                got_frame = filter_frame(m_av_filter_ctx.src_main_ctx, nullptr,
                                         frame_out);
            }
        }

        if (!got_frame) continue;

        if (av_audio_fifo_realloc(m_av_fifo, av_audio_fifo_size(m_av_fifo) +
                                                 frame_out->nb_samples) < 0)
            throw std::runtime_error("av_audio_fifo_realloc error");

        if (av_audio_fifo_write(m_av_fifo,
                                reinterpret_cast<void**>(frame_out->data),
                                frame_out->nb_samples) < frame_out->nb_samples)
            throw std::runtime_error("av_audio_fifo_write error");

        av_frame_unref(frame_out);
    }

    auto size_to_read =
        std::min(av_audio_fifo_size(m_av_fifo), m_out_av_ctx->frame_size);

    if (size_to_read == 0) return false;

    frame_out->nb_samples = size_to_read;
    av_channel_layout_copy(&frame_out->ch_layout, &m_out_av_ctx->ch_layout);
    frame_out->format = m_out_av_ctx->sample_fmt;
    frame_out->sample_rate = m_out_av_ctx->sample_rate;
    frame_out->time_base = m_out_av_ctx->time_base;

    if (av_frame_get_buffer(frame_out, 0) != 0)
        throw std::runtime_error("av_frame_get_buffer error");

    if (av_audio_fifo_read(m_av_fifo, reinterpret_cast<void**>(frame_out->data),
                           size_to_read) < size_to_read) {
        throw std::runtime_error("av_audio_fifo_read error");
    }

    return true;
}

bool media_compressor::decode_frame_var_size(AVPacket* pkt, AVFrame* frame_in,
                                             AVFrame* frame_out,
                                             AVSubtitle* subtitle) {
    const bool decode_main = m_in_main_av_format_ctx && m_in_main_av_ctx;

    bool got_frame = false;

    while (!m_shutdown && !got_frame && !m_data_info.filter_eof) {
        if (m_options && m_options->clip_info &&
            m_in_bytes_read >= m_options->clip_info->stop_bytes) {
            LOGD("demuxer clip stop");
            m_data_info.in_eof = true;
            m_data_info.filter_eof = true;
            break;
        }

        while (!m_data_info.in_eof) {
            if (auto ret = av_read_frame(m_in_av_format_ctx, pkt); ret != 0) {
                if (ret == AVERROR_EOF) {
                    if (m_input_files.empty()) {
                        LOGD("demuxer eof");
                        m_data_info.in_eof = true;
                    } else {
                        init_av_in_format(m_input_files.front(), false);
                        m_input_files.pop();
                        continue;
                    }
                } else {
                    throw std::runtime_error("av_read_frame error");
                }
            }

            if (m_data_info.in_eof) break;

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
                    LOGW("subtitle demuxer eof");
                    av_packet_unref(pkt);
                    m_data_info.in_eof = true;
                    m_data_info.filter_eof = true;
                    return false;
                }

                auto pts = pkt->pts;
                if (pts == AV_NOPTS_VALUE) pts = pkt->dts;
                if (pts == AV_NOPTS_VALUE) pts = 0;

                subtitle->pts = pts;
                subtitle->end_display_time =
                    pkt->time_base.num > 0
                        ? av_rescale_q(pkt->duration, pkt->time_base, {1, 1000})
                        : pkt->duration;

                av_packet_unref(pkt);

                got_frame = true;
            } else {
                if (m_options && m_options->clip_info &&
                    m_in_bytes_read < m_options->clip_info->start_bytes) {
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

                got_frame =
                    filter_frame(m_av_filter_ctx.src_ctx, frame_in, frame_out);
            }

            break;
        }

        if (!got_frame && m_data_info.in_eof && !m_data_info.filter_eof) {
            if (m_out_av_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                m_data_info.filter_eof = true;
                return false;
            } else {
                got_frame =
                    filter_frame(m_av_filter_ctx.src_ctx, nullptr, frame_out);
            }
        }

        if (!got_frame && decode_main) {
            while (!m_data_info.main_in_eof) {
                if (auto ret = av_read_frame(m_in_main_av_format_ctx, pkt);
                    ret != 0) {
                    if (ret == AVERROR_EOF) {
                        LOGD("main demuxer eof");
                        m_data_info.main_in_eof = true;
                    } else {
                        throw std::runtime_error("main av_read_frame error");
                    }
                }

                if (m_data_info.main_in_eof) break;

                if (pkt->stream_index != m_in_main_stream_idx) {
                    av_packet_unref(pkt);
                    continue;
                }

                if (auto ret = avcodec_send_packet(m_in_main_av_ctx, pkt);
                    ret != 0 && ret != AVERROR(EAGAIN)) {
                    av_packet_unref(pkt);
                    LOGW("main audio frame decoding error: "
                         << ret << " " << str_from_av_error(ret));
                    continue;
                }

                av_packet_unref(pkt);

                if (auto ret =
                        avcodec_receive_frame(m_in_main_av_ctx, frame_in);
                    ret != 0) {
                    if (ret == AVERROR(EAGAIN)) continue;

                    throw std::runtime_error("avcodec_receive_frame error");
                }

                got_frame = filter_frame(m_av_filter_ctx.src_main_ctx, frame_in,
                                         frame_out);

                break;
            }

            if (!got_frame && !m_data_info.filter_eof && m_data_info.in_eof &&
                m_data_info.main_in_eof) {
                LOGD("sending eof to filter");
                got_frame = filter_frame(m_av_filter_ctx.src_main_ctx, nullptr,
                                         frame_out);
            }
        }
    }

    if (got_frame && m_out_av_ctx->codec_type != AVMEDIA_TYPE_SUBTITLE) {
        av_channel_layout_copy(&frame_out->ch_layout, &m_out_av_ctx->ch_layout);
        frame_out->format = m_out_av_ctx->sample_fmt;
        frame_out->sample_rate = m_out_av_ctx->sample_rate;
        frame_out->time_base = m_out_av_ctx->time_base;
    }

    return got_frame;
}

bool media_compressor::decode_frame(AVPacket* pkt, AVFrame* frame_in,
                                    AVFrame* frame_out, AVSubtitle* subtitle) {
    if (m_out_av_ctx->frame_size > 0 &&
        m_out_av_ctx->codec_type !=
            AVMEDIA_TYPE_SUBTITLE) {  // encoder needs fix frame size
        return decode_frame_fix_size(pkt, frame_in, frame_out);
    }

    return decode_frame_var_size(pkt, frame_in, frame_out, subtitle);
}

bool media_compressor::filter_frame(AVFilterContext* src_ctx, AVFrame* frame_in,
                                    AVFrame* frame_out) {
    if (av_buffersrc_add_frame_flags(src_ctx, frame_in,
                                     AV_BUFFERSRC_FLAG_PUSH) < 0) {
        av_frame_unref(frame_in);
        throw std::runtime_error("av_buffersrc_add_frame_flags error");
    }

    av_frame_unref(frame_in);

    auto ret = av_buffersink_get_frame(m_av_filter_ctx.sink_ctx, frame_out);

    if (ret == AVERROR(EAGAIN)) return false;

    if (ret == AVERROR_EOF) {
        LOGD("filter eof");
        m_data_info.filter_eof = true;
        return false;
    }

    if (ret < 0) {
        av_frame_unref(frame_out);
        throw std::runtime_error("audio av_buffersink_get_frame error");
    }

    return true;
}

bool media_compressor::encode_subtitle(AVPacket* pkt, AVSubtitle* subtitle) {
    if (subtitle == nullptr) {
        LOGD("encoder eof");
        m_data_info.in_eof = true;
        m_data_info.filter_eof = false;
        m_data_info.eof = true;
        return false;
    }

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
    pkt->stream_index = m_out_stream_idx;

    return true;
}

bool media_compressor::encode_frame(AVFrame* frame, AVPacket* pkt) {
    auto ret = avcodec_send_frame(m_out_av_ctx, frame);

    av_frame_unref(frame);

    if (ret == AVERROR_EOF) {
        LOGD("encoder flushed");
    } else if (ret == AVERROR(EAGAIN)) {
        LOGD("encoder send again");
    } else if (ret < 0) {
        LOGW("avcodec_send_frame error: " << ret << " "
                                          << str_from_av_error(ret));
        return false;
    }

    if (auto ret = avcodec_receive_packet(m_out_av_ctx, pkt); ret != 0) {
        if (ret == AVERROR(EAGAIN)) {
            return false;
        } else if (ret == AVERROR_EOF) {
            LOGD("encoder eof");
            m_data_info.in_eof = true;
            m_data_info.filter_eof = true;
            m_data_info.eof = true;
            return false;
        }

        throw std::runtime_error("audio avcodec_receive_packet error");
    }

    if (m_out_av_ctx) {
        pkt->stream_index = m_out_stream_idx;
        pkt->time_base = m_out_av_ctx->time_base;
        pkt->pos = -1;
    } else {
        auto in_tb = m_in_av_format_ctx->streams[m_in_stream_idx]->time_base;
        auto out_tb = m_out_av_format_ctx->streams[m_out_stream_idx]->time_base;
        pkt->duration = av_rescale_q(pkt->duration, in_tb, out_tb);
        pkt->stream_index = m_out_stream_idx;
        pkt->time_base = out_tb;
        pkt->pos = -1;
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

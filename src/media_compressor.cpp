/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
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

static const char* str_from_av_error(int av_err) {
    static std::array<char, 1024> buf;

    return av_make_error_string(buf.data(), buf.size(), av_err);
}

static std::string str_for_av_opts(const AVDictionary* opts) {
    if (!opts) return {};

    std::ostringstream os;

    AVDictionaryEntry* t = nullptr;
    while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX))) {
        os << "[" << t->key << "=" << t->value << "],";
    }

    return os.str();
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

[[maybe_unused]] static void clean_av_opts(AVDictionary** opts) {
    if (*opts != nullptr) {
        LOGW("rejected av options: " << str_for_av_opts(*opts));
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
        case media_compressor::format_t::ogg:
            os << "ogg";
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
    if (ext == "ogg" || ext == "oga") return format_t::ogg;

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
        if (m_out_av_format_ctx->pb) avio_closep(&m_out_av_format_ctx->pb);
        avformat_free_context(m_out_av_format_ctx);
        m_out_av_format_ctx = nullptr;
    }
    if (m_out_av_audio_ctx) avcodec_free_context(&m_out_av_audio_ctx);

    if (m_in_av_audio_ctx) avcodec_free_context(&m_in_av_audio_ctx);

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

    if (m_in_av_audio_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
        av_channel_layout_default(&m_in_av_audio_ctx->ch_layout,
                                  m_in_av_audio_ctx->ch_layout.nb_channels);

    std::array<char, 512> src_args{};
    auto s = snprintf(
        src_args.data(), src_args.size(),
        "sample_rate=%d:sample_fmt=%s:time_base=%d/%d:channel_layout=",
        m_in_av_audio_ctx->sample_rate,
        av_get_sample_fmt_name(m_in_av_audio_ctx->sample_fmt),
        m_in_av_audio_ctx->time_base.num, m_in_av_audio_ctx->time_base.den);
    av_channel_layout_describe(&m_in_av_audio_ctx->ch_layout, &src_args.at(s),
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

    const AVSampleFormat out_sample_fmts[] = {m_out_av_audio_ctx->sample_fmt,
                                              AV_SAMPLE_FMT_NONE};
    if (av_opt_set_int_list(m_av_filter_ctx.sink_ctx, "sample_fmts",
                            out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN) < 0) {
        clean_av();
        throw std::runtime_error("av_opt_set_int_list error");
    }

    std::array<char, 512> chname{};
    av_channel_layout_describe(&m_out_av_audio_ctx->ch_layout, chname.data(),
                               chname.size());
    if (av_opt_set(m_av_filter_ctx.sink_ctx, "ch_layouts", chname.data(),
                   AV_OPT_SEARCH_CHILDREN) < 0) {
        clean_av();
        throw std::runtime_error("av_opt_set error");
    }

    const int out_sample_rates[] = {m_out_av_audio_ctx->sample_rate, -1};
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

void media_compressor::init_av_in_format(const std::string& input_file) {
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

    auto in_stream_idx = av_find_best_stream(
        m_in_av_format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (in_stream_idx < 0) {
        clean_av();
        throw std::runtime_error("no audio stream found in input file");
    }

    m_in_audio_stream_idx = in_stream_idx;
}

void media_compressor::init_av(task_t task) {
    init_av_in_format(m_input_files.front());
    m_input_files.pop();

    const auto* in_stream = m_in_av_format_ctx->streams[m_in_audio_stream_idx];

    bool same_codec = [&]() {
        switch (task) {
            case task_t::compress:
                switch (m_format) {
                    case format_t::mp3:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_MP3;
                    case format_t::ogg:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_VORBIS;
                    case format_t::wav:
                        return in_stream->codecpar->codec_id ==
                               AVCodecID::AV_CODEC_ID_PCM_S16LE;
                    case format_t::unknown:
                        break;
                }
                break;
            case task_t::decompress:
            case task_t::decompress_async:
                break;
        }
        return false;
    }();

    if (same_codec) {
        LOGD("in and out have the same codec: "
             << in_stream->codecpar->codec_id);
    } else {
        LOGD("input codec: " << in_stream->codecpar->codec_id);

        const auto* decoder =
            avcodec_find_decoder(in_stream->codecpar->codec_id);
        if (!decoder) {
            clean_av();
            throw std::runtime_error("avcodec_find_decoder error");
        }

        //    LOGD("sample fmts supported by audio decoder:");
        //    for (int i = 0; decoder->sample_fmts[i] != AV_SAMPLE_FMT_NONE;
        //    ++i) {
        //        LOGD("[" << i << "]: " << decoder->sample_fmts[i]);
        //    }

        m_in_av_audio_ctx = avcodec_alloc_context3(decoder);
        if (!m_in_av_audio_ctx) {
            clean_av();
            throw std::runtime_error("avcodec_alloc_context3 error");
        }

        if (avcodec_parameters_to_context(m_in_av_audio_ctx,
                                          in_stream->codecpar) < 0) {
            clean_av();
            throw std::runtime_error("avcodec_parameters_to_context error");
        }

        m_in_av_audio_ctx->time_base =
            AVRational{1, m_in_av_audio_ctx->sample_rate};

        if (avcodec_open2(m_in_av_audio_ctx, nullptr, nullptr) != 0) {
            clean_av();
            throw std::runtime_error("avcodec_open2 error");
        }

        //    LOGD("audio decoder: sample fmt="
        //         << m_in_av_audio_ctx->sample_fmt
        //         << ", channel_layout=" << &m_in_av_audio_ctx->ch_layout
        //         << ", sample_rate=" << m_in_av_audio_ctx->sample_rate);

        m_av_fifo =
            av_audio_fifo_alloc(m_in_av_audio_ctx->sample_fmt,
                                m_in_av_audio_ctx->ch_layout.nb_channels, 1);
        if (!m_av_fifo) {
            clean_av();
            throw std::runtime_error("av_audio_fifo_alloc error");
        }

        auto encoder_name = [&]() {
            switch (task) {
                case task_t::compress:
                    switch (m_format) {
                        case format_t::mp3:
                            return "libmp3lame";
                        case format_t::ogg:
                            return "libvorbis";
                        case format_t::wav:
                        case format_t::unknown:
                            break;
                    }
                    break;
                case task_t::decompress:
                case task_t::decompress_async:
                    break;
            }

            return "pcm_s16le";
        }();

        const auto* encoder = avcodec_find_encoder_by_name(encoder_name);
        if (!encoder) {
            clean_av();
            throw std::runtime_error("no audio encoder");
        }

        //    LOGD("sample fmts supported by audio encoder:");
        //    for (int i = 0; encoder->sample_fmts[i] != AV_SAMPLE_FMT_NONE;
        //    ++i) {
        //        LOGD("[" << i << "]: " << encoder->sample_fmts[i]);
        //    }

        m_out_av_audio_ctx = avcodec_alloc_context3(encoder);
        if (!m_out_av_audio_ctx) {
            clean_av();
            throw std::runtime_error("avcodec_alloc_context3 error");
        }

        if (task == task_t::compress) {
            m_out_av_audio_ctx->sample_fmt =
                best_sample_format(encoder, m_in_av_audio_ctx->sample_fmt);
            av_channel_layout_default(
                &m_out_av_audio_ctx->ch_layout,
                m_in_av_audio_ctx->ch_layout.nb_channels == 1 ? 1 : 2);
            m_out_av_audio_ctx->sample_rate = m_in_av_audio_ctx->sample_rate;
            m_out_av_audio_ctx->time_base =
                AVRational{1, m_out_av_audio_ctx->sample_rate};

            m_out_av_audio_ctx->flags |= AV_CODEC_FLAG_QSCALE;

            switch (m_quality) {
                case quality_t::vbr_high:
                    m_out_av_audio_ctx->global_quality =
                        FF_QP2LAMBDA * (m_format == format_t::mp3 ? 0 : 10);
                    break;
                case quality_t::vbr_medium:
                    m_out_av_audio_ctx->global_quality =
                        FF_QP2LAMBDA * (m_format == format_t::mp3 ? 4 : 3);
                    break;
                case quality_t::vbr_low:
                    m_out_av_audio_ctx->global_quality =
                        FF_QP2LAMBDA * (m_format == format_t::mp3 ? 9 : 0);
                    break;
            }
        } else {
            m_out_av_audio_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
            m_out_av_audio_ctx->sample_rate = 16000;
            av_channel_layout_default(&m_out_av_audio_ctx->ch_layout, 1);
        }

        if (avcodec_open2(m_out_av_audio_ctx, encoder, nullptr) < 0) {
            clean_av();
            throw std::runtime_error("avcodec_open2 error");
        }

        if (m_out_av_audio_ctx->sample_fmt != m_in_av_audio_ctx->sample_fmt) {
            LOGD("resampling needed: " << m_in_av_audio_ctx->sample_fmt
                                       << " => "
                                       << m_out_av_audio_ctx->sample_fmt);
        }

        init_av_filter("anull");
    }

    auto* format_name = [&]() {
        switch (task) {
            case task_t::compress:
                switch (m_format) {
                    case format_t::mp3:
                        return "mp3";
                    case format_t::ogg:
                        return "ogg";
                    case format_t::wav:
                    case format_t::unknown:
                        break;
                }
                break;
            case task_t::decompress_async:
                return "";
            case task_t::decompress:
                break;
        }
        return "wav";
    }();

    if (task != task_t::decompress_async) {
        if (avformat_alloc_output_context2(&m_out_av_format_ctx, nullptr,
                                           format_name, nullptr) < 0) {
            clean_av();
            throw std::runtime_error("avformat_alloc_output_context2 error");
        }

        auto* out_stream = avformat_new_stream(m_out_av_format_ctx, nullptr);
        if (!out_stream) {
            clean_av();
            throw std::runtime_error("avformat_new_stream error");
        }

        out_stream->id = 0;

        if (same_codec) {
            if (avcodec_parameters_copy(out_stream->codecpar,
                                        in_stream->codecpar) < 0) {
                clean_av();
                throw std::runtime_error("avcodec_parameters_copy error");
            }
            out_stream->time_base = in_stream->time_base;
        } else {
            if (avcodec_parameters_from_context(out_stream->codecpar,
                                                m_out_av_audio_ctx) < 0) {
                clean_av();
                throw std::runtime_error(
                    "avcodec_parameters_from_context error");
            }
            out_stream->time_base = m_out_av_audio_ctx->time_base;
        }

        // av_dump_format(m_out_av_format_ctx, out_stream->id, "", 1);

        if (avio_open(&m_out_av_format_ctx->pb, m_output_file.c_str(),
                      AVIO_FLAG_WRITE) < 0) {
            clean_av();
            throw std::runtime_error("avio_open error");
        }
    }
}

bool media_compressor::is_media_file(const std::string& input_file) {
    try {
        init_av_in_format(input_file);
    } catch (const std::runtime_error& err) {
        return false;
    }

    return true;
}

void media_compressor::decompress(std::vector<std::string> input_files,
                                  std::string output_file) {
    LOGD("task decompress");

    if (input_files.empty()) throw std::runtime_error("empty input file list");

    std::for_each(input_files.begin(), input_files.end(),
                  [&](auto& file) { m_input_files.push(std::move(file)); });

    m_output_file = std::move(output_file);

    init_av(task_t::decompress);

    process();
}

void media_compressor::decompress_async(std::vector<std::string> input_files) {
    LOGD("task decompress async");

    if (input_files.empty()) throw std::runtime_error("empty input file list");

    m_data_info = data_info{};
    std::for_each(input_files.begin(), input_files.end(), [&](auto& file) {
        struct stat st;
        if (stat(file.c_str(), &st) == 0) m_data_info.total += st.st_size;

        m_input_files.push(std::move(file));
    });

    init_av(task_t::decompress_async);

    if (m_async_thread.joinable()) m_async_thread.join();

    m_async_thread = std::thread([this]() {
        try {
            m_error = false;

            LOGD("process started");
            process();
            LOGD("process finished");
        } catch (const std::runtime_error& err) {
            LOGE("exception in process: " << err.what());
            m_error = true;
        }
    });
}

void media_compressor::compress_async(
    std::vector<std::string> input_files, std::string output_file,
    format_t format, quality_t quality,
    task_finished_callback_t task_finished_callback) {
    if (!task_finished_callback)
        throw std::runtime_error{"callback not provided"};

    compress_internal(std::move(input_files), std::move(output_file), format,
                      quality, std::move(task_finished_callback));
}

void media_compressor::compress(std::vector<std::string> input_files,
                                std::string output_file, format_t format,
                                quality_t quality) {
    compress_internal(std::move(input_files), std::move(output_file), format,
                      quality, task_finished_callback_t{});
}

void media_compressor::compress_internal(
    std::vector<std::string> input_files, std::string output_file,
    format_t format, quality_t quality,
    task_finished_callback_t task_finished_callback) {
    LOGD("task compress: format=" << format << ", quality=" << quality
                                  << ", async="
                                  << static_cast<bool>(task_finished_callback));

    if (input_files.empty()) throw std::runtime_error{"empty input file list"};

    std::for_each(input_files.begin(), input_files.end(),
                  [&](auto& file) { m_input_files.push(std::move(file)); });

    m_format = format;
    if (format == format_t::unknown)
        m_format = format_from_filename(m_output_file);
    if (m_format == format_t::unknown)
        throw std::runtime_error("unknown format requested");
    m_quality = quality;

    m_output_file = std::move(output_file);

    init_av(task_t::compress);

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

                callback();
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

    int64_t next_ts = 0;
    m_data_info.eof = false;

    bool no_decode = m_out_av_format_ctx &&
                     m_in_av_format_ctx->streams[m_in_audio_stream_idx]
                             ->codecpar->codec_id ==
                         m_out_av_format_ctx->streams[0]->codecpar->codec_id;

    while (!m_shutdown) {
        if (no_decode) {
            if (!read_frame(pkt)) break;
        } else {
            auto* frame{frame_in};

            if (!decode_frame(pkt, frame)) frame = nullptr;

            if (m_shutdown) break;

            filter_frame(frame, frame_out);

            if (frame) frame = frame_out;

            if (!encode_frame(frame, pkt)) {
                if (frame) continue;
                break;
            }
        }

        if (m_out_av_format_ctx) {
            av_packet_rescale_ts(
                pkt,
                m_in_av_format_ctx->streams[m_in_audio_stream_idx]->time_base,
                m_out_av_format_ctx->streams[0]->time_base);

            pkt->stream_index = 0;
            pkt->pts = next_ts;
            pkt->dts = next_ts;
            next_ts += pkt->duration;

            if (auto ret = av_write_frame(m_out_av_format_ctx, pkt); ret < 0)
                throw std::runtime_error("av_write_frame error");
        } else {
            if (pkt->size > BUF_MAX_SIZE)
                throw std::runtime_error("pkt size too large: " +
                                         std::to_string(pkt->size));

            std::unique_lock lock{m_mtx};
            m_cv.wait(lock, [&]() {
                return m_shutdown || m_buff.size() + pkt->size <= BUF_MAX_SIZE;
            });

            if (m_shutdown) break;

            auto old_size = m_buff.size();
            m_buff.resize(m_buff.size() + pkt->size);
            memcpy(std::next(m_buff.data(), old_size), pkt->data, pkt->size);
        }

        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_frame_free(&frame_in);
    av_frame_free(&frame_out);

    if (m_out_av_format_ctx) {
        if (av_write_trailer(m_out_av_format_ctx) < 0)
            LOGW("av_write_trailer error");

        avio_closep(&m_out_av_format_ctx->pb);

        if (m_shutdown) {
            unlink(m_output_file.c_str());
        }
    }
}

bool media_compressor::read_frame(AVPacket* pkt) {
    while (true) {
        if (auto ret = av_read_frame(m_in_av_format_ctx, pkt); ret != 0) {
            if (ret == AVERROR_EOF) {
                if (m_input_files.empty()) {
                    LOGD("demuxer eof");
                    m_data_info.eof = true;
                    return false;
                } else {
                    init_av_in_format(m_input_files.front());
                    m_input_files.pop();
                    continue;
                }
            }

            throw std::runtime_error("av_read_frame error");
        }

        return true;
    }
}

bool media_compressor::decode_frame(AVPacket* pkt, AVFrame* frame) {
    if (m_out_av_audio_ctx->frame_size > 0) {  // decoder needs fix frame size
        while (!m_shutdown &&
               av_audio_fifo_size(m_av_fifo) < m_out_av_audio_ctx->frame_size) {
            if (auto ret = av_read_frame(m_in_av_format_ctx, pkt); ret != 0) {
                if (ret == AVERROR_EOF) {
                    if (m_input_files.empty()) {
                        LOGD("demuxer eof");
                        m_data_info.eof = true;
                        return false;
                    } else {
                        init_av_in_format(m_input_files.front());
                        m_input_files.pop();
                        continue;
                    }
                }

                throw std::runtime_error("av_read_frame error");
            }

            if (pkt->stream_index != m_in_audio_stream_idx) {
                av_packet_unref(pkt);
                continue;
            }

            if (auto ret = avcodec_send_packet(m_in_av_audio_ctx, pkt);
                ret != 0 && ret != AVERROR(EAGAIN)) {
                av_packet_unref(pkt);
                LOGW("audio decoding error: " << ret << " "
                                              << str_from_av_error(ret));
                continue;
            }

            av_packet_unref(pkt);

            if (auto ret = avcodec_receive_frame(m_in_av_audio_ctx, frame);
                ret != 0) {
                if (ret == AVERROR(EAGAIN)) continue;

                throw std::runtime_error("avcodec_receive_frame error");
            }

            if (av_audio_fifo_realloc(m_av_fifo, av_audio_fifo_size(m_av_fifo) +
                                                     frame->nb_samples) < 0)
                throw std::runtime_error("av_audio_fifo_realloc error");

            if (av_audio_fifo_write(m_av_fifo,
                                    reinterpret_cast<void**>(frame->data),
                                    frame->nb_samples) < frame->nb_samples)
                throw std::runtime_error("av_audio_fifo_write error");

            av_frame_unref(frame);
        }

        frame->nb_samples = m_out_av_audio_ctx->frame_size;
        av_channel_layout_copy(&frame->ch_layout,
                               &m_in_av_audio_ctx->ch_layout);
        frame->format = m_in_av_audio_ctx->sample_fmt;
        frame->sample_rate = m_in_av_audio_ctx->sample_rate;

        if (av_frame_get_buffer(frame, 0) != 0)
            throw std::runtime_error("av_frame_get_buffer error");

        if (av_audio_fifo_read(m_av_fifo, reinterpret_cast<void**>(frame->data),
                               m_out_av_audio_ctx->frame_size) <
            m_out_av_audio_ctx->frame_size) {
            throw std::runtime_error("av_audio_fifo_read error");
        }
    } else {
        while (!m_shutdown) {
            if (auto ret = av_read_frame(m_in_av_format_ctx, pkt); ret != 0) {
                if (ret == AVERROR_EOF) {
                    if (m_input_files.empty()) {
                        LOGD("demuxer eof");
                        m_data_info.eof = true;
                        return false;
                    } else {
                        init_av_in_format(m_input_files.front());
                        m_input_files.pop();
                        continue;
                    }
                }

                throw std::runtime_error("av_read_frame error");
            }

            if (pkt->stream_index != m_in_audio_stream_idx) {
                av_packet_unref(pkt);
                continue;
            }

            if (auto ret = avcodec_send_packet(m_in_av_audio_ctx, pkt);
                ret != 0 && ret != AVERROR(EAGAIN)) {
                av_packet_unref(pkt);
                LOGW("audio frame decoding error: " << ret << " "
                                                    << str_from_av_error(ret));
                continue;
            }

            av_packet_unref(pkt);

            if (auto ret = avcodec_receive_frame(m_in_av_audio_ctx, frame);
                ret != 0) {
                if (ret == AVERROR(EAGAIN)) continue;

                throw std::runtime_error("avcodec_receive_frame error");
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

bool media_compressor::encode_frame(AVFrame* frame, AVPacket* pkt) {
    if (auto ret = avcodec_send_frame(m_out_av_audio_ctx, frame);
        ret != 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        av_frame_unref(frame);
        LOGW("avcodec_send_frame error: " << ret << " "
                                          << str_from_av_error(ret));
        return false;
    }

    av_frame_unref(frame);

    if (auto ret = avcodec_receive_packet(m_out_av_audio_ctx, pkt); ret != 0) {
        if (ret == AVERROR(EAGAIN)) return false;
        if (ret == AVERROR_EOF) {
            LOGD("encoder eof");
            return false;
        }

        throw std::runtime_error("audio avcodec_receive_packet error");
    }

    return true;
}

media_compressor::data_info media_compressor::get_data(char* data,
                                                       size_t max_size) {
    std::unique_lock lock{m_mtx};

    m_data_info.size = std::min(max_size, m_buff.size());

    if (m_data_info.size > 0) {
        memcpy(data, m_buff.data(), m_data_info.size);
        memmove(m_buff.data(), m_buff.data() + m_data_info.size,
                m_buff.size() - m_data_info.size);
        m_buff.resize(m_buff.size() - m_data_info.size);
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

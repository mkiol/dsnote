/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MEDIA_COMPRESSOR_HPP
#define MEDIA_COMPRESSOR_HPP

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
}

class media_compressor {
   public:
    enum class format_t { unknown, wav, mp3, ogg };
    friend std::ostream& operator<<(std::ostream& os, format_t format);

    enum class quality_t { vbr_high, vbr_medium, vbr_low };
    friend std::ostream& operator<<(std::ostream& os, quality_t quality);

    using task_finished_callback_t = std::function<void()>;

    struct data_info {
        size_t size = 0;
        uint64_t bytes_read = 0;
        uint64_t total = 0;
        bool eof = false;
    };

    media_compressor() = default;
    ~media_compressor();
    bool is_media_file(const std::string& input_file);
    void compress(std::vector<std::string> input_files, std::string output_file,
                  format_t format, quality_t quality);
    void compress_async(std::vector<std::string> input_files,
                        std::string output_file, format_t format,
                        quality_t quality,
                        task_finished_callback_t task_finished_callback);
    void decompress(std::vector<std::string> input_files,
                    std::string output_file);
    void decompress_async(std::vector<std::string> input_files);
    data_info get_data(char* data, size_t max_size);
    void cancel();
    inline bool error() const { return m_error; }

   private:
    enum class task_t { compress, decompress, decompress_async };

    struct filter_ctx {
        AVFilterInOut* out = nullptr;
        AVFilterInOut* in = nullptr;
        AVFilterContext* src_ctx = nullptr;
        AVFilterContext* sink_ctx = nullptr;
        AVFilterGraph* graph = nullptr;
    };

    static const int BUF_MAX_SIZE = 8192;

    std::queue<std::string> m_input_files;
    std::string m_output_file;
    format_t m_format = format_t::unknown;
    quality_t m_quality = quality_t::vbr_medium;
    AVFormatContext* m_in_av_format_ctx = nullptr;
    AVFormatContext* m_out_av_format_ctx = nullptr;
    AVCodecContext* m_in_av_audio_ctx = nullptr;
    AVCodecContext* m_out_av_audio_ctx = nullptr;
    filter_ctx m_av_filter_ctx;
    AVAudioFifo* m_av_fifo = nullptr;
    int m_in_audio_stream_idx = 0;
    bool m_shutdown = false;
    std::thread m_async_thread;
    std::condition_variable m_cv;
    std::mutex m_mtx;
    bool m_error = false;
    std::vector<char> m_buff;
    data_info m_data_info;

    void init_av(task_t task);
    void init_av_filter(const char* arg);
    void init_av_in_format(const std::string& input_file);
    void clean_av();
    void clean_av_in_format();
    void process();
    bool read_frame(AVPacket* pkt);
    bool decode_frame(AVPacket* pkt, AVFrame* frame);
    bool encode_frame(AVFrame* frame, AVPacket* pkt);
    bool filter_frame(AVFrame* frame_in, AVFrame* frame_out);
    static format_t format_from_filename(const std::string& filename);
    void compress_internal(std::vector<std::string> input_files,
                           std::string output_file, format_t format,
                           quality_t quality,
                           task_finished_callback_t task_finished_callback);
};

#endif // MEDIA_COMPRESSOR_HPP

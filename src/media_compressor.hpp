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
    enum class format_t { unknown, wav, mp3, ogg_vorbis, ogg_opus, flac };
    friend std::ostream& operator<<(std::ostream& os, format_t format);

    enum class quality_t { vbr_high, vbr_medium, vbr_low };
    friend std::ostream& operator<<(std::ostream& os, quality_t quality);

    struct options_t {
        bool mono = false;
        bool sample_rate_16 = false; /*smaple-rate = 16KHz*/
    };

    using task_finished_callback_t = std::function<void()>;
    using data_ready_callback_t = std::function<void()>;

    static const int BUF_MAX_SIZE = 16384;

    struct data_info_t {
        size_t size = 0;
        uint64_t bytes_read = 0;
        uint64_t total = 0;
        bool eof = false;
    };

    struct clip_info_t {
        static const uint64_t max = std::numeric_limits<uint64_t>::max();

        uint64_t start_time_ms = 0;
        uint64_t stop_time_ms = max;
        uint64_t start_bytes = 0;
        uint64_t stop_bytes = max;

        inline bool valid_time() const { return start_time_ms < stop_time_ms; }
        inline bool valid_bytes() const { return start_bytes < stop_bytes; }
    };

    media_compressor() = default;
    ~media_compressor();
    bool is_media_file(const std::string& input_file);
    size_t duration(const std::string& input_file);
    void compress(std::vector<std::string> input_files, std::string output_file,
                  format_t format, quality_t quality,
                  clip_info_t clip_info = {0, 0, 0, 0});
    void compress_async(std::vector<std::string> input_files,
                        std::string output_file, format_t format,
                        quality_t quality,
                        task_finished_callback_t task_finished_callback);
    void decompress(std::vector<std::string> input_files,
                    std::string output_file, options_t options);
    void decompress_to_raw_async(
        std::vector<std::string> input_files, options_t options,
        data_ready_callback_t data_ready_callback,
        task_finished_callback_t task_finished_callback);
    void decompress_to_wav_async(
        std::vector<std::string> input_files, options_t options,
        data_ready_callback_t data_ready_callback,
        task_finished_callback_t task_finished_callback);
    data_info_t get_data(char* data, size_t max_size);
    size_t data_size() const;
    void cancel();
    inline bool error() const { return m_error; }

   private:
    enum class task_t {
        compress,
        decompress,
        decompress_raw_async,
        decompress_wav_async
    };

    struct filter_ctx {
        AVFilterInOut* out = nullptr;
        AVFilterInOut* in = nullptr;
        AVFilterContext* src_ctx = nullptr;
        AVFilterContext* sink_ctx = nullptr;
        AVFilterGraph* graph = nullptr;
    };

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
    std::vector<char> m_buf;
    data_info_t m_data_info;
    clip_info_t m_clip_info;
    options_t m_options;
    bool m_no_decode = false;
    data_ready_callback_t m_data_ready_callback;
    uint64_t m_in_bytes_read = 0;

    void init_av(task_t task);
    void init_av_filter(const char* arg);
    void init_av_in_format(const std::string& input_file);
    void clean_av();
    void clean_av_in_format();
    void process();
    bool read_frame(AVPacket* pkt);
    bool decode_frame(AVPacket* pkt, AVFrame* frame_in, AVFrame* frame_out);
    bool encode_frame(AVFrame* frame, AVPacket* pkt);
    bool filter_frame(AVFrame* frame_in, AVFrame* frame_out);
    static format_t format_from_filename(const std::string& filename);
    void compress_internal(std::vector<std::string> input_files,
                           std::string output_file, format_t format,
                           quality_t quality,
                           task_finished_callback_t task_finished_callback);
    void setup_input_files(std::vector<std::string>&& input_files);
    void decompress_async_internal(
        task_t task, std::vector<std::string>&& input_files, options_t options,
        data_ready_callback_t&& data_ready_callback,
        task_finished_callback_t&& task_finished_callback);
    void write_to_buf(const char* data, int size);
    static int write_packet_callback(void* opaque, uint8_t* buf, int buf_size);
};

#endif // MEDIA_COMPRESSOR_HPP

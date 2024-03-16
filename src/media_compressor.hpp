/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MEDIA_COMPRESSOR_HPP
#define MEDIA_COMPRESSOR_HPP

#include <array>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <functional>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <utility>
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
    enum class format_t {
        unknown,
        audio_wav,
        audio_mp3,
        audio_ogg_vorbis,
        audio_ogg_opus,
        audio_flac,
        sub_srt,
        sub_ass,
        sub_vtt
    };
    friend std::ostream& operator<<(std::ostream& os, format_t format);

    enum class quality_t { vbr_high, vbr_medium, vbr_low };
    friend std::ostream& operator<<(std::ostream& os, quality_t quality);

    enum class media_type_t { unknown, audio, video, subtitles };
    friend std::ostream& operator<<(std::ostream& os, media_type_t media_type);

    struct stream_t {
        int index = -1;
        media_type_t media_type = media_type_t::unknown;
        std::string title;
        std::string language;
        int volume_change = 0; /* -30 dB <=> 30 dB */
    };
    friend std::ostream& operator<<(std::ostream& os, stream_t stream);

    struct media_info_t {
        std::vector<stream_t> audio_streams;
        std::vector<stream_t> video_streams;
        std::vector<stream_t> subtitles_streams;
    };
    friend std::ostream& operator<<(std::ostream& os, media_info_t media_info);

    struct clip_info_t {
        static const uint64_t max = std::numeric_limits<uint64_t>::max();

        uint64_t start_time_ms = 0;
        uint64_t stop_time_ms = max;
        uint64_t start_bytes = 0;
        uint64_t stop_bytes = max;

        inline bool valid_time() const { return start_time_ms < stop_time_ms; }
        inline bool valid_bytes() const { return start_bytes < stop_bytes; }
    };

    enum flags_t : unsigned int {
        flag_none = 0,
        flag_force_mono_output = 1 << 0,
        flag_force_16k_sample_rate_output = 1 << 1,
        flag_change_speed = 1 << 2
    };
    inline friend flags_t operator|(flags_t a, flags_t b) {
        return static_cast<flags_t>(static_cast<unsigned int>(a) |
                                    static_cast<unsigned int>(b));
    }

    struct options_t {
        quality_t quality = quality_t::vbr_medium;
        flags_t flags = flags_t::flag_none;
        double speed = 1.0;
        std::optional<stream_t> stream;
        std::optional<clip_info_t> clip_info;
    };

    using task_finished_callback_t = std::function<void()>;
    using data_ready_callback_t = std::function<void()>;
    using file_progress_callback_t = std::function<void(
        unsigned int processed_files, unsigned int total_files)>;

    static const int BUF_MAX_SIZE = 16384;

    struct data_info_t {
        size_t size = 0;
        uint64_t bytes_read = 0;
        uint64_t total = 0;
        bool eof = false;
        bool in_eof = false;
        bool main_in_eof = false;
        bool filter_eof = false;
    };

    media_compressor() = default;
    ~media_compressor();
    std::optional<media_info_t> media_info(const std::string& input_file);
    std::pair<size_t, unsigned int> duration_and_rate(
        const std::string& input_file);
    void compress_mix_to_file(std::string main_input_file,
                              std::vector<std::string> input_files,
                              std::string output_file, format_t format,
                              std::optional<options_t> options);
    void compress_mix_to_file_async(
        std::string main_input_file, std::vector<std::string> input_files,
        std::string output_file, format_t format,
        std::optional<options_t> options,
        file_progress_callback_t file_progress_callback,
        task_finished_callback_t task_finished_callback);
    void compress_to_file(std::vector<std::string> input_files,
                          std::string output_file, format_t format,
                          std::optional<options_t> options);
    void compress_to_file_async(
        std::vector<std::string> input_files, std::string output_file,
        format_t format, std::optional<options_t> options,
        file_progress_callback_t file_progress_callback,
        task_finished_callback_t task_finished_callback);
    void decompress_to_file(std::vector<std::string> input_files,
                            std::string output_file,
                            std::optional<options_t> options);
    void decompress_to_file_async(
        std::vector<std::string> input_files, std::string output_file,
        std::optional<options_t> options,
        task_finished_callback_t task_finished_callback);
    void decompress_to_data_raw_async(
        std::vector<std::string> input_files, std::optional<options_t> options,
        data_ready_callback_t data_ready_callback,
        task_finished_callback_t task_finished_callback);
    void decompress_to_data_format_async(
        std::vector<std::string> input_files, std::optional<options_t> options,
        data_ready_callback_t data_ready_callback,
        task_finished_callback_t task_finished_callback);
    data_info_t get_data(char* data, size_t max_size);
    std::pair<data_info_t, std::string> get_all_data();
    size_t data_size() const;
    void cancel();
    inline bool error() const { return m_error; }

   private:
    enum class task_t {
        compress_to_file,
        compress_mix_to_file,
        decompress_to_file,
        decompress_to_file_async,
        decompress_to_data_raw_async,
        decompress_to_data_format_async
    };

    struct filter_ctx {
        AVFilterContext* src_ctx = nullptr;
        AVFilterContext* src_main_ctx = nullptr;
        AVFilterContext* sink_ctx = nullptr;
        AVFilterGraph* graph = nullptr;
    };

    std::queue<std::string> m_input_files;
    std::string m_main_input_file;
    std::string m_output_file;
    format_t m_format = format_t::unknown;
    AVFormatContext* m_in_av_format_ctx = nullptr;
    AVFormatContext* m_in_main_av_format_ctx = nullptr;
    int m_in_stream_idx = -1;
    int m_in_main_stream_idx = -1;
    int m_out_stream_idx = -1;
    AVFormatContext* m_out_av_format_ctx = nullptr;
    AVCodecContext* m_in_av_ctx = nullptr;
    AVCodecContext* m_in_main_av_ctx = nullptr;
    AVCodecContext* m_out_av_ctx = nullptr;
    filter_ctx m_av_filter_ctx;
    AVAudioFifo* m_av_fifo = nullptr;
    bool m_shutdown = false;
    std::thread m_async_thread;
    std::condition_variable m_cv;
    std::mutex m_mtx;
    bool m_error = false;
    std::vector<char> m_buf;
    data_info_t m_data_info;
    std::optional<options_t> m_options;
    bool m_no_decode = false;
    data_ready_callback_t m_data_ready_callback;
    file_progress_callback_t m_file_progress_callback;
    uint64_t m_in_bytes_read = 0;
    unsigned int m_total_files_to_process = 0;

    void init_av(task_t task);
    void init_av_filter();
    void init_av_in_format(const std::string& input_file,
                           bool skip_stream_discovery);
    void init_av_in_main_format(const std::string& input_file);
    void clean_av();
    void clean_av_in_format();
    void process();
    bool read_main_frame(AVPacket* pkt);
    bool read_frame(AVPacket* pkt);
    bool decode_frame(AVPacket* pkt, AVFrame* frame_in, AVFrame* frame_out,
                      AVSubtitle* subtitle);
    bool decode_frame_fix_size(AVPacket* pkt, AVFrame* frame_in,
                               AVFrame* frame_out);
    bool decode_frame_var_size(AVPacket* pkt, AVFrame* frame_in,
                               AVFrame* frame_out, AVSubtitle* subtitle);
    bool encode_frame(AVFrame* frame, AVPacket* pkt);
    bool encode_subtitle(AVPacket* pkt, AVSubtitle* subtitle);
    bool filter_frame(AVFilterContext* src_ctx, AVFrame* frame_in,
                      AVFrame* frame_out);
    static format_t format_from_filename(const std::string& filename);
    void compress_internal(task_t task, std::string main_input_file,
                           std::vector<std::string> input_files,
                           std::string output_file, format_t format,
                           std::optional<options_t> options,
                           file_progress_callback_t file_progress_callback,
                           task_finished_callback_t task_finished_callback);
    void setup_input_files(std::vector<std::string>&& input_files);
    void decompress_async_internal(
        task_t task, std::vector<std::string>&& input_files,
        std::string output_file, std::optional<options_t> options,
        data_ready_callback_t&& data_ready_callback,
        task_finished_callback_t&& task_finished_callback);
    void write_to_buf(const char* data, int size);
    void setup_decoder(const AVStream* in_stream, AVCodecContext** in_av_ctx);
    static int write_packet_callback(void* opaque, uint8_t* buf, int buf_size);
    std::ofstream m_pcm_file;
};

#endif // MEDIA_COMPRESSOR_HPP

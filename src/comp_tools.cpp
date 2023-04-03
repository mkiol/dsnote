/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "comp_tools.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <lzma.h>
#include <zlib.h>

#include <fstream>

namespace comp_tools {

// source: https://github.com/libarchive/libarchive/blob/master/examples/untar.c
static int copy_data(struct archive* ar, struct archive* aw) {
    int r;
    const void* buff;
    size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
    int64_t offset;
#else
    off_t offset;
#endif

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF) return (ARCHIVE_OK);
        if (r != ARCHIVE_OK) return (r);
        r = archive_write_data_block(aw, buff, size, offset);
        if (r != ARCHIVE_OK) {
            return (r);
        }
    }
}

bool xz_decode(const QString& file_in, const QString& file_out) {
    qDebug() << "extracting xz archive:" << file_in;

    std::ifstream input{file_in.toStdString(),
                        std::ios::in | std::ifstream::binary};
    if (input.bad()) {
        qWarning() << "error opening in-file:" << file_in;
        return false;
    }

    std::ofstream output{file_out.toStdString(),
                         std::ios::out | std::ifstream::binary};
    if (output.bad()) {
        qWarning() << "error opening out-file:" << file_in;
        return false;
    }

    lzma_stream strm = LZMA_STREAM_INIT;
    if (lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, 0);
        ret != LZMA_OK) {
        qWarning() << "error initializing the xz decoder:" << ret;
        return false;
    }

    lzma_action action = LZMA_RUN;

    char buff_out[std::numeric_limits<unsigned short>::max()];
    char buff_in[std::numeric_limits<unsigned short>::max()];

    strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
    strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
    strm.avail_out = sizeof buff_out;
    strm.avail_in = 0;

    while (true) {
        if (strm.avail_in == 0 && input) {
            strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
            input.read(buff_in, sizeof buff_in);
            strm.avail_in = input.gcount();
        }

        if (!input) action = LZMA_FINISH;

        auto ret = lzma_code(&strm, action);

        if (strm.avail_out == 0 || ret == LZMA_STREAM_END) {
            output.write(buff_out, sizeof buff_out - strm.avail_out);

            strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
            strm.avail_out = sizeof buff_out;
        }

        if (ret == LZMA_STREAM_END) break;

        if (ret != LZMA_OK) {
            qWarning() << "xz decoder error:" << ret;
            lzma_end(&strm);
            return false;
        }
    }

    lzma_end(&strm);

    return true;
}

bool gz_decode(const QString& file_in, const QString& file_out) {
    qDebug() << "extracting gz archive:" << file_in;

    std::ifstream input{file_in.toStdString(),
                        std::ios::in | std::ifstream::binary};
    if (input.bad()) {
        qWarning() << "error opening in-file:" << file_in;
        return false;
    }

    std::ofstream output{file_out.toStdString(),
                         std::ios::out | std::ifstream::binary};
    if (output.bad()) {
        qWarning() << "error opening out-file:" << file_in;
        return false;
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (int ret = inflateInit2(&strm, MAX_WBITS | 16); ret != Z_OK) {
        qWarning() << "error initializing the gzip decoder:" << ret;
        return false;
    }

    char buff_out[std::numeric_limits<unsigned short>::max()];
    char buff_in[std::numeric_limits<unsigned short>::max()];

    strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
    strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
    strm.avail_out = sizeof buff_out;
    strm.avail_out = sizeof buff_out;
    strm.avail_in = 0;

    while (true) {
        if (strm.avail_in == 0 && input) {
            strm.next_in = reinterpret_cast<uint8_t*>(buff_in);
            input.read(buff_in, sizeof buff_in);
            strm.avail_in = input.gcount();
        }

        if (input.bad()) {
            inflateEnd(&strm);
            return false;
        }

        auto ret = inflate(&strm, Z_NO_FLUSH);

        if (strm.avail_out == 0 || ret == Z_STREAM_END) {
            output.write(buff_out, sizeof buff_out - strm.avail_out);

            strm.next_out = reinterpret_cast<uint8_t*>(buff_out);
            strm.avail_out = sizeof buff_out;
        }

        if (ret == Z_STREAM_END) break;

        if (ret != Z_OK) {
            qWarning() << "gzip decoder error:" << ret;
            inflateEnd(&strm);
            return false;
        }
    }

    inflateEnd(&strm);

    return true;
}

bool archive_decode(const QString& file_in, archive_type type,
                    files_to_extract&& files_out) {
    qDebug() << "extracting archive:" << file_in;

    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();

    switch (type) {
        case archive_type::tar:
            archive_read_support_format_tar(a);
            break;
        case archive_type::zip:
            archive_read_support_format_zip(a);
            break;
        default:
            throw std::runtime_error("unsupported archive type");
    }

    bool ok = true;

    if (archive_read_open_filename(a, file_in.toStdString().c_str(), 10240)) {
        qWarning() << "error opening in-file:" << file_in
                   << archive_error_string(a);
        ok = false;
    } else {
        archive_entry* entry{};

        while (true) {
            int ret = archive_read_next_header(a, &entry);
            if (ret == ARCHIVE_EOF) break;
            if (ret != ARCHIVE_OK) {
                qWarning() << "error archive_read_next_header:" << file_in
                           << archive_error_string(a);
                ok = false;
                break;
            }

            QString entry_path{archive_entry_pathname_utf8(entry)};

            qDebug() << "found file in archive:" << entry_path;

            auto file_out = [&]() -> QString {
                if (files_out.files.empty()) {
                    if (entry_path.endsWith('/')) return {};
                    auto split = entry_path.split('/');
                    if (split.size() < 2) return {};
                    split.first() = files_out.out_dir;
                    return split.join('/');
                }

                auto it = files_out.files.find(entry_path);
                if (it == files_out.files.cend()) return {};
                files_out.files.erase(it);
                return it->second;
            }();

            if (file_out.isEmpty()) continue;

            qDebug() << "extracting file:" << entry_path << "to" << file_out;

            auto file_out_std = file_out.toStdString();

            archive_entry_set_pathname(entry, file_out_std.c_str());

            ret = archive_write_header(ext, entry);
            if (ret != ARCHIVE_OK) {
                qWarning() << "error archive_write_header:" << file_in
                           << archive_error_string(ext);
                ok = false;
                break;
            }

            copy_data(a, ext);
            ret = archive_write_finish_entry(ext);
            if (ret != ARCHIVE_OK) {
                qWarning() << "error archive_write_finish_entry:" << file_in
                           << archive_error_string(ext);
                ok = false;
                break;
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return ok;
}
}  // namespace comp_tools

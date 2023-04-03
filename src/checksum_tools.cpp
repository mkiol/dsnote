/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "checksum_tools.hpp"

#include <zlib.h>

#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <cstdint>
#include <fstream>
#include <sstream>

static uint32_t make_file_checksum_number(const QString& file) {
    auto input =
        std::ifstream{file.toStdString(), std::ios::in | std::ifstream::binary};
    if (input.bad()) {
        qWarning() << "failed to open file:" << file;
        return 0;
    }

    uint32_t checksum = crc32(0L, Z_NULL, 0);

    char buff[std::numeric_limits<unsigned short>::max()];

    while (input) {
        input.read(buff, sizeof buff);
        checksum = crc32(checksum, reinterpret_cast<unsigned char*>(buff),
                         static_cast<unsigned int>(input.gcount()));
    }

    return checksum;
}

static uint32_t make_file_quick_checksum_number(const QString& file) {
    auto input =
        std::ifstream{file.toStdString(),
                      std::ios::in | std::ifstream::binary | std::ios::ate};

    if (input.bad()) {
        qWarning() << "failed to open file:" << file;
        return 0;
    }

    auto chunk = static_cast<std::ifstream::pos_type>(
        std::numeric_limits<unsigned short>::max());

    auto end_pos = input.tellg();

    if (end_pos < 2 * chunk) {
        // file too short, fallback to normal checksum
        input.close();
        return make_file_checksum_number(file);
    }

    uint32_t checksum = crc32(0L, Z_NULL, 0);

    char buff[std::numeric_limits<unsigned short>::max()];

    input.seekg(end_pos - chunk);
    input.read(buff, sizeof buff);
    checksum = crc32(checksum, reinterpret_cast<unsigned char*>(buff),
                     static_cast<unsigned int>(input.gcount()));

    input.seekg(0);
    input.read(buff, sizeof buff);
    checksum += crc32(checksum, reinterpret_cast<unsigned char*>(buff),
                      static_cast<unsigned int>(input.gcount()));

    return checksum;
}

static QString number_to_hex_str(uint32_t number) {
    std::stringstream ss;
    ss << std::hex << number;
    return QString::fromStdString(ss.str());
}

namespace checksum_tools {
QString make_checksum(const QString& file_or_dir) {
    if (QFileInfo{file_or_dir}.isDir()) return make_dir_checksum(file_or_dir);
    return make_file_checksum(file_or_dir);
}

QString make_file_checksum(const QString& file) {
    return number_to_hex_str(make_file_checksum_number(file));
}

QString make_dir_checksum(const QString& dir) {
    QDirIterator it{dir, QDir::Files | QDir::NoSymLinks | QDir::Readable,
                    QDirIterator::Subdirectories};

    uint64_t checksum = 0;

    while (it.hasNext()) {
        checksum += make_file_checksum_number(it.next());
    }

    return number_to_hex_str(checksum);
}

QString make_quick_checksum(const QString& file_or_dir) {
    if (QFileInfo{file_or_dir}.isDir())
        return make_dir_quick_checksum(file_or_dir);
    return make_file_quick_checksum(file_or_dir);
}

QString make_file_quick_checksum(const QString& file) {
    return number_to_hex_str(make_file_quick_checksum_number(file));
}

QString make_dir_quick_checksum(const QString& dir) {
    QDirIterator it{dir, QDir::Files | QDir::NoSymLinks | QDir::Readable,
                    QDirIterator::Subdirectories};

    uint64_t checksum = 0;

    while (it.hasNext()) {
        checksum += make_file_quick_checksum_number(it.next());
    }

    return number_to_hex_str(checksum);
}

}  // namespace checksum_tools

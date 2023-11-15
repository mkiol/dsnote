/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mtag_tools.hpp"

#include <taglib/fileref.h>
#include <taglib/tag.h>

#include "logger.hpp"

namespace mtag_tools {
bool write(const std::string &path, const mtag_t &mtag) {
    TagLib::FileRef file{path.c_str(), false};
    if (file.isNull()) {
        LOGE("taglib cannot open file: " << path);
        return false;
    }

    auto *tag = file.tag();
    if (!tag) {
        LOGE("taglib tag is null");
        return false;
    }

    if (!mtag.title.empty()) tag->setTitle({mtag.title, TagLib::String::UTF8});
    if (!mtag.artist.empty())
        tag->setArtist({mtag.artist, TagLib::String::UTF8});
    if (!mtag.album.empty()) tag->setAlbum({mtag.album, TagLib::String::UTF8});
    if (mtag.track > 0) tag->setTrack(mtag.track);

    file.save();

    return true;
}

std::optional<mtag_t> read(const std::string &path) {
    TagLib::FileRef file{path.c_str(), false};
    if (file.isNull()) {
        LOGE("taglib cannot open file: " << path);
        return std::nullopt;
    }

    auto *tag = file.tag();
    if (!tag) {
        LOGE("taglib tag is null");
        return std::nullopt;
    }

    mtag_t mtag;

    mtag.title = tag->title().toCString(true);
    mtag.artist = tag->artist().toCString(true);
    mtag.album = tag->album().toCString(true);
    mtag.track = tag->track();

    return mtag;
}

}  // namespace mtag_tools

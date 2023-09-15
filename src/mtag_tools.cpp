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
bool write(const std::string &path, const std::string &title,
           const std::string &artist, const std::string &album, int track) {
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

    if (!title.empty()) tag->setTitle({title, TagLib::String::UTF8});
    if (!artist.empty()) tag->setArtist({artist, TagLib::String::UTF8});
    if (!album.empty()) tag->setAlbum({album, TagLib::String::UTF8});
    if (track > 0) tag->setTrack(track);

    file.save();

    return true;
}
}  // namespace mtag_tools

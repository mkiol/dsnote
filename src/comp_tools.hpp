/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef COMP_TOOLS_HPP
#define COMP_TOOLS_HPP

#include <QDebug>
#include <QHash>
#include <QString>
#include <unordered_map>

#ifndef QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH
#define QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH
namespace std {
template <>
struct hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
        return static_cast<size_t>(qHash(s));
    }
};
}  // namespace std
#endif

namespace comp_tools {
enum class archive_type { tar, zip };

struct files_to_extract {
    QString out_dir;
    std::unordered_map<QString, QString> files;
};

bool xz_decode(const QString& file_in, const QString& file_out);
bool gz_decode(const QString& file_in, const QString& file_out);
bool archive_decode(const QString& file_in, archive_type type,
                    files_to_extract&& files_out, bool ignore_first_dir);
}  // namespace comp_tools

#endif // COMP_TOOLS_HPP

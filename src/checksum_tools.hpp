/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CHECKSUM_TOOLS_HPP
#define CHECKSUM_TOOLS_HPP

#include <QString>

namespace checksum_tools {
QString make_checksum(const QString& file_or_dir);
QString make_file_checksum(const QString& file);
QString make_dir_checksum(const QString& dir);
QString make_quick_checksum(const QString& file_or_dir);
QString make_file_quick_checksum(const QString& file);
QString make_dir_quick_checksum(const QString& file);
}  // namespace checksum_tools

#endif // CHECKSUM_TOOLS_HPP

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef MODULE_TOOLS_HPP
#define MODULE_TOOLS_HPP

#include <QString>

namespace module_tools {
QString path_in_share_dir(const QString& path);
QString path_to_share_dir_for_path(const QString& path);
bool unpack_module(const QString& name);
[[nodiscard]] bool module_unpacked(const QString& name);
bool init_module(const QString& name);
QString unpacked_dir(const QString& name);
QString module_file(const QString& name);
}  // namespace module_tools

#endif  // MODULE_TOOLS_HPP

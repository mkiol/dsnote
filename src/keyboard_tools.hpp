/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef KEYBOARD_TOOLS_CPP
#define KEYBOARD_TOOLS_CPP

#include <QDebug>
#include <QString>

namespace keyboard_tools {
void send_text(const QString &text);
}  // namespace keyboard_tools

#endif  // KEYBOARD_TOOLS_CPP

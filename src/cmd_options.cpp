/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QString>
#include "cmd_options.hpp"

namespace cmd {

bool validate_busy_policy_str(const QString& policy_str) {
#define X(name, str, ...)                                    \
    if (policy_str.compare(str, Qt::CaseInsensitive) == 0) { \
        return true;                                         \
    }
    ACTION_WHEN_BUSY_POLICY_TABLE
#undef X
    return false;
}

bool validate_action_str(const QString& action_str) {
#define X(name, str, ...)                                    \
    if (action_str.compare(str, Qt::CaseInsensitive) == 0) { \
        return true;                                         \
    }
    ACTION_TABLE
#undef X
    return false;
}

bool validate_text_format_str(const QString& text_format_str) {
#define X(name, str, ...)                                         \
    if (text_format_str.compare(str, Qt::CaseInsensitive) == 0) { \
        return true;                                              \
    }
    ACTION_TEXT_FORMAT_TABLE
#undef X
    return false;
}
}  // namespace cmd

/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GGML_TOOLS_H
#define GGML_TOOLS_H

// clang-format off
/*** copied from 'ggml.h' START ***/

extern "C" {
// Abort callback
// If not NULL, called before ggml computation
// If it returns true, the computation is aborted
typedef bool (*ggml_abort_callback)(void * data);

enum ggml_log_level {
    GGML_LOG_LEVEL_NONE  = 0,
    GGML_LOG_LEVEL_DEBUG = 1,
    GGML_LOG_LEVEL_INFO  = 2,
    GGML_LOG_LEVEL_WARN  = 3,
    GGML_LOG_LEVEL_ERROR = 4,
    GGML_LOG_LEVEL_CONT  = 5, // continue previous log
};

typedef void (*ggml_log_callback)(enum ggml_log_level level, const char * text, void * user_data);
}

/*** copied from 'ggml.h' END ***/
// clang-format on

#endif  // GGML_TOOLS_H

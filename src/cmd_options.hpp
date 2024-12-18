/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CMD_OPTIONS_HPP
#define CMD_OPTIONS_HPP

#include <QString>
#include <QStringList>
#include <cstdint>

#include "settings.h"

namespace cmd {
enum role_flag : uint8_t {
    role_none = 0,
    role_stt = 1 << 0,
    role_tts = 1 << 1
};
enum scope_flag : uint8_t {
    scope_none = 0,
    scope_general = 1 << 0,
    scope_task = 1 << 1
};

struct options {
    bool valid = true;
    settings::launch_mode_t launch_mode =
        settings::launch_mode_t::app_stanalone;
    bool verbose = false;
    bool gen_cheksums = false;
    bool hw_scan_off = false;
    bool py_scan_off = false;
    bool reset_models = false;
    bool start_in_tray = false;
    std::underlying_type_t<scope_flag> state_scope_to_print_flag = scope_none;
    std::underlying_type_t<role_flag> models_to_print_roles = role_none;
    std::underlying_type_t<role_flag> active_model_to_print_role = role_none;
    QString action;
    QString extra;
    QStringList files;
    QString log_file;
};
}  // namespace cmd

#endif // CMD_OPTIONS_HPP

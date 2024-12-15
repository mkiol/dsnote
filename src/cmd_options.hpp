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
enum model_type_flag : uint8_t { none = 0, stt = 1 << 0, tts = 1 << 1 };

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
    bool print_state = false;
    std::underlying_type_t<model_type_flag> model_list_types =
        model_type_flag::none;
    std::underlying_type_t<model_type_flag> active_model_types =
        model_type_flag::none;
    QString action;
    QString extra;
    QStringList files;
    QString log_file;
};
}  // namespace cmd

#endif // CMD_OPTIONS_HPP

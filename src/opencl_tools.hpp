/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef OPENCL_TOOLS_CPP
#define OPENCL_TOOLS_CPP

#include <string>
#include <vector>

namespace opencl_tools {
struct device {
    std::string platform_name;
    std::string device_name;
};

std::vector<device> available_devices();
}  // namespace opencl_tools

#endif  // OPENCL_TOOLS_CPP

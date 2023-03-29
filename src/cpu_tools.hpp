/* Copyright (C) 2032 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CPU_TOOLS_CPP
#define CPU_TOOLS_CPP

#include <iostream>

namespace cpu_tools {
enum class arch_t { unknown, x86_64, arm32, arm64 };

arch_t arch();
int number_of_cores();
bool neon_supported();
}  // namespace cpu_tools

std::ostream& operator<<(std::ostream& os, cpu_tools::arch_t arch);

#endif // CPU_TOOLS_CPP

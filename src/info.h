/* Copyright (C) 2021-2022 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INFO_H
#define INFO_H

namespace dsnote {
static constexpr const char* APP_NAME = "Speech Note";
#ifdef QT_DEBUG
static constexpr const char* APP_VERSION = "1.8.0 (debug)";
#else
static constexpr const char* APP_VERSION = "1.8.0";
#endif
#ifdef FLATPAK
static constexpr const char* APP_ID = "org.mkiol.Dsnote";
#else
static constexpr const char* APP_ID = "dsnote";
#endif
#ifdef SAILFISH
static constexpr const char* APP_BINARY_ID = "harbour-dsnote";
#else
static constexpr const char* APP_BINARY_ID = "dsnote";
#endif
static constexpr int CONF_VERSION = 8;
static constexpr const char* STT_VERSION = "1.3.0-0";
static constexpr const char* TENSORFLOW_VERSION = "v2.8.0-8";
static constexpr const char* ORG = "org.mkiol";
static constexpr const char* AUTHOR = "Michal Kosciesza";
static constexpr const char* AUTHOR_EMAIL = "michal@mkiol.net";
static constexpr const char* COPYRIGHT_YEAR = "2021-2022";
static constexpr const char* SUPPORT_EMAIL = "dsnote@mkiol.net";
static constexpr const char* PAGE = "https://github.com/mkiol/dsnote";
static constexpr const char* LICENSE = "Mozilla Public License 2.0";
static constexpr const char* LICENSE_URL = "http://mozilla.org/MPL/2.0/";
static constexpr const char* LICENSE_SPDX = "MPL-2.0";
}  // namespace dsnote

#endif  // INFO_H

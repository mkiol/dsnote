/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
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
static constexpr const char* APP_VERSION = "1.0.2 (debug)";
#else
    static constexpr const char* APP_VERSION = "1.0.2";
#endif
#ifdef SAILFISH
static constexpr const char* APP_ID = "harbour-dsnote";
#else
#ifdef FLATPAK
static constexpr const char* APP_ID = "org.mkiol.Dsnote";
#else
    static constexpr const char* APP_ID = "dsnote";
#endif
#endif // SAILFISH
    static const int CONF_VERSION = 1;
    static constexpr const char* DEEPSPEECH_VERSION = "0.10.0-alpha.3";
    static constexpr const char* TENSORFLOW_VERSION = "2.3.0-6";
    static constexpr const char* ORG = "org.mkiol";
    static constexpr const char* AUTHOR = "Michal Kosciesza";
    static constexpr const char* AUTHOR_EMAIL = "michal@mkiol.net";
    static constexpr const char* COPYRIGHT_YEAR = "2021";
    static constexpr const char* SUPPORT_EMAIL = "dsnote@mkiol.net";
    static constexpr const char* PAGE = "https://github.com/mkiol/Dsnote";
    static constexpr const char* LICENSE = "Mozilla Public License 2.0";
    static constexpr const char* LICENSE_URL = "http://mozilla.org/MPL/2.0/";
    static constexpr const char* LICENSE_SPDX = "MPL-2.0";
}

#endif // INFO_H

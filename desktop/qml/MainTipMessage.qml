/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

TipMessage {
    id: root

    property bool warning: false

    color: warning ? "red" : palette.text
    verticalMode: true
    Layout.topMargin: appWin.padding
    Layout.leftMargin: appWin.padding
    Layout.rightMargin: appWin.padding
    closable: true
}

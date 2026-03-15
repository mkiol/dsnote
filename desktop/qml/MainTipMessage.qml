/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

TipMessage {
    id: root

    property bool warning: false

    color: warning ? "red" : palette.text
    Layout.topMargin: appWin.padding
    Layout.leftMargin: appWin.padding
    Layout.rightMargin: appWin.padding
    closable: true

    Accessible.role: Accessible.Notification
    Accessible.name: warning ? qsTr("Warning message") : qsTr("Information message")
    Accessible.description: text
}

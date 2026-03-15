/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

InlineMessage {
    id: root

    property int indends: 0
    property alias text: _label.text
    property alias label: _label

    color: "red"
    Layout.fillWidth: true
    Layout.leftMargin: indends * appWin.padding
    Layout.rightMargin: 0

    onCloseClicked: visible = false

    FontMetrics {
        id: fontMetrics
    }

    Label {
        id: _label

        Layout.fillWidth: true
        color: root.color
        wrapMode: Text.Wrap
    }
}

/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15

ToolTip {
    id: root
    timeout: 3000
    delay: 0
    anchors.centerIn: parent

    function invertColor(color) {
        var r = 1 - color.r
        var g = 1 - color.g
        var b = 1 - color.b
        return Qt.rgba(r, g, b, color.a)
    }

    Component.onCompleted: {
        if (background && background.color)
            contentItem.color = invertColor(background.color)
    }

    contentItem: Label {
        text: root.text
        font: root.font
        color: palette.toolTipText
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        width: parent.width
        height: parent.height
        onClicked: root.hide()
    }
}

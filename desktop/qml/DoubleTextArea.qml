/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

GridLayout {
    id: root

    property int indends: 0
    property bool verticalMode: parent.verticalMode !== undefined ? parent.verticalMode :
                                parent.parent.verticalMode !== undefined ? parent.parent.verticalMode :
                                parent.parent.parent.verticalMode !== undefined ? parent.parent.parent.verticalMode :
                                false
    property string toolTip1: ""
    property string toolTip2: ""
    property alias textArea1: _textArea1
    property alias textArea2: _textArea2
    property int preferredHeight: preferredHeight
    readonly property color dimColor: {
        var c = palette.text
        return Qt.rgba(c.r, c.g, c.b, 0.8)
    }

    columns: verticalMode ? 1 : 2
    columnSpacing: appWin.padding
    rowSpacing: appWin.padding
    Layout.fillWidth: true

    ScrollView {
        clip: true

        Layout.fillWidth: true
        Layout.leftMargin: root.indends * appWin.padding
        Layout.preferredHeight: root.preferredHeight * (root.verticalMode ? 1 : 3)
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        TextArea {
            id: _textArea1

            selectByMouse: true
            wrapMode: TextEdit.Wrap
            verticalAlignment: TextEdit.AlignTop
            color: palette.text

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered && root.toolTip1.length !== 0
            ToolTip.text: root.toolTip1
            hoverEnabled: true

            TextContextMenu {}
        }
    }

    ScrollView {
        clip: true

        Layout.fillWidth: root.verticalMode
        Layout.preferredWidth: root.verticalMode ? 0 : (parent.width / 2)
        Layout.preferredHeight: root.preferredHeight * (root.verticalMode ? 1 : 3)
        Layout.leftMargin: root.verticalMode ? root.indends * appWin.padding : 0
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        TextArea {
            id: _textArea2

            selectByMouse: true
            wrapMode: TextEdit.Wrap
            verticalAlignment: TextEdit.AlignTop
            color: palette.text

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered && root.toolTip2.length !== 0
            ToolTip.text: root.toolTip2
            hoverEnabled: true

            TextContextMenu {}
        }
    }
}

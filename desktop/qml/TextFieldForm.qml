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
    property alias label: _label
    property string toolTip: ""
    property alias textField: _textField
    property alias button: _button
    property alias text: _textField.text
    property bool compact: true

    columns: verticalMode ? 1 : button.visible ? 3 : 2
    columnSpacing: appWin.padding
    rowSpacing: appWin.padding
    Layout.fillWidth: true

    Label {
        id: _label

        Layout.fillWidth: true
        Layout.leftMargin: root.indends * appWin.padding
    }

    TextField {
        id: _textField

        Layout.fillWidth: root.verticalMode
        Layout.preferredWidth: root.verticalMode ? 0 : (parent.width / 2) - (compact ? _button.width + root.columnSpacing : 0)
        Layout.leftMargin: root.verticalMode ? (root.indends + 1) * appWin.padding : 0
        color: palette.text

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered && root.toolTip.length !== 0
        ToolTip.text: root.toolTip
        hoverEnabled: true

        TextContextMenu {}
    }

    Button {
        id: _button

        visible: text.length !== 0
        Layout.leftMargin: root.verticalMode ? (root.indends + 1) * appWin.padding : 0
    }
}

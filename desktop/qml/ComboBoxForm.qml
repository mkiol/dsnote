/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: root

    property int indends: 0
    property bool verticalMode: parent !== undefined ? parent.verticalMode !== undefined ? parent.verticalMode :
                                parent.parent.verticalMode !== undefined ? parent.parent.verticalMode :
                                parent.parent.parent.verticalMode !== undefined ? parent.parent.parent.verticalMode :
                                false : false
    property alias label: _label
    property alias comboBox: _comboBox
    property string toolTip: ""
    property alias model: _comboBox.model
    property alias displayText: _comboBox.displayText
    property alias currentIndex: _comboBox.currentIndex
    property alias button: _button
    property string toolTipButton: button.text
    property bool compact: true

    columns: verticalMode ? 1 : button.visible ? 3 : 2
    columnSpacing: appWin.padding
    rowSpacing: appWin.padding
    Layout.fillWidth: true
    width: parent.width

    Label {
        id: _label

        Layout.fillWidth: true
        Layout.leftMargin: root.indends * appWin.padding
    }

    ComboBox {
        id: _comboBox

        Layout.fillWidth: root.verticalMode
        Layout.preferredWidth: root.verticalMode ? 0 : ((parent.width / 2) - (_button.visible && root.compact ? _button.width + root.columnSpacing : 0))
        Layout.leftMargin: root.verticalMode ? (root.indends + 1) * appWin.padding : 0

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered && root.toolTip.length !== 0
        ToolTip.text: root.toolTip
        hoverEnabled: true

        Accessible.name: root.label.text
    }

    Button {
        id: _button

        visible: text.length !== 0
        Layout.leftMargin: root.verticalMode ? (root.indends + 1) * appWin.padding : 0

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: button.display === AbstractButton.IconOnly ? hovered : false
        ToolTip.text: root.toolTipButton
        hoverEnabled: true
    }
}

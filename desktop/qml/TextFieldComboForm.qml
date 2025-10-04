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
    property bool verticalMode: parent.verticalMode !== undefined ? parent.verticalMode :
                                parent.parent.verticalMode !== undefined ? parent.parent.verticalMode :
                                parent.parent.parent.verticalMode !== undefined ? parent.parent.parent.verticalMode :
                                false
    property alias label: _label
    property string toolTip: ""
    property alias textField: _textField
    property alias comboBox: _comboBox
    property alias text: _textField.text
    property string toolTipCombo: comboBox.text
    property bool compact: true
    property bool valid: true

    columns: verticalMode ? 1 : comboBox.visible ? 3 : 2
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
        Layout.preferredWidth: root.verticalMode ? 0 :
             ((parent.width / 2) - (_comboBox.visible && root.compact ? _comboBox.width + root.columnSpacing : 0))
        Layout.leftMargin: root.verticalMode ? (root.indends + 1) * appWin.padding : 0
        color: root.valid ? palette.text : "red"

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered && root.toolTip.length !== 0
        ToolTip.text: root.toolTip
        hoverEnabled: true

        Accessible.name: root.label.text

        TextContextMenu {}
    }

    ComboBox {
        id: _comboBox

        Layout.leftMargin: root.verticalMode ? (root.indends + 1) * appWin.padding : 0

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered && root.toolTip.length !== 0
        ToolTip.text: root.toolTipCombo
        hoverEnabled: true

        Accessible.name: root.label.text
    }
}

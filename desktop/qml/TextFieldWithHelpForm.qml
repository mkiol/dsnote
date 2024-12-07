/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

RowForm {
    id: root

    property alias text: _textField.text
    property alias textField: _textField
    signal helpClicked

    TextField {
        id: _textField

        Layout.fillWidth: true
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered && root.toolTip.length !== 0
        ToolTip.text: root.toolTip
        hoverEnabled: true

        Accessible.name: root.label.text

        TextContextMenu {}
    }

    Button {
        id: _button

        icon.name: "help-about-symbolic"
        flat: true
        display: AbstractButton.IconOnly
        action: Action {
            text: qsTr("Help")
            onTriggered: root.helpClicked()
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: display === AbstractButton.IconOnly ? hovered : false
        ToolTip.text: text
        hoverEnabled: true
    }
}

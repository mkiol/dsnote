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
    property alias spinBox: _spinBox
    property string toolTip: ""
    property alias value: _spinBox.value

    columns: verticalMode ? 1 : 2
    columnSpacing: appWin.padding
    rowSpacing: appWin.padding
    Layout.fillWidth: true

    Label {
        id: _label

        Layout.fillWidth: true
        Layout.leftMargin: root.indends * appWin.padding
    }

    SpinBox {
        id: _spinBox

        Layout.fillWidth: verticalMode
        Layout.preferredWidth: verticalMode ? 0 : parent.width / 2
        Layout.leftMargin: verticalMode ? (root.indends + 1) * appWin.padding : 0

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered && root.toolTip.length !== 0
        ToolTip.text: root.toolTip
        hoverEnabled: true

        Component.onCompleted: {
            spinBox.contentItem.color = palette.text
        }
    }
}

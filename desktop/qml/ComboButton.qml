/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

RowLayout {
    id: root

    property alias button: _button
    property alias icon: _icon.icon
    property alias controlEnabled: row.enabled
    property alias combo: _combo
    property alias combo2: _combo2
    property alias combo3: _combo3
    property alias check: _check
    property alias frame: _frame
    property string comboToolTip: ""
    property string combo2ToolTip: ""
    property string combo3ToolTip: ""
    property string checkToolTip: ""
    property string buttonToolTip: ""
    readonly property bool off: combo.model.length === 0
    property string comboPlaceholderText: ""
    property string combo2PlaceholderText: ""
    property bool verticalMode: false
    property bool comboFillWidth: true
    property int comboPrefWidth: _combo.implicitWidth
    property alias comboRedBorder: _comboRedBorder.visible
    property alias combo2RedBorder: _combo2RedBorder.visible
    property alias combo3RedBorder: _combo3RedBorder.visible
    property alias showSeparator: _separator.visible

    Layout.fillWidth: verticalMode ? true : comboFillWidth

    Frame {
        id: _frame

        Layout.fillWidth: true
        background: Item {}

        topPadding: 2
        bottomPadding: 2
        leftPadding: appWin.padding
        rightPadding: appWin.padding

        RowLayout {
            id: row

            anchors.fill: parent

            ToolSeparator {
                id: _separator

                orientation: Qt.Vertical
                visible: false
                enabled: true
            }

            ToolButton {
                id: _icon

                hoverEnabled: false
                down: false
                Layout.alignment: Qt.AlignVCenter
                visible: icon.name.length !== 0
                enabled: !root.off
            }

            ComboBox {
                id: _combo

                Layout.fillWidth: verticalMode ? true : root.comboFillWidth
                Layout.preferredWidth: root.verticalMode ? implicitWidth : root.comboPrefWidth
                Layout.alignment: Qt.AlignVCenter
                enabled: !root.off
                displayText: root.off ? root.comboPlaceholderText : currentText
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: root.comboToolTip

                Rectangle {
                    id: _comboRedBorder

                    visible: false
                    border.width: 1
                    border.color: "red"
                    radius: 2
                    anchors.fill: parent
                    color: "transparent"
                }
            }

            ComboBox {
                id: _combo2

                visible: false
                Layout.preferredWidth: appWin.buttonWidth * 1.5
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                enabled: !root.off
                displayText: !model || model.length === 0 ? root.combo2PlaceholderText : currentText
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: root.combo2ToolTip

                Rectangle {
                    id: _combo2RedBorder

                    visible: false
                    border.width: 1
                    border.color: "red"
                    radius: 2
                    anchors.fill: parent
                    color: "transparent"
                }
            }

            ComboBox {
                id: _combo3

                visible: false
                Layout.preferredWidth: appWin.buttonWidth
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                enabled: !root.off
                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: root.combo3ToolTip

                Rectangle {
                    id: _combo3RedBorder

                    visible: false
                    border.width: 1
                    border.color: "red"
                    radius: 2
                    anchors.fill: parent
                    color: "transparent"
                }
            }

            Switch {
                id: _check

                visible: false
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                enabled: !root.off
                ToolTip.visible: hovered
                //ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: root.checkToolTip
            }

            Button {
                id: _button

                visible: text.length !== 0
                enabled: !root.off

                ToolTip.visible: root.buttonToolTip.length !== 0 && hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: root.buttonToolTip

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: appWin.buttonWidth
            }
        }
    }
}

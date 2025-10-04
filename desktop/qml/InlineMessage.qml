/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
     id: root

     property color color: palette.highlight
     property bool closable: false
     property alias actionButton: _actionButton
     property string actionButtonToolTip: ""
     default property alias content: column.data

     signal closeClicked

     padding: appWin.padding
     implicitHeight: column.height + 2 * appWin.padding

     ColumnLayout {
         id: column

         width: parent.width - 3*x - buttonsRow.width
     }

     RowLayout {
         id: buttonsRow

         anchors {
             right: parent.right
             top: parent.top
         }

         Button {
             id: _actionButton

             Layout.preferredHeight: Math.min(implicitHeight, column.height)

             visible: icon.name.length > 0 || text.length > 0
             ToolTip.text: root.actionButtonToolTip
             ToolTip.visible: hovered && root.actionButtonToolTip.length > 0
             ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
             hoverEnabled: true
         }

         Button {
             id: closeButton

             Layout.preferredHeight: Math.min(implicitHeight, column.height)
             Layout.preferredWidth: height

             visible: root.closable
             icon.name: "window-close-symbolic"
             text: qsTr("Close")
             display: AbstractButton.IconOnly
             onClicked: root.closeClicked()
             ToolTip.visible: hovered
             ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
             ToolTip.text: qsTr("Close")
             hoverEnabled: true
         }
     }
 }

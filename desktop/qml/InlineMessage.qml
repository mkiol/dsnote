/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

Control {
     id: root

     property color color: palette.highlight
     default property alias content: column.data

     padding: appWin.padding
     implicitHeight: column.height + 2 * appWin.padding

     background: Rectangle {
         id: bgBorderRect

         border.width: 1
         border.color: root.color
         color: "transparent"
         radius: 3
     }

     ColumnLayout {
         id: column
         y: appWin.padding

         anchors {
             left: parent.left
             leftMargin: appWin.padding
             right: parent.right
             rightMargin: appWin.padding
         }
     }
 }

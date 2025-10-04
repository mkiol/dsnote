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

    default property alias content: row.data

    property int indends: 0
    property bool verticalMode: parent.verticalMode !== undefined ? parent.verticalMode :
                                parent.parent.verticalMode !== undefined ? parent.parent.verticalMode :
                                parent.parent.parent.verticalMode !== undefined ? parent.parent.parent.verticalMode :
                                false
    property alias label: _label
    property string toolTip: ""
    property bool valid: true

    columns: verticalMode ? 1 : 2
    columnSpacing: appWin.padding
    rowSpacing: appWin.padding
    Layout.fillWidth: true

    Label {
        id: _label

        Layout.fillWidth: true
        Layout.leftMargin: root.indends * appWin.padding
    }

    RowLayout {
        id: row

        Layout.fillWidth: root.verticalMode
        Layout.preferredWidth: root.verticalMode ? 0 : (parent.width / 2)
        Layout.leftMargin: root.verticalMode ? (root.indends + 1) * appWin.padding : 0
    }
}

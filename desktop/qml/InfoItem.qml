/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    property alias label: _label.text
    property alias value: _value.text

    spacing: appWin.padding

    Label {
        Layout.alignment: Qt.AlignTop
        id: _label
        text: qsTr("Project website")
    }
    Label {
        id: _value
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignLeft
        textFormat: Text.StyledText
        onLinkActivated: Qt.openUrlExternally(link)
        wrapMode: Text.Wrap
    }
}

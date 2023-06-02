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

    property alias label: _label.text
    property alias value: _value.text

    spacing: appWin.padding

    Label {
        id: _label
        text: qsTr("Project website")
    }
    Label {
        id: _value
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignLeft
        textFormat: Text.StyledText
        onLinkActivated: Qt.openUrlExternally(link)
    }
}

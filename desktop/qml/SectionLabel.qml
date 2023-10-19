/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

Item {
    id: root
    property alias text: label.text

    implicitHeight: label.height + 2 * appWin.padding

    Layout.fillWidth: true

    RowLayout {
        anchors.fill: parent

        Label {
            id: label
            font.bold: true
            Layout.alignment: Qt.AlignVCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: palette.buttonText
            opacity: 0.2
            Layout.alignment: Qt.AlignVCenter
        }
    }
}

/* Copyright (C) 2017 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Row {
    id: root

    property alias title: label1.text
    property alias description: label2.text

    anchors.left: parent.left; anchors.right: parent.right
    anchors.leftMargin: Theme.paddingLarge; anchors.rightMargin: Theme.paddingLarge
    spacing: Theme.paddingLarge

    Column {
        spacing: Theme.paddingMedium
        anchors.top: parent.top
        width: parent.width-1*Theme.paddingLarge

        Label {
            id: label1
            width: parent.width
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }

        Label {
            id: label2
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.highlightColor
            linkColor: Theme.primaryColor
            textFormat: Text.StyledText
            onLinkActivated: {
                Qt.openUrlExternally(link);
            }
        }

        Spacer {}
    }
}

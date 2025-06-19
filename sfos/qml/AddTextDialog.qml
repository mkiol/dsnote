/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

DockedPanel {
    id: root

    signal addClicked
    signal replaceClicked

    open: true
    modal: true

    width: parent.width
    height: column.height + 2 * Theme.paddingLarge
    dock: Dock.Bottom

    Column {
        id: column

        spacing: Theme.paddingLarge
        y: Theme.paddingMedium
        width: parent.width - 2 * Theme.paddingMedium

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Add text to the current note or replace it?")
        }

        Row {
            spacing: Theme.paddingLarge
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: qsTr("Add")
                onClicked: {
                    root.addClicked()
                    root.hide()
                }
            }

            Button {
                text: qsTr("Replace")
                onClicked: {
                    root.replaceClicked()
                    root.hide()
                }
            }
        }
    }
}

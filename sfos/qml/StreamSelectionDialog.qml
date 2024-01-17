/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.DirModel 1.0

Dialog {
    id: root

    property var streams
    property int selectedId: 0
    property string filePath
    property bool replace: false

    allowedOrientations: Orientation.All

    onAccepted: {
        app.transcribe_file(filePath, replace, selectedId)
    }

    Column {
        width: parent.width

        DialogHeader {}

        Label {
            font.pixelSize: Theme.fontSizeLarge
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            color: Theme.secondaryHighlightColor
            wrapMode: Text.Wrap
            text: qsTr("The file contains multiple audio streams. Select which one you want to process.")
        }

        ComboBox {
            id: combo

            label: qsTr("Audio stream")
            menu: ContextMenu {
                Repeater {
                    model: root.streams
                    MenuItem { text: modelData }
                }
            }

            onCurrentIndexChanged: {
                var name = value
                root.selectedId = parseInt(name.substring(name.lastIndexOf("(") + 1, name.lastIndexOf(")")))
            }
        }
    }
}

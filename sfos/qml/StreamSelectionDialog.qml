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
    property int selectedIndex: 0

    property string filePath
    property bool replace: false

    allowedOrientations: Orientation.All

    function updateSelectedIndex(name) {
        selectedIndex = parseInt(name.substring(name.lastIndexOf("(") + 1, name.lastIndexOf(")")))
    }

    canAccept: app.stt_configured || combo.value.lastIndexOf("Audio") !== 0

    onAccepted: {
        updateSelectedIndex(combo.value)
        app.open_file(filePath, selectedIndex, replace)
    }

    SilicaFlickable {
        width: parent.width
        height: parent.height
        contentHeight: column.height
        clip: true

        Column {
            id: column

            width: parent.width

            DialogHeader {}

            Label {
                font.pixelSize: Theme.fontSizeLarge
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                color: Theme.secondaryHighlightColor
                wrapMode: Text.Wrap
                text: qsTr("The file contains multiple streams. Select which one you want to import.")
            }

            Spacer {}

            ComboBox {
                id: combo

                label: qsTr("Stream")
                menu: ContextMenu {
                    Repeater {
                        model: root.streams
                        MenuItem { text: modelData }
                    }
                }
            }

            Spacer {}

            PaddedLabel {
                id: errorLabel

                visible: !root.canAccept
                color: Theme.errorColor
                text: qsTr("Speech to Text model has not been set up yet.")
            }
        }
    }
}

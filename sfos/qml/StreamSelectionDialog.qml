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

    property var audioStreams
    property var subtitlesStreams
    property int selectedIndex: 0
    readonly property bool hasAudio: audioStreams !== undefined && audioStreams.length > 0
    readonly property bool hasSubtitles: subtitlesStreams !== undefined && subtitlesStreams.length > 0

    property string filePath
    property bool replace: false

    allowedOrientations: Orientation.All

    function updateSelectedIndex(name) {
        selectedIndex = parseInt(name.substring(name.lastIndexOf("(") + 1, name.lastIndexOf(")")))
    }

    onAccepted: {
        if ((hasAudio && !hasSubtitles) || (hasAudio && switchAudio.checked)) {
            root.updateSelectedIndex(comboAudio.value)
        } else if ((hasSubtitles && !hasAudio) || (hasSubtitles && switchSubtitles.checked)) {
            root.updateSelectedIndex(comboSubtitles.value)
        } else {
            return
        }

        app.open_file(filePath, selectedIndex, replace)
    }

    canAccept: (hasAudio && !hasSubtitles) || (!hasAudio && hasSubtitles) ||
               (hasAudio && switchAudio.checked) || (hasSubtitles && switchSubtitles.checked)

    Column {
        width: parent.width

        DialogHeader {}

        Label {
            font.pixelSize: Theme.fontSizeLarge
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            color: Theme.secondaryHighlightColor
            wrapMode: Text.Wrap
            text: qsTr("The file contains multiple streams. Select which one you want to process.")
        }

        Spacer {}

        ComboBox {
            id: comboAudio

            visible: root.hasAudio
            label: qsTr("Audio stream")
            menu: ContextMenu {
                Repeater {
                    model: root.audioStreams
                    MenuItem { text: modelData }
                }
            }
        }

        TextSwitch {
            id: switchAudio

            checked: true
            visible: root.hasAudio && root.hasSubtitles
            text: qsTr("Transcribe selected audio stream")

            onCheckedChanged: {
                switchSubtitles.checked = !checked
            }
        }

        Spacer {}

        ComboBox {
            id: comboSubtitles

            visible: root.hasSubtitles
            label: qsTr("Subtitles")
            menu: ContextMenu {
                Repeater {
                    model: root.subtitlesStreams
                    MenuItem { text: modelData }
                }
            }
        }

        TextSwitch {
            id: switchSubtitles

            checked: false
            visible: root.hasAudio && root.hasSubtitles
            text: qsTr("Import selected subtitles")

            onCheckedChanged: {
                switchAudio.checked = !checked
            }
        }
    }
}

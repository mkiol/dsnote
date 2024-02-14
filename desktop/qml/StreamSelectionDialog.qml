/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

Dialog {
    id: root

    property var audioStreams
    property var subtitlesStreams
    property int selectedIndex: 0

    readonly property bool hasAudio: audioStreams !== undefined && audioStreams.length > 0
    readonly property bool hasSubtitles: subtitlesStreams !== undefined && subtitlesStreams.length > 0

    modal: true
    width: Math.min(implicitWidth, parent.width - 2 * appWin.padding)
    height: column.implicitHeight + 2 * verticalPadding + footer.height
    closePolicy: Popup.CloseOnEscape
    clip: true

    function updateSelectedIndex(name) {
        selectedIndex = parseInt(name.substring(name.lastIndexOf("(") + 1, name.lastIndexOf(")")))
    }

    onOpened: {
        comboAudio.currentIndex = 0
        comboSubtitles.currentIndex = 0
    }

    header: Item {}

    footer: Item {
        height: closeButton.height + appWin.padding

        RowLayout {
            anchors {
                right: parent.right
                rightMargin: root.rightPadding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.reject()
                Keys.onEscapePressed: root.reject()
            }
        }
    }

    ColumnLayout {
        id: column

        width: parent.width
        spacing: appWin.padding * 2

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("The file contains multiple streams. Select which one you want to process.")
            font.pixelSize: appWin.textFontSizeBig
        }

        ColumnLayout {
            visible: root.hasAudio
            Layout.fillWidth: true
            spacing: appWin.padding

            SectionLabel {
                Layout.fillWidth: true
                text: qsTr("Audio streams")
            }

            ComboBox {
                id: comboAudio

                Layout.fillWidth: true
                model: root.audioStreams
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Transcribe selected audio stream")
                icon.name: "document-open-symbolic"
                Keys.onReturnPressed: {
                    root.updateSelectedIndex(comboAudio.displayText)
                    root.accept()
                }
                onClicked: {
                    root.updateSelectedIndex(comboAudio.displayText)
                    root.accept()
                }
            }
        }

        ColumnLayout {
            visible: root.hasSubtitles
            Layout.fillWidth: true
            spacing: appWin.padding

            SectionLabel {
                Layout.fillWidth: true
                text: qsTr("Subtitles")
            }

            ComboBox {
                id: comboSubtitles

                Layout.fillWidth: true
                model: root.subtitlesStreams
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Import selected subtitles")
                icon.name: "document-open-symbolic"
                Keys.onReturnPressed: {
                    root.updateSelectedIndex(comboSubtitles.displayText)
                    root.accept()
                }
                onClicked: {
                    root.updateSelectedIndex(comboSubtitles.displayText)
                    root.accept()
                }
            }
        }
    }
}

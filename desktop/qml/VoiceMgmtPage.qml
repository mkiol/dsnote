/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

DialogPage {
    id: root

    readonly property bool verticalMode: width < appWin.verticalWidthThreshold

    title: qsTr("Voice samples")

    footer: Item {
        height: closeButton.height + appWin.padding

        Button {
            id: createNewButton

            anchors {
                left: parent.left
                leftMargin: root.leftPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            text: qsTr("Create a new voice sample")
            icon.name: "special-effects-symbolic"
            onClicked: appWin.openDialog("VoiceImportPage.qml")
            Keys.onReturnPressed: appWin.openDialog("VoiceImportPage.qml")

            Dot {
                visible: app.available_tts_ref_voices.length === 0 && ((app.tts_ref_voice_needed && !_settings.translator_mode) ||
                         (_settings.translator_mode && (app.tts_for_in_mnt_ref_voice_needed || app.tts_for_out_mnt_ref_voice_needed)))
                size: createNewButton.height / 5
                anchors {
                    right: createNewButton.right
                    rightMargin: size / 2
                    top: createNewButton.top
                    topMargin: size / 2
                }
            }
        }

        Button {
            id: closeButton

            anchors {
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            text: qsTr("Close")
            //icon.name: "window-close-symbolic"
            onClicked: root.reject()
            Keys.onEscapePressed: root.reject()
        }
    }

    onOpenedChanged: {
        if (!opened) {
            app.player_reset()
            app.recorder_reset()
        }
    }

    Component {
        id: voiceDelegate

        Control {
            id: control

            property bool editActive: false

            function stop_playback() {
                if (app.player_playing)
                    app.player_stop_voice_ref()
            }

            background: Rectangle {
                id: bg

                anchors.fill: parent
                color: palette.text
                opacity: control.hovered ? 0.1 : 0.0
            }

            width: root.listViewItem.width
            height: deleteButton.height

            Label {
                id: nameField

                text: modelData
                visible: !control.editActive
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: appWin.padding
                anchors.right: renameButton.left
                anchors.rightMargin: appWin.padding
            }

            RowLayout {
                visible: control.editActive
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: appWin.padding
                anchors.right: playButton.left
                anchors.rightMargin: appWin.padding

                TextField {
                    id: editField

                    Layout.fillWidth: true
                    text: modelData
                    color: palette.text

                    TextContextMenu {}
                }

                Button {
                    width: appWin.buttonWithIconWidth
                    text: qsTr("Save")
                    icon.name: "document-save-symbolic"
                    onClicked: {
                        control.editActive = !control.editActive

                        if (editField.text.length !== 0)
                            app.rename_tts_ref_voice(index, editField.text)
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Save changes")
                    hoverEnabled: true
                }

                Button {
                    width: appWin.buttonWithIconWidth
                    icon.name: "action-unavailable-symbolic"
                    text: qsTr("Cancel")
                    onClicked: control.editActive = !control.editActive
                }
            }

            Button {
                id: playButton

                enabled: !control.editActive
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: renameButton.left
                anchors.rightMargin: appWin.padding
                icon.name: app.player_playing && app.player_current_voice_ref_idx === index ?
                               "media-playback-stop-symbolic" : "media-playback-start-symbolic"
                onClicked: {
                    control.editActive = false

                    if (app.player_playing && app.player_current_voice_ref_idx === index)
                        app.player_stop_voice_ref()
                    else
                        app.player_play_voice_ref_idx(index)
                }
            }

            Button {
                id: renameButton

                enabled: !control.editActive
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: deleteButton.left
                anchors.rightMargin: appWin.padding
                width: appWin.buttonWithIconWidth
                text: qsTr("Rename")
                onClicked: {
                    control.stop_playback()
                    control.editActive = !control.editActive
                }
            }

            Button {
                id: deleteButton

                enabled: !control.editActive
                icon.name: "edit-delete-symbolic"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: root._rightMargin
                width: appWin.buttonWithIconWidth
                text: qsTr("Delete")
                onClicked: {
                    control.stop_playback()
                    app.delete_tts_ref_voice(index)
                }
            }
        }
    }

    placeholderLabel {
        text: qsTr("You haven't created voice samples yet.") + " " +
              qsTr("Use %1 to make a new one.").arg("<i>" + createNewButton.text + "</i>")
        enabled: root.listViewItem.model.length === 0
    }

    listViewItem {
        model: app.available_tts_ref_voices
        delegate: voiceDelegate
    }
}

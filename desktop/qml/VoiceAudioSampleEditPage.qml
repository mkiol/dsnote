/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Settings 1.0

DialogPage {
    id: root

    readonly property bool verticalMode: width < height
    property var data: null
    property int index: 0

    readonly property string dataName: data ? data : ""
    readonly property bool canSave: testData()
    readonly property bool nameTaken: _nameForm.textField.text.trim().length > 0 && !testData()
    readonly property int effectiveWidth: root.implicitWidth - root._leftMargin - root._rightMargin - appWin.padding

    title: qsTr("Rename voice audio sample")

    function cancel() {
        appWin.openDialog("VoiceMgmtPage.qml")
    }

    function saveData() {
        var idx = index
        var name = dataName;
        var new_name = _nameForm.textField.text.trim()

        if (new_name.length === 0) {
            return
        }

        app.rename_tts_ref_voice(idx, new_name)

        appWin.openDialog("VoiceMgmtPage.qml")
    }

    function testData() {
        var idx = index
        var new_name = _nameForm.textField.text.trim()

        if (new_name.length === 0) {
            return false
        }

        return app.test_tts_ref_voice(idx, new_name)
    }

    function stop_playback() {
        if (app.player_playing)
            app.player_stop_voice_ref()
    }

    onOpenedChanged: {
        stop_playback()
    }

    footer: Item {
        height: closeButton.height + appWin.padding

        RowLayout {
            anchors {
                left: parent.left
                leftMargin: root.leftPadding + appWin.padding
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                property bool playing: app.player_playing && app.player_current_voice_ref_idx === root.index
                icon.name: playing ? "media-playback-stop-symbolic" : "media-playback-start-symbolic"
                display: AbstractButton.IconOnly
                text: playing ? qsTr("Stop") : qsTr("Play")
                onClicked: {
                    if (playing)
                        app.player_stop_voice_ref()
                    else
                        app.player_play_voice_ref_idx(index)
                }

                ToolTip.visible: hovered
                ToolTip.text: text
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Save")
                enabled: root.canSave
                icon.name: "document-save-symbolic"
                Keys.onReturnPressed: root.saveData()
                onClicked: root.saveData()
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.cancel()
                Keys.onEscapePressed: root.cancel()
            }
        }
    }

    ColumnLayout {
        property alias verticalMode: root.verticalMode
        Layout.fillWidth: true

        TextFieldForm {
            id: _nameForm

            label.text: qsTr("Name")
            valid: !root.nameTaken
            toolTip: valid ? "" : qsTr("This name is already taken")
            textField {
                text: root.dataName
            }
        }
    }
}

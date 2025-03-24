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
import org.mkiol.dsnote.Dsnote 1.0

DialogPage {
    id: root

    readonly property bool verticalMode: width < height
    property var data: null
    property int index: 0

    readonly property string dataName: data && data.length > 0 ? data[0] : ""
    readonly property string dataFile: data && data.length > 1 ? data[1] : ""
    readonly property string dataText: data && data.length > 2 ? data[2] : ""

    readonly property bool dataOk: testData()
    readonly property bool canSave: _textForm.text.trim().length > 0 && dataOk
    readonly property bool nameTaken: _nameForm.textField.text.trim().length > 0 && !dataOk
    readonly property int effectiveWidth: root.implicitWidth - root._leftMargin - root._rightMargin - appWin.padding

    title: qsTr("Edit voice audio sample")

    function cancel() {
        appWin.openDialog("VoiceMgmtPage.qml")
    }

    function saveData() {
        var idx = index
        var name = dataName;
        var text = dataText;
        var new_name = _nameForm.textField.text.trim()
        var new_text = _textForm.text.trim()

        if (new_name.length === 0 || new_text.length === 0) {
            return
        }

        app.update_tts_ref_voice(idx, new_name, new_text)

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

    Connections {
        target: app
        onText_decoded_internal: {
            var new_text = text.trim()
            if (new_text.length > 0) {
                _textForm.text = new_text
                appWin.toast.show(qsTr("Text decoding has completed!"))
            }
        }
    }

    onOpenedChanged: {
        stop_playback()
        app.cancel_if_internal()
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
                display: root.verticalMode ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                text: playing ? qsTr("Stop") : qsTr("Play")
                onClicked: {
                    if (playing)
                        app.player_stop_voice_ref()
                    else
                        app.player_play_voice_ref_idx(index)
                }

                ToolTip.visible: display === AbstractButton.IconOnly ? hovered : false
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

        TextArea {
            id: _textForm

            readOnly: app.state !== DsnoteApp.StateIdle
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            verticalAlignment: TextEdit.AlignTop
            placeholderText: qsTr("Text spoken in audio sample")
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: placeholderText
            hoverEnabled: true
            Layout.fillWidth: true
            text: root.dataText
            Layout.minimumHeight: _nameForm.textField.implicitHeight * 3

            TextContextMenu {}

            BusyIndicator {
                id: busyIndicator

                anchors.centerIn: parent
                running: app.state === DsnoteApp.StateTranscribingFile
                visible: running
            }
        }

        Button {
            enabled: app.stt_configured && app.state === DsnoteApp.StateIdle
            Layout.alignment: Qt.AlignRight
            text: qsTr("Decode text from audio sample")
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Use the current active Speech to Text model to decode text from an audio sample.")
            onClicked: {
                app.transcribe_ref_file(root.dataFile)
            }
        }

        TextFieldForm {
            id: _nameForm

            label.text: qsTr("Name")
            valid: !root.nameTaken
            toolTip: valid ? "" : qsTr("This name is already taken")
            textField {
                text: root.dataName
                placeholderText: _nameForm.label.text
            }
        }
    }
}

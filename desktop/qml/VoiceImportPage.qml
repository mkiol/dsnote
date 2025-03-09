/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
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

    readonly property bool verticalMode: (micButton.implicitWidth + fileButton.implicitWidth + cleanupButton.implicitWidth) > (width - 2 * appWin.padding)
    readonly property bool canCreate: app.player_ready &&
                                      rangeSlider.first.value < rangeSlider.second.value &&
                                      !app.recorder_processing &&
                                      testData()
    readonly property bool nameTaken: _nameForm.textField.text.trim().length > 0 && !testData()
    function format_time_to_s(time_ms) {
        return (time_ms / 1000).toFixed(1)
    }

    function cancel() {
        appWin.openDialog("VoiceMgmtPage.qml")
    }

    function export_voice() {
        if (!root.canCreate) return

        app.player_export_ref_voice(rangeSlider.first.value, rangeSlider.second.value, _nameForm.textField.text)
        appWin.openDialog("VoiceMgmtPage.qml")
    }

    function update_probs(probs) {
        probsRow.probs = probs
        probsRow.segSize = (rangeSlider.width -
                            rangeSlider.first.handle.width / 2) / probsRow.probs.length
    }

    function testData() {
        var name = _nameForm.textField.text.trim()

        if (name.length === 0) {
            return false
        }

        return app.test_tts_ref_voice(-1, name)
    }

    onOpenedChanged: {
        if (!opened) {
            app.player_reset()
            app.recorder_reset()
        }
    }

    title: qsTr("Create a new audio sample")

    Component {
        id: helpDialog

        HelpDialog {
            title: qsTr("Audio sample")

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Audio sample lets you clone someone's voice.") + " " +
                      qsTr("It can be created by recording a audio sample directly from the microphone or by providing an audio file.")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Tips:") +
                      "<ul>" +
                      "<li>" + qsTr("Audio sample should not be very long.") + " " + qsTr("The duration from 10 to 30 seconds is good enough.") + "</li>" +
                      "<li>" + qsTr("Use the range slider to clip the audio sample to the part containing speech.") + "</li>" +
                      "<li>" + qsTr("If you're not satisfied with voice cloning quality, try creating a few different audio samples and see which one gives you the best result.") + "</li>" +
                      "</ul>"
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Currently, voice cloning can be used in the following models:") +
                      "<ul>" +
                      "<li><i>Coqui XTTS</i></li>" +
                      "<li><i>Coqui YourTTS</i></li>" +
                      "<li><i>WhisperSpeech</i></li>" +
                      "</ul>"
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("To learn more about how to create a good audio sample, check out the specific model's website.")
            }
        }
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
                icon.name: "help-about-symbolic"
                display: AbstractButton.IconOnly
                text: qsTr("Help")
                onClicked: appWin.openPopup(helpDialog)
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: text
                hoverEnabled: true
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Create")
                enabled: root.canCreate
                icon.name: "document-save-symbolic"
                Keys.onReturnPressed: root.export_voice()
                onClicked: root.export_voice()
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                Keys.onReturnPressed: root.cancel()
                onClicked: root.cancel()
            }
        }
    }

    Connections {
        target: app
        onPlayer_duration_changed: {
            rangeSlider.from = 0
            rangeSlider.to = app.player_duration
            rangeSlider.first.value = 0
            rangeSlider.second.value = app.player_duration
        }
        onRecorder_new_stream_name: {
            _nameForm.textField.text = name
        }
        onRecorder_new_probs: root.update_probs(probs)
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 3
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding
        enabled: !app.recorder_processing

        Button {
            id: micButton

            Layout.fillWidth: root.verticalMode
            icon.name: "audio-input-microphone-symbolic"
            text: qsTr("Use a microphone")
            onClicked: voiceRecordDialog.open()

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Use a voice sample recorded from a microphone.")
            hoverEnabled: true
        }

        Button {
            id: fileButton

            Layout.fillWidth: root.verticalMode
            icon.name: "document-open-symbolic"
            text: qsTr("Import from a file")
            onClicked: fileReadDialog.open()

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Use a voice sample from an audio file.")
            hoverEnabled: true
        }

        CheckBox {
            id: cleanupButton

            checked: _settings.clean_ref_voice
            text: qsTr("Clean up audio")
            onCheckedChanged: {
                _settings.clean_ref_voice = checked
            }

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Automatically normalize volume and remove non-speech noise in a microphone recording or imported audio file.")
            hoverEnabled: true
        }
    }

    Frame {
        Layout.fillWidth: true
        enabled: app.player_ready && !app.recorder_processing

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Use the range slider to clip the audio sample to the part containing speech.")

        ColumnLayout {
            id: clipColumn

            anchors {
                left: parent.left
                right: parent.right
            }

            RangeSlider {
                id: rangeSlider

                onWidthChanged: root.update_probs(probsRow.probs)
                Layout.fillWidth: true
                from: 0
                to: app.player_duration
                snapMode: RangeSlider.SnapAlways
                stepSize: 100
                first {
                    value: 0
                    onValueChanged: {
                        if (rangeSlider.first.value > app.player_position)
                            app.player_stop()
                    }
                }
                second {
                    value: app.player_duration
                    onValueChanged: {
                        if (rangeSlider.second.value < app.player_position)
                            app.player_stop()
                    }
                }

                Row {
                    id: probsRow

                    property var probs: []
                    property int segSize: 0

                    visible: rangeSlider.enabled
                    spacing: 0
                    height: 6
                    x: 0.3 * rangeSlider.first.handle.width
                    y: rangeSlider.height / 2 - 3

                    Repeater {
                        model: probsRow.probs
                        Rectangle {
                            height: 6
                            width: probsRow.segSize
                            color: "green"
                            opacity: 0.8 * modelData
                        }
                    }
                }

                Rectangle {
                    visible: rangeSlider.enabled
                    height: 3
                    y: parent.height / 2
                    x: parent.first.handle.x + parent.first.handle.width / 2
                    width: (parent.width * app.player_position/app.player_duration) - x
                    color: palette.text
                }
            }

            RowLayout {
                spacing: appWin.padding

                Button {
                    enabled: !app.player_playing
                    icon.name: "media-playback-start-symbolic"
                    text: qsTr("Play")
                    onClicked: {
                        app.player_play(rangeSlider.first.value, rangeSlider.second.value)
                    }
                }

                Button {
                    enabled: app.player_playing
                    icon.name: "media-playback-stop-symbolic"
                    text: qsTr("Stop")
                    onClicked: {
                        app.player_stop()
                    }
                }

                Label {
                    enabled: app.player_ready
                    text: root.format_time_to_s(rangeSlider.first.value) +
                          " - " + root.format_time_to_s(rangeSlider.second.value) +
                          " (" + qsTr("Duration: %1 seconds").arg(root.format_time_to_s(rangeSlider.second.value - rangeSlider.first.value)) + ")"
                }
            }
        }
    }

    TextFieldForm {
        id: _nameForm

        enabled: app.player_ready && !app.recorder_processing
        label.text: qsTr("Name")
        valid: !root.nameTaken
        toolTip: valid ? "" : qsTr("This name is already taken")
        textField {
            text: root.dataName
        }
    }

    Item { height: 1}

    VoiceRecordPage {
        id: voiceRecordDialog

        anchors.centerIn: parent
        onRejected: app.recorder_reset()
    }

    BusyIndicator {
        Layout.alignment: Qt.AlignCenter
        running: app.recorder_processing
    }

    Dialogs.FileDialog {
        id: fileReadDialog

        title: qsTr("Open File")
        nameFilters: [
            qsTr("Audio and video files") + " (*.wav *.mp3 *.ogg *.oga *.ogx *.opus *.spx *.flac *.m4a *.aac *.mp4 *.mkv *.ogv *.webm)",
            qsTr("Audio files") + " (*.wav *.mp3 *.ogg *.oga *.ogx *.opus *.spx *.flac *.m4a *.aac)",
            qsTr("Video files") + " (*.mp4 *.mkv *.ogv *.webm)",
            qsTr("All files") + " (*)"]
        folder: _settings.file_audio_open_dir_url
        selectExisting: true
        selectMultiple: false
        onAccepted: {
            _nameForm.textField.text = ""
            app.player_import_from_url(fileUrl);
            _settings.file_audio_open_dir_url = fileUrl
        }
    }
}

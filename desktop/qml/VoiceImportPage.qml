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
    readonly property bool canCreate: app.player_ready && titleTextField.text.length !== 0 &&
                                      rangeSlider.first.value < rangeSlider.second.value

    function format_time_to_s(time_ms) {
        return (time_ms / 1000).toFixed(1)
    }

    function export_voice() {
        if (!root.canCreate) return

        app.player_export_ref_voice(rangeSlider.first.value, rangeSlider.second.value, titleTextField.text)
        appWin.openDialog("VoiceMgmtPage.qml")
    }

    onOpenedChanged: {
        if (!opened)
            app.player_reset()
    }

    title: qsTr("Create a new voice sample")

    footer: Item {
        height: closeButton.height + appWin.padding

        RowLayout {
            anchors {
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                icon.name: "go-previous-symbolic"
                DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
                Keys.onReturnPressed: appWin.openDialog("VoiceMgmtPage.qml")
                onClicked: appWin.openDialog("VoiceMgmtPage.qml")
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                //icon.name: "window-close-symbolic"
                onClicked: root.reject()
                Keys.onReturnPressed: root.reject()
            }

            Button {
                text: qsTr("Create")
                enabled: root.canCreate
                icon.name: "document-save-symbolic"
                Keys.onReturnPressed: root.export_voice()
                onClicked: root.export_voice()
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
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding
        enabled: !app.recorder_processing

        Button {
            Layout.fillWidth: root.verticalMode
            icon.name: "audio-input-microphone-symbolic"
            text: qsTr("Use a microphone")
            onClicked: voiceRecordDialog.open()

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Use a voice sample recorded from a microphone.")
        }

        Button {
            Layout.fillWidth: root.verticalMode
            icon.name: "document-open-symbolic"
            text: qsTr("Import from a file")
            onClicked: fileReadDialog.open()

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Use a voice sample from an audio file.")
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

                Rectangle {
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
                    icon.name: app.player_playing ?
                                   "media-playback-stop-symbolic" : "media-playback-start-symbolic"
                    text: app.player_playing ? qsTr("Stop") : qsTr("Play")
                    onClicked: {
                        if (app.player_playing)
                            app.player_stop()
                        else {
                            app.player_play(rangeSlider.first.value, rangeSlider.second.value)
                        }
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

    GridLayout {
        id: gridVoceName

        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding
        enabled: app.player_ready

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: verticalMode ? appWin.padding : 2 * appWin.padding
            text: qsTr("Voice name")
        }

        TextField {
            id: titleTextField

            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? gridVoceName.width : gridVoceName.width / 2
            Layout.leftMargin: verticalMode ? appWin.padding : 0
            color: palette.text
            text: app.tts_ref_voice_auto_name()
        }
    }

    VoiceRecordPage {
        id: voiceRecordDialog

        anchors.centerIn: parent
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
            titleTextField.text = app.player_import_from_url(fileUrl);
            _settings.file_audio_open_dir_url = fileUrl
        }
    }
}

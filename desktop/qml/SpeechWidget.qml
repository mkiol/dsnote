/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

RowLayout {
    id: root

    Frame {
        Layout.fillWidth: true
        leftPadding: appWin.padding
        rightPadding: appWin.padding
        topPadding: 0
        bottomPadding: appWin.padding

        background: Item {}

        RowLayout {
            id: row

            anchors.fill: parent

            ToolButton {
                id: _icon
                enabled: false
                visible: false
                Layout.alignment: Qt.AlignVCenter
                icon.name: "audio-speakers-symbolic"
            }

            ComboBox {
                id: _combo
                enabled: false
                visible: false
            }

            Item {
                width: _icon.width
                height: _icon.height
                Layout.alignment: Qt.AlignBottom

                SpeechIndicator {
                    id: indicator
                    anchors.centerIn: parent
                    width: 27
                    height: 24
                    visible: !app.busy && !service.busy &&
                             app.state !== DsnoteApp.StateTranscribingFile &&
                             app.state !== DsnoteApp.StateWritingSpeechToFile
                    status: {
                        switch (app.task_state) {
                        case DsnoteApp.TaskStateIdle: return 0;
                        case DsnoteApp.TaskStateSpeechDetected: return 1;
                        case DsnoteApp.TaskStateProcessing: return 2;
                        case DsnoteApp.TaskStateInitializing: return 3;
                        case DsnoteApp.TaskStateSpeechPlaying: return 4;
                        case DsnoteApp.TaskStateSpeechPaused: return 5;
                        }
                        return 0;
                    }
                    color: palette.text
                }

                BusyIndicator {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    running: app.busy || service.busy ||
                             app.state === DsnoteApp.StateTranscribingFile ||
                             app.state === DsnoteApp.StateWritingSpeechToFile
                    visible: running
                }
            }

            Frame {
                id: frame

                topPadding: 2
                bottomPadding: 2
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(_combo.height,
                                                 speechText.implicitHeight + topPadding + bottomPadding)
                background: Rectangle {
                    color: palette.button
                    border.color: palette.buttonText
                    opacity: 0.3
                    radius: 3
                }

                Label {
                    id: speechText

                    anchors.fill: parent
                    wrapMode: TextEdit.WordWrap
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: _settings.font_size < 5 ? appWin.textFontSize : _settings.font_size
                    color: palette.text

                    property string placeholderText: {
                        if (app.busy || service.busy)
                            return qsTr("Busy...")
                        if (app.task_state === DsnoteApp.TaskStateInitializing)
                            return qsTr("Getting ready, please wait...")
                        if (app.state === DsnoteApp.StateWritingSpeechToFile)
                            return qsTr("Writing speech to file...") +
                                    (app.speech_to_file_progress > 0.0 ? " " +
                                                                         Math.round(app.speech_to_file_progress * 100) + "%" : "")
                        if (app.task_state === DsnoteApp.TaskStateProcessing)
                            return qsTr("Processing, please wait...")
                        if (app.state === DsnoteApp.StateTranscribingFile)
                            return qsTr("Transcribing audio file...") +
                                    (app.transcribe_progress > 0.0 ? " " +
                                                                     Math.round(app.transcribe_progress * 100) + "%" : "")
                        if (app.state === DsnoteApp.StateListeningSingleSentence ||
                                app.state === DsnoteApp.StateListeningAuto ||
                                app.state === DsnoteApp.StateListeningManual) return qsTr("Say something...")

                        if (app.task_state === DsnoteApp.TaskStateSpeechPaused) return qsTr("Reading is paused.")
                        if (app.state === DsnoteApp.StatePlayingSpeech) return qsTr("Reading a note...")
                        if (app.state === DsnoteApp.StateTranslating) return qsTr("Translating...")

                        return ""
                    }

                    text: app.intermediate_text.length === 0 ? placeholderText : app.intermediate_text
                    opacity: app.intermediate_text.length === 0 ? 0.6 : 1.0
                }
            }

            SequentialAnimation {
                running: app.task_state === DsnoteApp.TaskStateSpeechPaused
                loops: Animation.Infinite
                alwaysRunToEnd: true

                OpacityAnimator {
                    target: pauseButton
                    from: 1.0
                    to: 0.0
                    duration: 500
                }
                OpacityAnimator {
                    target: pauseButton
                    from: 0.0
                    to: 1.0
                    duration: 500
                }
            }

            Button {
                id: pauseButton

                Layout.alignment: Qt.AlignBottom
                Layout.preferredHeight: _icon.implicitHeight
                icon.name: app.task_state === DsnoteApp.TaskStateSpeechPaused ?
                               "media-playback-start-symbolic" : "media-playback-pause-symbolic"
                enabled: app.state === DsnoteApp.StatePlayingSpeech &&
                         (app.task_state === DsnoteApp.TaskStateProcessing ||
                          app.task_state === DsnoteApp.TaskStateSpeechPlaying ||
                          app.task_state === DsnoteApp.TaskStateSpeechPaused)
                visible: !stopButton.visible
                onClicked: {
                    if (app.task_state === DsnoteApp.TaskStateSpeechPaused)
                        app.resume_speech()
                    else
                        app.pause_speech()
                }

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: app.task_state === DsnoteApp.TaskStateSpeechPaused ?
                                  qsTr("Resume reading") : qsTr("Pause reading")
            }

            Button {
                id: stopButton

                Layout.alignment: Qt.AlignBottom
                Layout.preferredHeight: _icon.implicitHeight
                icon.name: "media-playback-stop-symbolic"
                enabled: app.task_state !== DsnoteApp.TaskStateProcessing &&
                         app.task_state !== DsnoteApp.TaskStateInitializing &&
                         (app.state === DsnoteApp.StateListeningSingleSentence ||
                          app.state === DsnoteApp.StateListeningManual ||
                          app.state === DsnoteApp.StateListeningAuto)

                visible: app.state === DsnoteApp.StateListeningSingleSentence ||
                         app.state === DsnoteApp.StateListeningManual ||
                         app.state === DsnoteApp.StateListeningAuto
                text: qsTr("Stop")
                onClicked: app.stop_listen()

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Stops listening. The already captured voice is decoded into text.")
            }

            Button {
                id: cancelButton

                Layout.alignment: Qt.AlignBottom
                Layout.preferredHeight: _icon.implicitHeight
                icon.name: "action-unavailable-symbolic"
                enabled: app.task_state === DsnoteApp.TaskStateProcessing ||
                         app.task_state === DsnoteApp.TaskStateInitializing ||
                         app.state === DsnoteApp.StateTranscribingFile ||
                         app.state === DsnoteApp.StateListeningSingleSentence ||
                         app.state === DsnoteApp.StateListeningManual ||
                         app.state === DsnoteApp.StateListeningAuto ||
                         app.state === DsnoteApp.StatePlayingSpeech ||
                         app.state === DsnoteApp.StateWritingSpeechToFile ||
                         app.state === DsnoteApp.StateTranslating
                text: qsTr("Cancel")
                onClicked: app.cancel()
            }
        }
    }
}

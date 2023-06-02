/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ApplicationWindow {
    id: appWin

    property int padding: 8

    width: 640
    height: 480

    visible: true

    header: MainToolBar {}

    ColumnLayout {
        anchors.fill: parent
        spacing: appWin.padding

        ScrollView {
            enabled: app.stt_configured || app.tts_configured
            opacity: enabled ? 1.0 : 0.0
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            TextArea {
                id: textArea

                wrapMode: TextEdit.WordWrap
                verticalAlignment: TextEdit.AlignBottom
                text: _settings.note
                onTextChanged: _settings.note = text

                Keys.onUpPressed: scrollBar.decrease()
                Keys.onDownPressed: scrollBar.increase()

                ScrollBar.vertical: ScrollBar { id: scrollBar }
            }
        }

        RowLayout {
            spacing: appWin.padding
            Layout.fillWidth: true
            Layout.rightMargin: appWin.padding
            Layout.leftMargin: appWin.padding
            Layout.bottomMargin: appWin.padding

            Frame {
                Layout.fillWidth: true
                Layout.minimumHeight: listenButton.height - 2
                Layout.preferredHeight: Math.max(listenButton.height - 2, speechText.implicitHeight + 2 * topPadding)
                background: Rectangle {
                    color: palette.button
                    border.color: palette.buttonText
                    opacity: 0.3
                    radius: 3
                }

                RowLayout {
                    property real _initialHeight: 0
                    Component.onCompleted: _initialHeight = implicitHeight

                    anchors.fill: parent

                    BusyIndicator {
                        Layout.preferredHeight: parent._initialHeight
                        Layout.preferredWidth: parent._initialHeight
                        Layout.alignment: Qt.AlignBottom
                        running: app.state === DsnoteApp.StateTranscribingFile
                        visible: running
                    }

                    SpeechIndicator {
                        id: indicator
                        Layout.preferredHeight: parent._initialHeight
                        Layout.preferredWidth: parent._initialHeight * 1.2
                        visible: app.state !== DsnoteApp.StateTranscribingFile
                        status: {
                            switch (app.speech) {
                            case DsnoteApp.SpeechStateNoSpeech: return 0;
                            case DsnoteApp.SpeechStateSpeechDetected: return 1;
                            case DsnoteApp.SpeechStateSpeechDecodingEncoding: return 2;
                            case DsnoteApp.SpeechStateSpeechInitializing: return 3;
                            case DsnoteApp.SpeechStateSpeechPlaying: return 4;
                            }
                            return 0;
                        }
                        Layout.alignment: Qt.AlignBottom
                        color: palette.text
                    }

                    Label {
                        id: speechText
                        Layout.fillWidth: true
                        Layout.leftMargin: appWin.padding / 2
                        wrapMode: TextEdit.WordWrap

                        property string placeholderText: {
                            if (app.speech === DsnoteApp.SpeechStateSpeechInitializing)
                                return qsTr("Getting ready, please wait...")
                            if (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding)
                                return qsTr("Processing, please wait...")
                            if (app.state === DsnoteApp.StateTranscribingFile)
                                return qsTr("Transcribing audio file...") +
                                        (app.transcribe_progress > 0.0 ? " " +
                                                                         Math.round(app.transcribe_progress * 100) + "%" : "")
                            if (app.state === DsnoteApp.StateListeningSingleSentence ||
                                    app.state === DsnoteApp.StateListeningAuto ||
                                    app.state === DsnoteApp.StateListeningManual) return qsTr("Say something...")

                            if (app.state === DsnoteApp.StatePlayingSpeech) return qsTr("Reading a note...")

                            return ""
                        }

                        font.italic: true
                        text: app.intermediate_text.length === 0 ? placeholderText : app.intermediate_text
                        opacity: app.intermediate_text.length === 0 ? 0.6 : 1.0
                    }
                }
            }

            Button {
                id: listenButton
                visible: _settings.speech_mode !== Settings.SpeechAutomatic
                icon.name: "audio-input-microphone-symbolic"
                Layout.alignment: Qt.AlignBottom
                enabled: app.stt_configured &&
                         (app.state === DsnoteApp.StateIdle || app.state === DsnoteApp.StateListeningManual)
                text: qsTr("Listen")
                onPressed: {
                    if (_settings.speech_mode === Settings.SpeechManual &&
                            app.state === DsnoteApp.StateIdle &&
                            app.speech === DsnoteApp.SpeechStateNoSpeech) {
                        app.listen()
                    }
                }
                onClicked: {
                    if (_settings.speech_mode !== Settings.SpeechManual &&
                            app.state === DsnoteApp.StateIdle &&
                            app.speech === DsnoteApp.SpeechStateNoSpeech)
                        app.listen()
                }
                onReleased: {
                    if (app.state === DsnoteApp.StateListeningManual)
                        app.stop_listen()
                }
            }

            Button {
                visible: _settings.speech_mode !== Settings.SpeechAutomatic
                icon.name: "audio-speakers-symbolic"
                Layout.alignment: Qt.AlignBottom
                enabled: app.tts_configured && textArea.text.length > 0 && app.state === DsnoteApp.StateIdle
                text: qsTr("Read")
                onClicked: app.play_speech()
            }

            Button {
                Layout.alignment: Qt.AlignBottom
                icon.name: "action-unavailable-symbolic"
                visible: _settings.speech_mode !== Settings.SpeechAutomatic
                enabled: app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                         app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                         app.state === DsnoteApp.StateTranscribingFile ||
                         app.state === DsnoteApp.StateListeningSingleSentence ||
                         app.state === DsnoteApp.StatePlayingSpeech
                text: qsTr("Cancel")
                onClicked: app.cancel()
            }

            Button {
                Layout.alignment: Qt.AlignBottom
                icon.name: "action-unavailable-symbolic"
                visible: _settings.speech_mode === Settings.SpeechAutomatic
                enabled: app.state === DsnoteApp.StateListeningAuto
                text: qsTr("Cancel 'Always on' listening")
                onClicked: {
                    _settings.speech_mode = Settings.SpeechSingleSentence
                    toast.show("Listening has been switched to 'One sentence' mode.")
                }
            }
        }
    }

    PlaceholderLabel {
        visible: !app.stt_configured && !app.tts_configured
        text: qsTr("No language has been set.") + " " +
              qsTr("Go to 'Languages' to download more language models.")
    }

    ToastNotification {
        id: toast
    }

    SpeechConfig {
        id: service

        onModel_download_finished: toast.show(qsTr("The model download is complete!"))
        onModel_download_error: toast.show(qsTr("Error: The model download was not successful."))
    }

    DsnoteApp {
        id: app

        onNote_copied: toast.show(qsTr("Copied!"))

        onError: {
            switch (type) {
            case DsnoteApp.ErrorFileSource:
                toast.show(qsTr("Error: Audio file processing was not successful."))
                break;
            case DsnoteApp.ErrorMicSource:
                toast.show(qsTr("Error: Microphone access was not successful."))
                break;
            default:
                toast.show(qsTr("Error: An unknown problem has occurred."))
            }
        }
    }
}

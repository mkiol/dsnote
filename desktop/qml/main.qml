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

    property var _dialogPage

    function openDialog(file) {
        if (_dialogPage) _dialogPage.close()
        _dialogPage = undefined

        var cmp = Qt.createComponent(file)
        if (cmp.status === Component.Ready) {
            var dialog = cmp.createObject(appWin);
            dialog.open()
            _dialogPage = dialog
        }
    }

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

            BusyIndicator {
                Layout.preferredHeight: listenButton.height
                Layout.preferredWidth: listenButton.height
                Layout.alignment: frame.height > listenButton.height ? Qt.AlignBottom : Qt.AlignVCenter
                running: app.state === DsnoteApp.StateTranscribingFile ||
                         app.state === DsnoteApp.StateWritingSpeechToFile
                visible: running
            }

            SpeechIndicator {
                id: indicator
                Layout.preferredHeight: listenButton.height * 0.7
                Layout.preferredWidth: listenButton.height
                visible: app.state !== DsnoteApp.StateTranscribingFile &&
                         app.state !== DsnoteApp.StateWritingSpeechToFile
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
                Layout.alignment: frame.height > listenButton.height ? Qt.AlignBottom : Qt.AlignVCenter
                color: palette.text
            }

            Frame {
                id: frame
                topPadding: 2
                bottomPadding: 2
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(listenButton.height,
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

                    property string placeholderText: {
                        if (app.speech === DsnoteApp.SpeechStateSpeechInitializing)
                            return qsTr("Getting ready, please wait...")
                        if (app.state === DsnoteApp.StateWritingSpeechToFile)
                            return qsTr("Writing speech to file...") +
                                    (app.speech_to_file_progress > 0.0 ? " " +
                                                                     Math.round(app.speech_to_file_progress * 100) + "%" : "")
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

            Button {
                id: listenButton
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
                icon.name: "audio-speakers-symbolic"
                Layout.alignment: Qt.AlignBottom
                enabled: app.tts_configured && textArea.text.length > 0 && app.state === DsnoteApp.StateIdle
                text: qsTr("Read")
                onClicked: app.play_speech()
            }

            Button {
                id: cancelButton
                Layout.alignment: Qt.AlignBottom
                icon.name: "action-unavailable-symbolic"
                enabled: app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                         app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                         app.state === DsnoteApp.StateTranscribingFile ||
                         app.state === DsnoteApp.StateListeningSingleSentence ||
                         app.state === DsnoteApp.StateListeningAuto ||
                         app.state === DsnoteApp.StatePlayingSpeech ||
                         app.state === DsnoteApp.StateWritingSpeechToFile
                text: qsTr("Cancel")
                onClicked: app.cancel()
            }
        }
    }

    PlaceholderLabel {
        visible: !app.stt_configured && !app.tts_configured
        text: qsTr("No language has been set.") + " " +
              qsTr("Go to 'Languages' to download language models.")
    }

    ToastNotification {
        id: toast
    }

    SpeechConfig {
        id: service

        onModel_download_finished: toast.show(qsTr("The model download is complete!"))
        onModel_download_error: toast.show(qsTr("Error: Couldn't download the model file."))
    }

    function showWelcome() {
        if (!app.busy && !app.stt_configured && !app.tts_configured)
            appWin.openDialog("HelloPage.qml")
    }

    DsnoteApp {
        id: app

        onBusyChanged: showWelcome()
        onStt_configuredChanged: showWelcome()
        onTts_configuredChanged: showWelcome()

        onNote_copied: toast.show(qsTr("Copied!"))
        onTranscribe_done: toast.show(qsTr("File transcription is complete!"))
        onSpeech_to_file_done: toast.show(qsTr("Speech saved to audio file!"))
        onError: {
            switch (type) {
            case DsnoteApp.ErrorFileSource:
                toast.show(qsTr("Error: Audio file processing has failed."))
                break;
            case DsnoteApp.ErrorMicSource:
                toast.show(qsTr("Error: Couldn't access Microphone."))
                break;
            case DsnoteApp.ErrorSttEngine:
                toast.show(qsTr("Error: Speech to Text engine initialization has failed."))
                break;
            case DsnoteApp.ErrorTtsEngine:
                toast.show(qsTr("Error: Text to Speech engine initialization has failed."))
                break;
            default:
                toast.show(qsTr("Error: An unknown problem has occurred."))
            }
        }
    }
}

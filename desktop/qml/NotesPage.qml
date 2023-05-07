/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2

import org.mkiol.dsnote.Settings 1.0
import org.mkiol.dsnote.Dsnote 1.0

Page {
    id: root

    title: qsTr("Note")

    header: ToolBar {
        GridLayout {
            anchors.fill: parent
            columns: 2

            ItemDelegate {
                Layout.fillWidth: true
                text: qsTr("Active Speech to Text model")
                enabled: false
            }

            ItemDelegate {
                Layout.fillWidth: true
                text: qsTr("Active Text to Speech model")
                enabled: false
            }

            ComboBox {
                id: comboBox
                Layout.fillWidth: true
                enabled: app.stt_configured
                currentIndex: app.active_stt_model_idx
                model: app.available_stt_models
                onActivated: app.set_active_stt_model_idx(index)
            }

            ComboBox {
                Layout.fillWidth: true
                enabled: app.tts_configured
                currentIndex: app.active_tts_model_idx
                model: app.available_tts_models
                onActivated: app.set_active_tts_model_idx(index)
            }

            ComboBox {
                Layout.rowSpan: 2
                Layout.fillWidth: true
                currentIndex: _settings.mode === Settings.Stt ? 0 : 1
                model: [qsTr("Speech to Text"), qsTr("Text to Speech")]
                onActivated: {
                    _settings.mode = _settings.mode === Settings.Stt ? Settings.Tts : Settings.Stt
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Flickable {
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            TextArea {
                id: textArea
                anchors.fill: parent
                wrapMode: TextEdit.WordWrap
                verticalAlignment: TextEdit.AlignBottom
                text: _settings.note
                onEditingFinished: _settings.note = text
            }
        }
        RowLayout {
            Layout.fillWidth: true

            SpeechIndicator {
                id: indicator
                width: 20
                Layout.leftMargin: 10
                height: speechText.height / 2
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
                Layout.alignment: Qt.AlignTop
                color: palette.text
            }

            TextArea {
                id: speechText
                Layout.fillWidth: true
                readOnly: true
                wrapMode: TextEdit.WordWrap
                placeholderText: {
                    if (!app.connected) return qsTr("Starting...")
                    if ((_settings.mode === Settings.Stt && !app.stt_configured) ||
                        (_settings.mode === Settings.Tts && !app.tts_configured)) {
                        return qsTr("Language model is not set")
                    }
                    if (app.speech === DsnoteApp.SpeechStateSpeechInitializing)
                        return qsTr("Getting ready, please wait...")
                    if (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding)
                        return qsTr("Decoding, please wait...")
                    if (app.state === DsnoteApp.StateTranscribingFile)
                        return qsTr("Transcribing audio file...") +
                                (app.transcribe_progress > 0.0 ? " " +
                                        Math.round(app.transcribe_progress * 100) + "%" : "")
                    if (_settings.mode === Settings.Stt) {
                        if (app.state === DsnoteApp.StateListeningSingleSentence ||
                            app.state === DsnoteApp.StateListeningAuto) return qsTr("Say something...")
                        if (_settings.speech_mode === Settings.SpeechSingleSentence) return qsTr("Click and say something...")
                        return qsTr("Press and say something...")
                    } else {
                        if (app.state === DsnoteApp.StatePlayingSpeech)
                            return qsTr("Reading a note...")
                        if (textArea.text.length > 0)
                            return qsTr("Press to read a note...")
                        else return qsTr("Make a note and press to read...")
                    }
                }

                font.italic: true
                text: app.intermediate_text
                leftPadding: 0
                Component.onCompleted: {
                    indicator.Layout.topMargin = (speechText.implicitHeight - speechText.font.pixelSize) / 2
                }
            }
        }
    }

    footer: ToolBar {
        contentHeight: toolButton.implicitHeight
        RowLayout {
            anchors.fill: parent

            ToolButton {
                Layout.alignment: Qt.AlignLeft
                readonly property bool press_and_hold: _settings.speech_mode === Settings.SpeechManual &&
                                                       (app.state === DsnoteApp.StateListeningManual || app.state === DsnoteApp.StateIdle)
                readonly property bool click_to_start: _settings.speech_mode === Settings.SpeechSingleSentence &&
                                                       (app.state === DsnoteApp.StateListeningSingleSentence ||
                                                        app.state === DsnoteApp.StateIdle)
                visible: _settings.mode === Settings.Stt && app.stt_configured
                enabled: press_and_hold || click_to_start
                text: {
                    if (_settings.speech_mode === Settings.SpeechManual)
                        return qsTr("Press and hold to start listening")
                    return qsTr("Click to start listening")
                }
                onClicked: app.listen()
            }

            ToolButton {
                Layout.alignment: Qt.AlignLeft
                visible: _settings.mode === Settings.Tts && app.tts_configured
                enabled: textArea.text.length > 0 && app.state === DsnoteApp.StateIdle
                text: qsTr("Click to read a note")
                onClicked: app.play_speech()
            }

            ToolButton {
                Layout.alignment: Qt.AlignLeft
                visible: app.stt_configured || app.tts_configured
                enabled: app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                         app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                         app.state === DsnoteApp.StateTranscribingFile ||
                         app.state === DsnoteApp.StateListeningSingleSentence ||
                         app.state === DsnoteApp.StatePlayingSpeech
                text: qsTr("Cancel")
                onClicked: app.cancel()
            }

            ToolButton {
                Layout.alignment: Qt.AlignLeft
                visible: textArea.text.length > 0
                text: qsTr("Clear")
                onClicked: _settings.note = ""
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                Layout.alignment: Qt.AlignRight
                visible: app.stt_configured
                enabled: (app.state === DsnoteApp.StateListeningManual ||
                          app.state === DsnoteApp.StateListeningAuto ||
                          app.state === DsnoteApp.StateListeningSingleSentence ||
                          app.state === DsnoteApp.StateTranscribingFile ||
                          app.state === DsnoteApp.StateIdle ||
                          app.state === DsnoteApp.StatePlayingSpeech)
                text: app.state === DsnoteApp.StateTranscribingFile ?
                          qsTr("Transcribing audio file...") + (app.transcribe_progress > 0.0 ? " (" + Math.round(app.transcribe_progress * 100) + "%)" : "") :
                          qsTr("Transcribe audio file")
                onClicked: {
                    if (app.state !== DsnoteApp.StateTranscribingFile)
                        fileDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        selectMultiple: false
        onAccepted: app.transcribe_file(fileDialog.fileUrl)
    }
}

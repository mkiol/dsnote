/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

import harbour.dsnote.Settings 1.0
import harbour.dsnote.Dsnote 1.0

Page {
    id: root

    allowedOrientations: Orientation.All

    SilicaFlickable {
        id: flick
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height - panel.height

        contentHeight: Math.max(column.height + textArea.height, height)
        onContentHeightChanged: scrollToBottom()
        clip: true

        Column {
            id: column

            width: root.width
            spacing: Theme.paddingLarge

            PullDownMenu {
                busy: app.busy || service.busy || app.state === DsnoteApp.StateTranscribingFile

                MenuItem {
                    text: qsTr("About %1").arg(APP_NAME)
                    onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
                }

                MenuItem {
                    text: qsTr("Settings")
                    onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
                }

                MenuItem {
                    enabled: (app.stt_configured || app.tts_configured) && !app.busy && !service.busy
                    text: app.state === DsnoteApp.StateTranscribingFile ||
                          app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                          app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                          app.speech === DsnoteApp.StatePlayingSpeech ?
                              qsTr("Cancel") : qsTr("Transcribe audio file")
                    onClicked: {
                        if (app.state === DsnoteApp.StateTranscribingFile ||
                            app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                            app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                            app.speech === DsnoteApp.StatePlayingSpeech)
                            app.cancel()
                        else
                            pageStack.push(fileDialog)
                    }
                }

                MenuItem {
                    enabled: textArea.text.length > 0
                    text: qsTr("Clear")
                    onClicked: _settings.note = ""
                }

                MenuItem {
                    visible: textArea.text.length > 0
                    text: qsTr("Copy")
                    onClicked: Clipboard.text = textArea.text
                }

                MenuItem {
                    text: qsTr("Mode: %1").arg(_settings.mode === Settings.Stt ? qsTr("Making a note") : qsTr("Reading a note"))
                    onClicked: _settings.mode = _settings.mode === Settings.Stt ? Settings.Tts : Settings.Stt
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: textArea.forceActiveFocus()
        }

        TextArea {
            id: textArea
            width: root.width
            anchors.bottom: parent.bottom
            text: _settings.note
            verticalAlignment: TextEdit.AlignBottom
            background: null
            labelComponent: null
            onTextChanged: _settings.note = text

            Connections {
                target: app
                onText_changed: {
                    flick.scrollToBottom()
                }
            }
        }

        ViewPlaceholder {
            enabled: textArea.text.length === 0 && !app.stt_configured &&
                     !app.tts_configured && !app.busy && !service.busy
            text: qsTr("Language model is not set")
            hintText: qsTr("Pull down and select Settings to download language models")
        }
    }

    VerticalScrollDecorator {
        flickable: flick
    }

    SpeechPanel {
        id: panel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        clickable: (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                    app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                   app.state !== DsnoteApp.StateListeningAuto) &&
                   !app.busy && !service.busy && app.connected &&
                   (_settings.mode !== Settings.Tts || textArea.text.length > 0)
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
        off: {
            if (!app.connected) return true
            if (_settings.mode === Settings.Stt && !app.stt_configured) return true
            if (_settings.mode === Settings.Tts && !app.tts_configured) return true
            return false
        }
        icon: _settings.mode === Settings.Stt ? "image://theme/icon-m-mic" :
                                                "image://theme/icon-m-speaker"
        busy: app.speech !== DsnoteApp.SpeechStateSpeechDecodingEncoding &&
              app.speech !== DsnoteApp.SpeechStateSpeechInitializing &&
              (app.busy || service.busy || !app.connected ||
              app.state === DsnoteApp.StateTranscribingFile)
        text: app.intermediate_text
        textPlaceholder: {
            if (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding) return qsTr("Decoding, please wait...")
            if (app.speech === DsnoteApp.SpeechStateSpeechInitializing) return qsTr("Getting ready, please wait...")
            if (app.state === DsnoteApp.StateTranscribingFile) return qsTr("Transcribing audio file...")
            if (_settings.mode === Settings.Stt) {
                if (app.state === DsnoteApp.StateListeningSingleSentence) return qsTr("Say something...")
                if (_settings.speech_mode === Settings.SpeechSingleSentence) return qsTr("Click and say something...")
                return qsTr("Press and say something...")
            } else {
                if (app.state === DsnoteApp.StatePlayingSpeech) return qsTr("Reading a note...")
                if (textArea.text.length > 0)
                    return qsTr("Click to read a note...")
                else return qsTr("Make a note and click to read it...")
            }
        }
        textPlaceholderActive: {
            if (!app.connected) return qsTr("Starting...")
            if ((_settings.mode === Settings.Stt && !app.stt_configured) ||
                (_settings.mode === Settings.Tts && !app.tts_configured)) {
                return qsTr("Language model is not set")
            }
            if (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding) return qsTr("Decoding, please wait...")
            if (app.speech === DsnoteApp.SpeechStateSpeechInitializing) return qsTr("Getting ready, please wait...")
            if (app.state === DsnoteApp.StatePlayingSpeech && app.speech === DsnoteApp.SpeechStateSpeechPlaying)
                return qsTr("Reading a note...")
            if (app.state === DsnoteApp.StatePlayingSpeech && app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding)
                return qsTr("Synthesizing speech, please wait...")
            if (!busy) {
                if (_settings.mode === Settings.Stt)
                    return qsTr("Say something...")
                else if (textArea.text.length > 0)
                    return qsTr("Click to read a note...")
                else
                    return qsTr("Make a note and click to read it...")
            }
            if (app.state === DsnoteApp.StateTranscribingFile) return qsTr("Transcribing audio file...")
            return qsTr("Busy...")
        }

        progress: app.transcribe_progress
        onPressed: {
            if (_settings.mode === Settings.Stt &&
                    _settings.speech_mode === Settings.SpeechManual &&
                    app.state === DsnoteApp.StateIdle &&
                    app.speech === DsnoteApp.SpeechStateNoSpeech) {
                app.listen()
            }
        }
        onReleased: {
            if (app.state === DsnoteApp.StateListeningManual) app.stop_listen()
        }
        onClicked: {
            if (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                    app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                    app.state === DsnoteApp.StateTranscribingFile ||
                    app.state === DsnoteApp.StateListeningSingleSentence ||
                    app.state === DsnoteApp.StatePlayingSpeech) {
                app.cancel()
                return
            }

            if (_settings.mode === Settings.Stt) {
                if (app.speech === DsnoteApp.SpeechStateNoSpeech) app.listen()
            } else {
                app.play_speech()
            }
        }
    }

    Component {
        id: fileDialog
        FilePickerPage {
            nameFilters: [ '*.wav', '*.mp3', '*.ogg', '*.flac', '*.m4a', '*.aac', '*.opus' ]
            onSelectedContentPropertiesChanged: {
                app.transcribe_file(selectedContentProperties.filePath)
            }
        }
    }

    Toast {
        id: notification
    }

    Connections {
        target: app

        onError: {
            switch (type) {
            case DsnoteApp.ErrorFileSource:
                notification.show(qsTr("Audio file couldn't be transcribed."))
                break;
            case DsnoteApp.ErrorMicSource:
                notification.show(qsTr("Microphone was unexpectedly disconnected."))
                break;
            case DsnoteApp.ErrorNoService:
                notification.show(qsTr("Unable to start service."))
                break;
            default:
                notification.show(qsTr("Oops! Something went wrong."))
            }
        }
    }
}

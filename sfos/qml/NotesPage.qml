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
                busy: app.busy || service.busy || app.state === DsnoteApp.SttTranscribingFile

                MenuItem {
                    text: qsTr("About %1").arg(APP_NAME)
                    onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
                }

                MenuItem {
                    text: qsTr("Settings")
                    onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
                }

                MenuItem {
                    enabled: app.configured && !app.busy && !service.busy
                    text: app.state === DsnoteApp.SttTranscribingFile ||
                          app.speech === DsnoteApp.SttSpeechDecoding ? qsTr("Cancel") : qsTr("Transcribe audio file")
                    onClicked: {
                        if (app.state === DsnoteApp.SttTranscribingFile)
                            app.cancel_transcribe()
                        else if (app.speech == DsnoteApp.SttSpeechDecoding)
                            app.stop_listen()
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
            }
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
            enabled: textArea.text.length === 0 && !app.configured && !app.busy && !service.busy
            text: qsTr("Language is not configured")
            hintText: qsTr("Pull down and select Settings to download language")
        }
    }

    VerticalScrollDecorator {
        flickable: flick
    }

    SttPanel {
        id: panel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        clickable: (app.speech == DsnoteApp.SttSpeechDecoding ||
                   app.state !== DsnoteApp.SttListeningAuto) &&
                   !app.busy && !service.busy && app.connected
        status: {
            switch (app.speech) {
            case DsnoteApp.SttNoSpeech: return 0;
            case DsnoteApp.SttSpeechDetected: return 1;
            case DsnoteApp.SttSpeechDecoding: return 2;
            }
            return 0;
        }
        off: !app.configured || !app.connected
        busy: app.speech !== DsnoteApp.SttSpeechDecoding && (app.busy || service.busy || !app.connected ||
              app.state === DsnoteApp.SttTranscribingFile)
        text: app.intermediate_text
        textPlaceholder: {
            if (app.speech === DsnoteApp.SttSpeechDecoding) return qsTr("Decoding, please wait...")
            if (app.state === DsnoteApp.SttListeningSingleSentence) return qsTr("Say something...")
            if (app.state === DsnoteApp.SttTranscribingFile) return qsTr("Transcribing audio file...")
            if (_settings.speech_mode === Settings.SpeechSingleSentence) return qsTr("Click and say something...")
            return qsTr("Press and say something...")
        }
        textPlaceholderActive: {
            if (!app.connected) return qsTr("Starting...")
            if (!app.configured) return qsTr("Language is not configured")
            if (app.speech === DsnoteApp.SttSpeechDecoding) return qsTr("Decoding, please wait...")
            if (!busy) return qsTr("Say something...")
            if (app.state === DsnoteApp.SttTranscribingFile) return qsTr("Transcribing audio file...")
            return qsTr("Busy...")
        }

        progress: app.transcribe_progress
        onPressed: {
            if (app.speech === DsnoteApp.SttSpeechDecoding || app.state === DsnoteApp.SttTranscribingFile) {
                if (app.state === DsnoteApp.SttTranscribingFile) app.cancel_transcribe()
                else app.stop_listen()
                return
            }

            if (app.state === DsnoteApp.SttListeningSingleSentence) app.stop_listen()
            else app.listen()
        }
        onReleased: {
            if (_settings.speech_mode !== Settings.SpeechSingleSentence) app.stop_listen()
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
            case DsnoteApp.ErrorNoSttService:
                notification.show(qsTr("Unable to start service."))
                break;
            default:
                notification.show(qsTr("Oops! Something went wrong."))
            }
        }
    }
}

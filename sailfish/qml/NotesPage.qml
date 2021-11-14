/* Copyright (C) 2017-2021 Michal Kosciesza <michal@mkiol.net>
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
                    text: app.state === DsnoteApp.SttTranscribingFile ? qsTr("Cancel file transcription") : qsTr("Transcribe audio file")
                    onClicked: {
                        if (app.state === DsnoteApp.SttTranscribingFile)
                            app.cancel_transcribe()
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

        clickable: app.state !== DsnoteApp.SttListeningAuto && app.state !== DsnoteApp.SttTranscribingFile
        speech: app.speech
        off: !app.configured || !app.connected
        busy: app.busy || service.busy || !app.connected || app.state === DsnoteApp.SttTranscribingFile
        text: app.intermediate_text
        textPlaceholder: qsTr("Press and say something...")
        textPlaceholderActive: app.connected ?
                                   app.configured ?
                                       busy ? app.state === DsnoteApp.SttTranscribingFile ? qsTr("Transcribing audio file...") : qsTr("Busy...") :
                                       qsTr("Say something...") : qsTr("Language is not configured") :
                                   qsTr("Starting...")
        progress: app.transcribe_progress
        onPressed: app.listen()
        onReleased: app.stop_listen()
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

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

    readonly property bool inactive: app.intermediate_text.length === 0

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
                busy: app.busy || app.state === DsnoteApp.SttTranscribingFile

                MenuItem {
                    text: qsTr("About %1").arg(APP_NAME)
                    onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
                }

                MenuItem {
                    text: qsTr("Settings")
                    onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
                }

                MenuItem {
                    enabled: app.configured
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
            visible: opacity > 0.0
            opacity: app.configured ? app.busy ? 0.3 : 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
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
            enabled: !app.configured && !app.busy
            text: qsTr("Language is not configured")
            hintText: qsTr("Pull down and select Settings to download language")
        }
    }

    VerticalScrollDecorator {
        flickable: flick
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: app.busy
        size: BusyIndicatorSize.Large
    }

    SilicaItem {
        id: panel
        visible: opacity > 0.0
        opacity: app.configured ? app.busy ? 0.3 : 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 150 } }
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: intermediateLabel.height + 2 * Theme.paddingLarge
        highlighted: mouse.pressed

        readonly property bool active: app.state === DsnoteApp.SttListeningManual || app.state === DsnoteApp.SttListeningAuto || app.state === DsnoteApp.SttTranscribingFile || highlighted
        readonly property color pColor: active ? Theme.highlightColor : Theme.primaryColor
        readonly property color sColor: active ? Theme.secondaryHighlightColor : Theme.secondaryColor

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 1.0; color: Theme.rgba(root.palette.highlightBackgroundColor, 0.05) }
                GradientStop { position: 0.0; color: Theme.rgba(root.palette.highlightBackgroundColor, 0.10) }
            }
        }

        SpeechIndicator {
            id: indicator
            anchors.topMargin: Theme.paddingLarge
            anchors.top: parent.top
            anchors.leftMargin: Theme.paddingSmall
            anchors.left: parent.left
            width: Theme.itemSizeSmall
            color: panel.pColor
            active: app.speech
            off: !app.configured
            Component.onCompleted: {
                height = parent.height / 2
            }

            visible: opacity > 0.0
            opacity: app.state === DsnoteApp.SttTranscribingFile ? 0.0 : 1.0
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        BusyIndicatorWithProgress {
            id: busyIndicator
            size: BusyIndicatorSize.Medium
            anchors.centerIn: indicator
            running: app.state === DsnoteApp.SttTranscribingFile
            progress: app.transcribe_progress
        }

        Label {
            id: intermediateLabel
            anchors.topMargin: Theme.paddingLarge
            anchors.top: parent.top
            anchors.left: indicator.right
            anchors.right: parent.right
            anchors.rightMargin: Theme.horizontalPageMargin
            anchors.leftMargin: Theme.paddingMedium * 0.7
            text: inactive ?
                      app.state === DsnoteApp.SttTranscribingFile ? qsTr("Transcribing audio file...") :
                      app.state === DsnoteApp.SttListeningAuto || app.state === DsnoteApp.SttListeningManual ? qsTr("Say something...") :
                      app.state === DsnoteApp.SttIdle ? qsTr("Press and say something...") :
                      "" : app.intermediate_text
            wrapMode: inactive ? Text.NoWrap : Text.WordWrap
            truncationMode: inactive ? TruncationMode.Fade : TruncationMode.None
            color: inactive ? panel.sColor : panel.pColor
            font.italic: inactive
        }

        MouseArea {
            id: mouse
            enabled: app.configured && !app.busy
            anchors.fill: parent
            onPressed: app.listen()
            onReleased: app.stop_listen()
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
            default:
                notification.show(qsTr("Oops! Something went wrong."))
            }
        }

        onTranscribe_done: notification.show(qsTr("Audio file transcription is completed."))
    }
}

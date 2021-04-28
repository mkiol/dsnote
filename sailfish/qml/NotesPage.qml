/* Copyright (C) 2017-2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Settings 1.0

Page {
    id: root

    readonly property bool configured: app.available_langs.length > 0
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
                busy: app.busy

                MenuItem {
                    text: qsTr("About %1").arg(APP_NAME)
                    onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
                }

                MenuItem {
                    text: qsTr("Settings")
                    onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
                }

                MenuItem {
                    visible: textArea.text.length > 0
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
            opacity: configured && !app.busy ? 1.0 : 0.3
            Behavior on opacity { NumberAnimation { duration: 150 } }
            anchors.bottom: parent.bottom
            text: _settings.note
            verticalAlignment: TextEdit.AlignBottom
            background: null
            labelComponent: null
            onTextChanged: {
                cursorPosition = text.length
                _settings.note = text
                flick.scrollToBottom()
            }
        }

        ViewPlaceholder {
            enabled: !configured && !app.busy
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
        opacity: configured && !app.busy ? 1.0 : 0.3
        Behavior on opacity { NumberAnimation { duration: 150 } }
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: intermediateLabel.height + 2 * Theme.paddingLarge
        highlighted: mouse.pressed

        property color pColor: _settings.speech_mode === Settings.SpeechAutomatic || highlighted ? Theme.highlightColor : Theme.primaryColor
        property color sColor: _settings.speech_mode === Settings.SpeechAutomatic || highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

        SpeechIndicator {
            id: indicator
            anchors.topMargin: Theme.paddingLarge
            anchors.top: parent.top
            anchors.leftMargin: Theme.paddingSmall
            anchors.left: parent.left
            width: Theme.itemSizeSmall
            color: panel.pColor
            active: app.speech
            Component.onCompleted: {
                height = parent.height / 2
            }
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
                      _settings.speech_mode === Settings.SpeechAutomatic || app.speech ?
                      qsTr("Say something...") : qsTr("Tap and say something...") :
                      app.intermediate_text
            wrapMode: inactive ? Text.NoWrap : Text.WordWrap
            truncationMode: inactive ? TruncationMode.Fade : TruncationMode.None
            color: inactive ? panel.sColor : panel.pColor
            font.italic: inactive
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 1.0; color: Theme.rgba(root.palette.highlightBackgroundColor, 0.05) }
                GradientStop { position: 0.0; color: Theme.rgba(root.palette.highlightBackgroundColor, 0.10) }
            }
        }

        MouseArea {
            id: mouse
            enabled: _settings.speech_mode === Settings.SpeechManual
            anchors.fill: parent

            onPressed: app.speech = true
            onReleased: app.speech = false
        }
    }
}


/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
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

    readonly property bool configured: app.available_langs.length > 0

    title: qsTr("Note")

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
            }
        }
        RowLayout {
            visible: configured

            Layout.fillWidth: true

            SpeechIndicator {
                id: indicator
                width: 20
                Layout.leftMargin: 10
                height: speechText.height / 2
                active: app.speech
                Layout.alignment: Qt.AlignTop
                color: palette.text
            }

            TextArea {
                id: speechText
                Layout.fillWidth: true
                readOnly: true
                wrapMode: TextEdit.WordWrap
                placeholderText: app.audio_source_type === Dsnote.SourceFile ?
                                     qsTr("Transcribing audio file...") +(app.progress > -1 ? " " + parseFloat(app.progress * 100).toFixed(1) + "%" : "") :
                                     _settings.speech_mode === Settings.SpeechAutomatic || app.speech ?
                                     qsTr("Say something...") : qsTr("Press and say something...")
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
                id: toolButton

                visible: _settings.speech_mode === Settings.SpeechManual &&
                         configured &&
                         app.audio_source_type === Dsnote.SourceMic
                text: qsTr("Press and hold to speek")
                onPressed: app.speech = true
                onReleased: app.speech = false
            }

            ToolButton {
                visible: textArea.text.length > 0
                text: qsTr("Clear")
                onClicked: _settings.note = ""
            }

            ToolButton {
                visible: app.audio_source_type !== Dsnote.SourceNone
                text: app.audio_source_type === Dsnote.SourceFile ?
                          qsTr("Cancel file transcribtion") :
                          qsTr("Transcribe audio file")
                onClicked: {
                    if (app.audio_source_type === Dsnote.SourceFile)
                        app.cancel_file_source()
                    else
                        fileDialog.open()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                Layout.rightMargin: 10
                visible: !configured
                text: qsTr("No language is configured")
            }

            ComboBox {
                visible: configured
                currentIndex: app.active_lang_idx
                model: app.available_langs
                onActivated: app.set_active_lang_idx(index)
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        folder: shortcuts.home
        selectMultiple: false
        onAccepted: app.set_file_source(fileDialog.fileUrl)
    }
}

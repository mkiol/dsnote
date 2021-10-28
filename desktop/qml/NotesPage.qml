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
            visible: app.configured

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
                placeholderText: app.state === DsnoteApp.SttTranscribingFile ?
                                     qsTr("Transcribing audio file...") + (app.transcribe_progress > 0.0 ? " " + Math.round(app.transcribe_progress * 100) + "%" : "") :
                                     app.state === DsnoteApp.SttListeningAuto || app.speech ?
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
                         (app.state === DsnoteApp.SttListeningManual || app.state === DsnoteApp.SttIdle)
                text: qsTr("Press and hold to speek")
                onPressed: app.listen()
                onReleased: app.stop_listen()
            }

            ToolButton {
                visible: textArea.text.length > 0
                text: qsTr("Clear")
                onClicked: _settings.note = ""
            }

            ToolButton {
                visible: (app.state === DsnoteApp.SttListeningManual ||
                          app.state === DsnoteApp.SttListeningAuto ||
                          app.state === DsnoteApp.SttTranscribingFile ||
                          app.state === DsnoteApp.SttIdle)
                text: app.state === DsnoteApp.SttTranscribingFile ?
                          qsTr("Cancel file transcription") + (app.transcribe_progress > 0.0 ? " (" + Math.round(app.transcribe_progress * 100) + "%)" : "") :
                          qsTr("Transcribe audio file")
                onClicked: {
                    if (app.state === DsnoteApp.SttTranscribingFile)
                        app.cancel_transcribe()
                    else
                        fileDialog.open()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                Layout.rightMargin: 10
                visible: !app.configured
                text: qsTr("No language is configured")
            }

            ComboBox {
                visible: app.configured
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
        onAccepted: app.transcribe_file(fileDialog.fileUrl)
    }
}

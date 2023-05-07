/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3

import org.mkiol.dsnote.Settings 1.0

Page {
    id: root

    title: qsTr("Settings")

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.height + 20
        clip: true

        ScrollIndicator.vertical: ScrollIndicator {}

        ColumnLayout {
            id: column
            x: 10
            y: 10
            width: parent.width - 20
            spacing: 10

            Button {
                Layout.alignment: Qt.AlignHCenter
                enabled: !service.busy
                text: qsTr("Language models")
                onClicked: appWin.push("LangsPage.qml")
            }

            RowLayout {
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Listening mode")
                }
                ComboBox {
                    currentIndex: {
                        if (_settings.speech_mode === Settings.SpeechSingleSentence) return 0
                        if (_settings.speech_mode === Settings.SpeechAutomatic) return 2
                        return 1
                    }
                    model: ListModel {
                        ListElement { text: qsTr("One sentence") }
                        ListElement { text: qsTr("Press and hold") }
                        ListElement { text: qsTr("Always on") }
                    }
                    onActivated: {
                        if (index === 0) {
                            _settings.speech_mode = Settings.SpeechSingleSentence
                        } else if (index === 2) {
                            _settings.speech_mode = Settings.SpeechAutomatic
                        } else {
                            _settings.speech_mode = Settings.SpeechManual
                        }
                    }
                }
            }

            RowLayout {
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Text appending style")
                }
                ComboBox {
                    currentIndex: {
                        if (_settings.insert_mode === Settings.InsertInLine) return 0
                        if (_settings.insert_mode === Settings.InsertNewLine) return 1
                        return 0
                    }
                    model: ListModel {
                        ListElement { text: qsTr("In line") }
                        ListElement { text: qsTr("After line break") }
                    }
                    onActivated: {
                        if (index === 0) {
                            _settings.insert_mode = Settings.InsertInLine
                        } else if (index === 1) {
                            _settings.insert_mode = Settings.InsertNewLine
                        } else {
                            _settings.insert_mode = Settings.InsertInLine
                        }
                    }
                }
            }

            RowLayout {
                Label {
                    text: qsTr("Location of language files")
                }
                TextField {
                    Layout.fillWidth: true
                    text: _settings.models_dir
                    enabled: false

                }
                Button {
                    text: qsTr("Change")
                    onClicked: fileDialog.open()
                }
            }

            CheckBox {
                checked: _settings.translate
                text: qsTr("Translate to English")
                onCheckedChanged: {
                    _settings.translate = checked
                }
            }

            CheckBox {
                checked: _settings.restore_punctuation
                text: qsTr("Restore punctuation")
                onCheckedChanged: {
                    _settings.restore_punctuation = checked
                }
            }

            FileDialog {
                id: fileDialog
                title: qsTr("Please choose a directory")
                selectFolder: true
                selectExisting: true
                folder:  _settings.models_dir_url
                onAccepted: {
                    _settings.models_dir_url = fileUrl
                }
            }
        }
    }
}

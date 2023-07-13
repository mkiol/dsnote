/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
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

    allowedOrientations: Orientation.All

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: root.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("Speech to Text")
            }

            ComboBox {
                label: qsTr("Listening mode")
                currentIndex: {
                    if (_settings.speech_mode === Settings.SpeechSingleSentence) return 0
                    if (_settings.speech_mode === Settings.SpeechAutomatic) return 2
                    return 1
                }
                menu: ContextMenu {
                    MenuItem { text: qsTr("One sentence") }
                    MenuItem { text: qsTr("Press and hold") }
                    MenuItem { text: qsTr("Always on") }
                }
                onCurrentIndexChanged: {
                    if (currentIndex === 0) {
                        _settings.speech_mode = Settings.SpeechSingleSentence
                    } else if (currentIndex === 2) {
                        _settings.speech_mode = Settings.SpeechAutomatic
                    } else {
                        _settings.speech_mode = Settings.SpeechManual
                    }
                }
                description: qsTr("One sentence: Clicking on the 'Listen' button starts listening, which ends when the first sentence is recognized.\n" +
                                  "Press and hold: Pressing and holding the 'Listen' button enables listening. When you stop holding, listening will turn off.\n" +
                                  "Always on: After clicking on the 'Listen' button, listening is always turn on.")
            }

            ComboBox {
                label: qsTr("Text appending style")
                currentIndex: {
                    if (_settings.insert_mode === Settings.InsertInLine) return 0
                    if (_settings.insert_mode === Settings.InsertNewLine) return 1
                    return 0
                }
                menu: ContextMenu {
                    MenuItem { text: qsTr("In line") }
                    MenuItem { text: qsTr("After line break") }
                }
                onCurrentIndexChanged: {
                    if (currentIndex === 0) {
                        _settings.insert_mode = Settings.InsertInLine
                    } else if (currentIndex === 1) {
                        _settings.insert_mode = Settings.InsertNewLine
                    } else {
                        _settings.insert_mode = Settings.InsertInLine
                    }
                }
                description: qsTr("Text is appended to the note in the same line or after line break.")
            }

            TextSwitch {
                visible: _settings.py_supported()
                checked: _settings.restore_punctuation
                automaticCheck: false
                text: qsTr("Restore punctuation")
                description: qsTr("Enable advanced punctuation restoration after speech recognition. To make it work, " +
                                  "make sure you have enabled 'Punctuation' model for your language.") + " " +
                             qsTr("When this option is enabled model initialization takes much longer and memory usage is much higher.")
                onClicked: {
                    _settings.restore_punctuation = !_settings.restore_punctuation
                }
            }

            PaddedLabel {
                visible: _settings.py_supported() && _settings.restore_punctuation && !app.ttt_configured
                color: Theme.errorColor
                text: qsTr("To make 'Restore punctuation' work, download 'Punctuation' model.")
            }

            TextSwitch {
                visible: false
                checked: _settings.translate
                automaticCheck: false
                text: qsTr("Translate to English")
                description: qsTr("Translate decoded text to English. This option works only with Whisper models.")
                onClicked: {
                    _settings.translate = !_settings.translate
                }
            }

            SectionHeader {
                text: qsTr("Other")
            }

            ItemBox {
                title: qsTr("Location of language files")
                value: _settings.models_dir_name
                description: qsTr("Directory where language files are downloaded to and stored.")

                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Change")
                        onClicked: {
                            var obj = pageStack.push(Qt.resolvedUrl("DirPage.qml"), {path: _settings.models_dir});
                            obj.accepted.connect(function() {
                                _settings.models_dir = obj.selectedPath
                            })
                        }
                    }
                    MenuItem {
                        text: qsTr("Set default")
                        onClicked: {
                            _settings.models_dir = ""
                        }
                    }
                }
            }
        }
    }

    RemorsePopup {
        id: remorse
    }

    VerticalScrollDecorator {
        flickable: flick
    }
}

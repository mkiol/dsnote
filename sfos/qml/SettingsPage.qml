/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
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
                text: qsTr("User Interface")
            }

            TextSwitch {
                checked: _settings.keep_last_note
                automaticCheck: false
                text: qsTr("Remember the last note")
                description: qsTr("The note will be saved automatically, so when you restart the app, your last note will always be available.")
                onClicked: {
                    _settings.keep_last_note = !_settings.keep_last_note
                }
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
                description: "<i>" + qsTr("One sentence") + "</i>" + " — " + qsTr("Clicking on the %1 button starts listening, which ends when the first sentence is recognized.")
                                .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                             "<i>" + qsTr("Press and hold") + "</i>" + " — " + qsTr("Pressing and holding the %1 button enables listening. When you stop holding, listening will turn off.")
                                .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                             "<i>" + qsTr("Always on") + "</i>" + " — " + qsTr("After clicking on the %1 button, listening is always turn on.")
                                .arg("<i>" + qsTr("Listen") + "</i>")
            }

            SectionHeader {
                text: qsTr("Text to Speech")
            }

            Slider {
                id: speechSpeedSlider

                opacity: enabled ? 1.0 : Theme.opacityLow
                width: parent.width
                minimumValue: 1
                maximumValue: 20
                stepSize: 1
                handleVisible: true
                label: qsTr("Speech speed")
                value: _settings.speech_speed
                valueText: "x " + (value / 10).toFixed(1)
                onValueChanged: _settings.speech_speed = value

                Connections {
                    target: _settings
                    onSpeech_speedChanged: {
                        speechSpeedSlider.value = _settings.speech_speed
                    }
                }
            }

            TextSwitch {
                checked: _settings.diacritizer_enabled
                automaticCheck: false
                text: qsTr("Restore diacritics before speech synthesis")
                description: qsTr("This works only for Arabic language.")
                onClicked: {
                    _settings.diacritizer_enabled = !_settings.diacritizer_enabled
                }
            }

            SectionHeader {
                text: qsTr("Other")
            }

            TextSwitch {
                checked: _settings.cache_policy === Settings.CacheRemove
                automaticCheck: false
                text: qsTr("Clear cache on close")
                description: qsTr("When closing, delete all cached audio files.")
                onClicked: {
                    _settings.cache_policy =
                            _settings.cache_policy === Settings.CacheRemove ?
                                Settings.CacheNoRemove : Settings.CacheRemove
                }
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

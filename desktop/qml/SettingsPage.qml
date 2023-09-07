/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Settings 1.0

DialogPage {
    id: root

    property bool verticalMode: width < appWin.verticalWidthThreshold

    title: qsTr("Settings")

    SectionLabel {
        text: qsTr("Speech to Text")
    }

    GridLayout {
        id: grid

        Layout.fillWidth: true
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            Layout.fillWidth: true
            text: qsTr("Listening mode")
        }
        ComboBox {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            currentIndex: {
                if (_settings.speech_mode === Settings.SpeechSingleSentence) return 0
                if (_settings.speech_mode === Settings.SpeechAutomatic) return 2
                return 1
            }
            model: [
                qsTr("One sentence"),
                qsTr("Press and hold"),
                qsTr("Always on")
            ]
            onActivated: {
                if (index === 0) {
                    _settings.speech_mode = Settings.SpeechSingleSentence
                } else if (index === 2) {
                    _settings.speech_mode = Settings.SpeechAutomatic
                } else {
                    _settings.speech_mode = Settings.SpeechManual
                }
            }

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: "<i>" + qsTr("One sentence") + "</i>" + " — " + qsTr("Clicking on the %1 button starts listening, which ends when the first sentence is recognized.")
                            .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                          "<i>" + qsTr("Press and hold") + "</i>" + " — " + qsTr("Pressing and holding the %1 button enables listening. When you stop holding, listening will turn off.")
                            .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                          "<i>" + qsTr("Always on") + "</i>" + " — " + qsTr("After clicking on the %1 button, listening is always turn on.")
                            .arg("<i>" + qsTr("Listen") + "</i>")
        }
    }



    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            Layout.fillWidth: true
            text: qsTr("Text appending style")
        }
        ComboBox {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            currentIndex: {
                if (_settings.insert_mode === Settings.InsertInLine) return 0
                if (_settings.insert_mode === Settings.InsertNewLine) return 1
                return 0
            }
            model: [
                qsTr("In line"),
                qsTr("After line break")
            ]
            onActivated: {
                if (index === 0) {
                    _settings.insert_mode = Settings.InsertInLine
                } else if (index === 1) {
                    _settings.insert_mode = Settings.InsertNewLine
                } else {
                    _settings.insert_mode = Settings.InsertInLine
                }
            }

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Text is appended to the note in the same line or after line break.")
        }
    }

    CheckBox {
        id: puncCheckBox

        visible: _settings.py_supported()
        checked: _settings.restore_punctuation
        text: qsTr("Restore punctuation")
        onCheckedChanged: {
            _settings.restore_punctuation = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Enable advanced punctuation restoration after speech recognition. To make it work, " +
                           "make sure you have enabled %1 model for your language.")
                      .arg("<i>" + qsTr("Punctuation") + "</i>") + " " +
                      qsTr("When this option is enabled model initialization takes much longer and memory usage is much higher.")
    }

    Label {
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        visible: _settings.py_supported() && _settings.restore_punctuation && !app.ttt_configured
        color: "red"
        text: qsTr("To make %1 work, download %2 model.")
                .arg("<i>" + qsTr("Restore punctuation") + "</i>").arg("<i>" + qsTr("Punctuation") + "</i>")
    }

    CheckBox {
        visible: _settings.gpu_supported()
        checked: _settings.whisper_use_gpu
        text: qsTr("Use GPU acceleration for Whisper")
        onCheckedChanged: {
            _settings.whisper_use_gpu = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTr("If a suitable GPU device is found in the system, it will be used to accelerate processing.") + " " +
                      qsTr("GPU hardware acceleration significantly reduces the time of decoding.") + " " +
                      qsTr("Disable this option if you observe problems when using Speech to Text with Whisper models.")
    }

    Label {
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        visible: _settings.gpu_supported() && _settings.whisper_use_gpu && _settings.gpu_devices.length <= 1
        color: "red"
        text: qsTr("A suitable GPU device could not be found.")
    }

    GridLayout {
        visible: _settings.gpu_supported() && _settings.whisper_use_gpu && _settings.gpu_devices.length > 1
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            text: qsTr("GPU device")
        }
        ComboBox {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            currentIndex: _settings.gpu_device_idx
            model: _settings.gpu_devices
            onActivated: {
                _settings.gpu_device_idx = index
            }

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Select preferred GPU device for hardware acceleration.")
        }
    }

    SectionLabel {
        text: qsTr("Text to Speech")
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            text: qsTr("Speech speed")
        }

        Slider {
            id: speechSpeedSlider

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered && !pressed
            ToolTip.text: qsTr("Change to make synthesized speech slower or faster.")

            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            snapMode: Slider.SnapAlways
            stepSize: 1
            from: -2
            to: 2
            value: {
                switch(_settings.speech_speed) {
                case Settings.SpeechSpeedVerySlow: return -2
                case Settings.SpeechSpeedSlow: return -1
                case Settings.SpeechSpeedNormal: return 0
                case Settings.SpeechSpeedFast: return 1
                case Settings.SpeechSpeedVeryFast: return 2
                }
            }

            onValueChanged: {
                switch(value) {
                case -2: _settings.speech_speed = Settings.SpeechSpeedVerySlow; break;
                case -1: _settings.speech_speed = Settings.SpeechSpeedSlow; break;
                case 0: _settings.speech_speed = Settings.SpeechSpeedNormal; break;
                case 1: _settings.speech_speed = Settings.SpeechSpeedFast; break;
                case 2: _settings.speech_speed = Settings.SpeechSpeedVeryFast; break;
                }
            }

            Connections {
                target: _settings
                onSpeech_speedChanged: {
                    switch(_settings.speech_speed) {
                    case Settings.SpeechSpeedVerySlow: speechSpeedSlider.value = -2; break
                    case Settings.SpeechSpeedSlow: speechSpeedSlider.value = -1; break
                    case Settings.SpeechSpeedNormal: speechSpeedSlider.value = 0; break
                    case Settings.SpeechSpeedFast: speechSpeedSlider.value = 1; break
                    case Settings.SpeechSpeedVeryFast: speechSpeedSlider.value = 2; break
                    }
                }
            }

            Label {
                anchors.bottom: parent.handle.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: appWin.padding
                text: {
                    switch(parent.value) {
                    case -2: return qsTr("Very slow");
                    case -1: return qsTr("Slow");
                    case 0: return qsTr("Normal");
                    case 1: return qsTr("Fast");
                    case 2: return qsTr("Very fast");
                    }
                }
            }
        }
    }

    SectionLabel {
        text: qsTr("User Interface")
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            Layout.fillWidth: true
            text: qsTr("Font size in text editor")
            wrapMode: Text.Wrap
        }
        SpinBox {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            from: 4
            to: 25
            stepSize: 1
            value: _settings.font_size < 5 ? 4 : _settings.font_size
            textFromValue: function(value) {
                return value < 5 ? qsTr("Auto") : value.toString() + " px"
            }
            valueFromText: function(text) {
                if (text === qsTr("Auto")) return 4
                return parseInt(text);
            }
            onValueChanged: {
                _settings.font_size = value;
            }
            Component.onCompleted: {
                contentItem.color = palette.text
            }
        }
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 2
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            Layout.fillWidth: true
            text: qsTr("Graphical style") + " (" + qsTr("advanced option") + ")"
            wrapMode: Text.Wrap
        }
        ComboBox {
            Layout.fillWidth: verticalMode
            Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
            currentIndex: _settings.qt_style_idx
            model: _settings.qt_styles()
            onActivated: _settings.qt_style_idx = index

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Application graphical interface style.") + " " +
                          qsTr("Change if you observe problems with incorrect colors under a dark theme.")
        }
    }

    Label {
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        visible: _settings.restart_required
        color: "red"
        text: qsTr("Restart an application to apply changes.")
    }

    SectionLabel {
        text: qsTr("Other")
    }

    GridLayout {
        columns: root.verticalMode ? 1 : 3
        columnSpacing: appWin.padding
        rowSpacing: appWin.padding

        Label {
            text: qsTr("Location of language files")
        }
        TextField {
            Layout.fillWidth: true
            text: _settings.models_dir
            enabled: false

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: _settings.models_dir
        }
        Button {
            text: qsTr("Change")
            onClicked: directoryDialog.open()

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Directory where language files are downloaded to and stored.")
        }
    }

    Dialogs.FileDialog {
        id: directoryDialog
        title: qsTr("Select Directory")
        selectFolder: true
        selectExisting: true
        folder:  _settings.models_dir_url
        onAccepted: {
            _settings.models_dir_url = fileUrl
        }
    }
}

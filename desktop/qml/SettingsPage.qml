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
        checked: _settings.translate
        text: qsTr("Translate to English")
        onCheckedChanged: {
            _settings.translate = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Translate decoded text to English. This option works only with Whisper models.")
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

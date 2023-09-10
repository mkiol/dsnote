/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ToolBar {
    id: root

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            ToolButton {
                id: menuButton
                icon.name: "open-menu-symbolic"
                Layout.alignment: Qt.AlignLeft
                onClicked: menuMenu.open()

                Menu {
                    id: menuMenu
                    y: menuButton.height

                    MenuItem {
                        text: qsTr("Settings")
                        icon.name: "document-properties-symbolic"
                        onClicked: appWin.openDialog("SettingsPage.qml")
                    }

                    MenuItem {
                        text: qsTr("About %1").arg(APP_NAME)
                        icon.name: "starred-symbolic"
                        onClicked: appWin.openDialog("AboutPage.qml")
                    }

                    MenuItem {
                        icon.name: "application-exit-symbolic"
                        text: qsTr("Quit")
                        onClicked: {
                            appWin.close()
                            Qt.quit()
                        }
                    }
                }
            }

            ToolButton {
                id: fileButton
                enabled: app.stt_configured || app.tts_configured
                opacity: enabled ? 1.0 : 0.6
                Layout.alignment: Qt.AlignLeft
                text: qsTr("File")
                onClicked: fileMenu.open()

                Menu {
                    id: fileMenu
                    y: fileButton.height

                    MenuItem {
                        text: qsTr("Transcribe audio file")
                        icon.name: "document-open-symbolic"
                        enabled: !_settings.translator_mode && app.stt_configured &&
                                 (app.state === DsnoteApp.StateListeningManual ||
                                  app.state === DsnoteApp.StateListeningAuto ||
                                  app.state === DsnoteApp.StateListeningSingleSentence ||
                                  app.state === DsnoteApp.StateIdle ||
                                  app.state === DsnoteApp.StatePlayingSpeech)
                        onClicked: fileReadDialog.open()

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Convert audio file to text.")
                    }

                    MenuItem {
                        text: qsTr("Save to audio file")
                        icon.name: "document-save-symbolic"
                        enabled: app.note.length !== 0 && app.tts_configured &&
                                 (app.state === DsnoteApp.StateListeningManual ||
                                  app.state === DsnoteApp.StateListeningAuto ||
                                  app.state === DsnoteApp.StateListeningSingleSentence ||
                                  app.state === DsnoteApp.StateIdle ||
                                  app.state === DsnoteApp.StatePlayingSpeech)
                        onClicked: {
                            fileWriteDialog.translated = false
                            fileWriteDialog.open()
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Convert text to audio and save as WAV file.")
                    }

                    MenuItem {
                        text: qsTr("Save the translation to audio file")
                        icon.name: "document-save-symbolic"
                        enabled: app.translated_text.length !== 0 && _settings.translator_mode &&
                                 app.tts_configured && app.active_tts_model_for_out_mnt.length !== 0 &&
                                 (app.state === DsnoteApp.StateListeningManual ||
                                  app.state === DsnoteApp.StateListeningAuto ||
                                  app.state === DsnoteApp.StateListeningSingleSentence ||
                                  app.state === DsnoteApp.StateIdle ||
                                  app.state === DsnoteApp.StatePlayingSpeech)
                        onClicked: {
                            fileWriteDialog.translated = true
                            fileWriteDialog.open()
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Convert translated text to audio and save as WAV file.")
                    }
                }
            }

            ToolButton {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Languages")
                onClicked: appWin.openDialog("LangsPage.qml")

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Set languages and download models.")
            }

            Item {
                Layout.fillWidth: true
                height: 1
            }

            ToolButton {
                id: notepadButton

                enabled: _settings.translator_mode
                Layout.alignment: Qt.AlignRight
                text: qsTr("Notepad")
                checkable: true
                checked: !_settings.translator_mode
                onClicked: {
                    if (_settings.translator_mode) {
                        _settings.translator_mode = false
                    }
                }

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Switch to Notepad")
            }

            ToolButton {
                id: translatorButton

                enabled: !_settings.translator_mode
                Layout.alignment: Qt.AlignRight
                text: qsTr("Translator")
                checkable: true
                checked: _settings.translator_mode
                onClicked: {
                    if (!_settings.translator_mode) {
                        _settings.translator_mode = true
                        _settings.hint_translator = false
                    }
                }

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Switch to Translator")
            }
        }
    }

    Dialogs.FileDialog {
        id: fileReadDialog
        title: qsTr("Open File")
        nameFilters: [ '*.wav', '*.mp3', '*.ogg', '*.flac', '*.m4a', '*.aac', '*.opus' ]
        folder: _settings.file_open_dir_url
        selectExisting: true
        selectMultiple: false
        onAccepted: {
            app.transcribe_file(fileReadDialog.fileUrl)
            _settings.file_open_dir_url = fileReadDialog.fileUrl
        }
    }

    Dialogs.FileDialog {
        id: fileWriteDialog
        property bool translated: false

        //defaultSuffix: "wav"
        title: qsTr("Save File")
        nameFilters: [ "Wave (*.wav)", "MP3 (*.mp3)", "Ogg Vorbis (*.ogg)", qsTr("All files") + " (*)" ]
        folder: _settings.file_save_dir_url
        selectExisting: false
        selectMultiple: false
        onAccepted: {
            if (_settings.translator_mode) {
                app.speech_to_file_translator(translated, fileWriteDialog.fileUrl)
                _settings.file_save_dir_url = fileWriteDialog.fileUrl
            } else {
                app.speech_to_file(fileWriteDialog.fileUrl)
                _settings.file_save_dir_url = fileWriteDialog.fileUrl
            }
        }
    }
}

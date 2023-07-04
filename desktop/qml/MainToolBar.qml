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
                        icon.source: _settings.app_icon()
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
                        enabled: app.stt_configured &&
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
                        text: qsTr("Save speech to audio file")
                        icon.name: "document-save-symbolic"
                        enabled: app.tts_configured &&
                                 (app.state === DsnoteApp.StateListeningManual ||
                                  app.state === DsnoteApp.StateListeningAuto ||
                                  app.state === DsnoteApp.StateListeningSingleSentence ||
                                  app.state === DsnoteApp.StateIdle ||
                                  app.state === DsnoteApp.StatePlayingSpeech)
                        onClicked: fileWriteDialog.open()

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Convert text to audio and save as WAV file.")
                    }
                }
            }

            ToolButton {
                id: editButton
                enabled: app.stt_configured || app.tts_configured
                opacity: enabled ? 1.0 : 0.6
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Edit")
                onClicked: editMenu.open()

                Menu {
                    id: editMenu
                    y: editButton.height

                    MenuItem {
                        text: qsTr("Undo")
                        icon.name: "edit-undo-symbolic"
                        enabled: textArea.canUndo
                        onClicked: textArea.undo()
                    }

                    MenuItem {
                        text: qsTr("Redo")
                        icon.name: "edit-redo-symbolic"
                        enabled: textArea.canRedo
                        onClicked: textArea.redo()
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: qsTr("Copy All")
                        icon.name: "edit-copy-symbolic"
                        enabled: textArea.text.length > 0
                        onClicked: app.copy_to_clipboard()
                    }

                    MenuItem {
                        text: qsTr("Paste")
                        icon.name: "edit-paste-symbolic"
                        enabled: textArea.canPaste
                        onClicked: textArea.paste()
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: qsTr("Clear")
                        icon.name: "edit-clear-all-symbolic"
                        enabled: textArea.text.length > 0
                        onClicked: textArea.clear()
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
        }

        ModelsSwitcher {
            id: modelsSwitcher
            enabled: !appWin.compactMode
            visible: enabled
        }

        ModelsSwitcherCompact {
            id: modelsSwitcherCompact
            enabled: appWin.compactMode
            visible: enabled
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
        defaultSuffix: "wav"
        title: qsTr("Save File")
        nameFilters: [ qsTr("MS Wave") + " (*.wav)", qsTr("All files") + " (*)" ]
        folder: _settings.file_save_dir_url
        selectExisting: false
        selectMultiple: false
        onAccepted: {
//            if (app.file_exists(fileWriteDialog.fileUrl)) {
//                fileOverwriteDialog.fileUrl = fileWriteDialog.fileUrl
//                fileOverwriteDialog.open()
//            } else {
                app.speech_to_file(fileWriteDialog.fileUrl)
                _settings.file_save_dir_url = fileWriteDialog.fileUrl
//            }
        }
    }

//    Dialogs.MessageDialog {
//        id: fileOverwriteDialog
//        property url fileUrl: ""
//        title: qsTr("Overwrite File?")
//        text: qsTr("The file already exists. Do you wish to overwrite it?")
//        standardButtons: Dialogs.StandardButton.Ok | Dialogs.StandardButton.Cancel
//        onAccepted: app.speech_to_file(fileUrl)
//    }

    function update() {
        if (app.busy || service.busy) return;
        if (app.stt_configured) {
            modelsSwitcher.sttIndex = app.active_stt_model_idx
            modelsSwitcherCompact.sttIndex = app.active_stt_model_idx
        }
        if (app.tts_configured) {
            modelsSwitcher.ttsIndex = app.active_tts_model_idx
            modelsSwitcherCompact.ttsIndex = app.active_tts_model_idx
        }
    }

    Connections {
        target: app
        onAvailable_stt_models_changed: root.update()
        onAvailable_tts_models_changed: root.update()
        onBusyChanged: root.update()
        onStt_configuredChanged: root.update()
        onTts_configuredChanged: root.update()
        onActive_stt_model_idxChanged: root.update()
        onActive_tts_model_idxChanged: root.update()
    }
    Connections {
        target: service
        onBusyChanged: root.update()
    }
}

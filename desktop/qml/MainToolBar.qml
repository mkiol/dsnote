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

    function openDialog(file) {
        var cmp = Qt.createComponent(file)
        if (cmp.status === Component.Ready) {
            var dialog = cmp.createObject(appWin);
            dialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            ToolButton {
                id: fileButton
                icon.name: "open-menu-symbolic"
                Layout.alignment: Qt.AlignLeft
                onClicked: fileMenu.open()

                Menu {
                    id: fileMenu
                    y: fileButton.height

                    MenuItem {
                        text: qsTr("&Settings")
                        icon.name: "document-properties-symbolic"
                        onClicked: root.openDialog("SettingsPage.qml")
                    }

                    MenuItem {
                        text: qsTr("&About %1").arg(APP_NAME)
                        icon.source: _settings.app_icon()
                        onClicked: root.openDialog("AboutPage.qml")
                    }

                    MenuItem {
                        icon.name: "application-exit-symbolic"
                        text: qsTr("&Quit")
                        onClicked: Qt.quit()
                    }
                }
            }

            ToolButton {
                id: editButton
                enabled: app.stt_configured || app.tts_configured
                opacity: enabled ? 1.0 : 0.6
                Layout.alignment: Qt.AlignLeft
                text:  qsTr("&Edit")
                onClicked: editMenu.open()

                Menu {
                    id: editMenu
                    y: editButton.height

                    MenuItem {
                        text: qsTr("&Undo")
                        icon.name: "edit-undo-symbolic"
                        enabled: textArea.canUndo
                        onClicked: textArea.undo()
                    }

                    MenuItem {
                        text: qsTr("&Redo")
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
                text: qsTr("Transcribe audio file")
                enabled: app.stt_configured &&
                         (app.state === DsnoteApp.StateListeningManual ||
                          app.state === DsnoteApp.StateListeningAuto ||
                          app.state === DsnoteApp.StateListeningSingleSentence ||
                          app.state === DsnoteApp.StateIdle ||
                          app.state === DsnoteApp.StatePlayingSpeech)
                opacity: enabled ? 1.0 : 0.6
                onClicked: fileDialog.open()
            }

            ToolButton {
                Layout.alignment: Qt.AlignLeft
                text: qsTr("&Languages")
                onClicked: root.openDialog("LangsPage.qml")
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Frame {
                Layout.fillWidth: true
                background: Item {}

                RowLayout {
                    anchors.fill: parent

                    ToolButton {
                        enabled: false
                        icon.name: "audio-input-microphone-symbolic"
                    }

                    ComboBox {
                        id: sttCombo
                        Layout.fillWidth: true
                        enabled: app.stt_configured
                        currentIndex: app.active_stt_model_idx
                        model: app.available_stt_models
                        onActivated: app.set_active_stt_model_idx(index)
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                background: Item {}

                RowLayout {
                    anchors.fill: parent

                    ToolButton {
                        enabled: false
                        icon.name: "audio-speakers-symbolic"
                    }

                    ComboBox {
                        id: ttsCombo
                        Layout.fillWidth: true
                        enabled: app.tts_configured
                        currentIndex: app.active_tts_model_idx
                        model: app.available_tts_models
                        onActivated: app.set_active_tts_model_idx(index)
                    }
                }
            }
        }
    }

    Dialogs.FileDialog {
        id: fileDialog
        title: qsTr("Please choose a file")
        folder: shortcuts.home
        selectMultiple: false
        onAccepted: app.transcribe_file(fileDialog.fileUrl)
    }

    function update() {
        if (app.busy || service.busy) return;

        if (app.stt_configured)
            sttCombo.currentIndex = app.active_stt_model_idx
        if (app.tts_configured)
            ttsCombo.currentIndex = app.active_tts_model_idx
    }

    Connections {
        target: app
        onAvailable_stt_models_changed: root.update()
        onAvailable_tts_models_changed: root.update()
        onBusyChanged: root.update()
        onStt_configuredChanged: root.update()
        onTts_configuredChanged: root.update()
    }
    Connections {
        target: service
        onBusyChanged: root.update()
    }
}

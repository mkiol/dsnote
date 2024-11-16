/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ToolBar {
    id: root

    property bool verticalMode: (toolsRow.width + tabRow.width + 4 * appWin.padding) > appWin.width

    ColumnLayout {
        anchors.fill: parent

        GridLayout {
            Layout.fillWidth: true
            columns: root.verticalMode ? 1 : 2

            RowLayout {
                RowLayout {
                    id: toolsRow

                    Layout.alignment: Qt.AlignLeft

                    ToolButton {
                        id: menuButton

                        display: AbstractButton.IconOnly
                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Open Menu")
                        hoverEnabled: true

                        action: Action {
                            icon.name: "open-menu-symbolic"
                            text: qsTr("Menu")
                            shortcut: "F10"
                            onTriggered: {
                                menuButton.focus = true
                                menuMenu.open()
                            }
                        }

                        Menu {
                            id: menuMenu
                            y: menuButton.height

                            MenuItem {
                                id: settingMenuItem

                                action: Action {
                                    icon.name: "document-properties-symbolic"
                                    text: qsTr("Settings")
                                    shortcut: StandardKey.Preferences
                                    onTriggered: appWin.openDialog("SettingsPage.qml")
                                }
                            }

                            MenuItem {
                                action: Action {
                                    icon.name: "starred-symbolic"
                                    text: qsTr("About %1").arg(APP_NAME)
                                    onTriggered: appWin.openDialog("AboutPage.qml")
                                }
                            }

                            MenuItem {
                                action: Action {
                                    icon.name: "application-exit-symbolic"
                                    text: qsTr("Quit")
                                    shortcut: StandardKey.Quit
                                    onTriggered:  {
                                        appWin.close()
                                        Qt.quit()
                                    }
                                }
                            }
                        }

                        Item {
                            height: 1
                            Layout.fillWidth: true
                        }
                    }

                    ToolButton {
                        id: fileButton

                        action: Action {
                            text: qsTr("File")
                            shortcut: "Ctrl+F"
                            onTriggered: fileMenu.open()
                        }

                        Menu {
                            id: fileMenu

                            y: fileButton.height

                            MenuItem {
                                enabled: !app.busy && app.state === DsnoteApp.StateIdle
                                action: Action {
                                    icon.name: "document-open-symbolic"
                                    text: qsTr("Import from a file")
                                    shortcut: StandardKey.Open
                                    onTriggered: fileReadDialog.open()
                                }

                                ToolTip.visible: hovered
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                                ToolTip.text: qsTr("Import a note from a text file, subtitle file, video or audio file.") + " " +
                                              qsTr("If the imported file is an audio or video file, the audio is converted to text.")
                                hoverEnabled: true
                            }

                            MenuSeparator {}

                            MenuItem {
                                action: Action {
                                    enabled: app.note.length !== 0 && !app.busy && app.state === DsnoteApp.StateIdle
                                    icon.name: "document-save-symbolic"
                                    text: qsTr("Export to a file")
                                    shortcut: StandardKey.Save
                                    onTriggered: appWin.openDialog("ExportFilePage.qml", {"translated": false})
                                }


                                ToolTip.visible: hovered
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                                ToolTip.text: qsTr("Export the current note to a text file, subtitle file or audio file.")
                                hoverEnabled: true
                            }

                            MenuItem {
                                action: Action {
                                    enabled: app.translated_text.length !== 0 && _settings.translator_mode && !app.busy && app.state === DsnoteApp.StateIdle
                                    icon.name: "document-save-symbolic"
                                    text: qsTr("Export the translation to a file")
                                    onTriggered: appWin.openDialog("ExportFilePage.qml", {"translated": true})
                                }
                                visible: enabled
                                height: visible ? implicitHeight : 0

                                ToolTip.visible: hovered
                                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                                ToolTip.text: qsTr("Export the translated note to a text file, subtitle file or audio file.")
                                hoverEnabled: true
                            }
                        }
                    }

                    ToolButton {
                        action: Action {
                            enabled: !app.busy
                            text: qsTr("Languages")
                            shortcut: "Ctrl+L"
                            onTriggered: appWin.openDialog("LangsPage.qml")
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Set languages and download models.")
                        hoverEnabled: true
                    }

                    ToolButton {
                        id: repairTextButton

                        visible: _settings.show_repair_text && !_settings.translator_mode
                        action: Action {
                            enabled: !app.busy && _settings.show_repair_text && !_settings.translator_mode
                            text: qsTr("Repair text")
                            onTriggered: rapairTextMenu.open()
                        }

                        Menu {
                            id: rapairTextMenu

                            y: repairTextButton.height

                            MenuItem {
                                action: Action {
                                    enabled: !app.busy && app.state === DsnoteApp.StateIdle &&
                                             app.feature_punctuator && app.ttt_punctuation_configured &&
                                             app.note.length !== 0
                                    text: qsTr("Restore punctuation")
                                    onTriggered: app.restore_punctuation()
                                }
                            }

                            MenuItem {
                                action: Action {
                                    enabled: !app.busy && app.state === DsnoteApp.StateIdle &&
                                             app.ttt_diacritizer_ar_configured &&
                                             app.note.length !== 0
                                    text: qsTr("Restore diacritical marks (%1)").arg(qsTr("Arabic"))
                                    onTriggered: app.restore_diacritics_ar()
                                }
                            }

                            MenuItem {
                                action: Action {
                                    enabled: !app.busy && app.state === DsnoteApp.StateIdle &&
                                             app.feature_diacritizer_he && app.ttt_diacritizer_he_configured &&
                                             app.note.length !== 0
                                    text: qsTr("Restore diacritical marks (%1)").arg(qsTr("Hebrew"))
                                    onTriggered: app.restore_diacritics_he()
                                }
                            }
                        }
                    }

                    ToolButton {
                        id: voicesButton

                        visible: enabled
                        Layout.alignment: Qt.AlignLeft
                        action: Action {
                            enabled: (app.tts_ref_voice_needed && !_settings.translator_mode) ||
                                     (_settings.translator_mode && (app.tts_for_in_mnt_ref_voice_needed || app.tts_for_out_mnt_ref_voice_needed))
                            text: qsTr("Voice samples")
                            shortcut: "Ctrl+V"
                            onTriggered: appWin.openDialog("VoiceMgmtPage.qml")
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Voice samples are used in speech synthesis with voice cloning.")
                        hoverEnabled: true

                        Dot {
                            visible: app.available_tts_ref_voices.length === 0 && ((app.tts_ref_voice_needed && !_settings.translator_mode) ||
                                                                                   (_settings.translator_mode && (app.tts_for_in_mnt_ref_voice_needed || app.tts_for_out_mnt_ref_voice_needed)))
                            size: menuButton.height / 5
                            anchors {
                                right: voicesButton.right
                                rightMargin: size / 2
                                top: voicesButton.top
                                topMargin: size / 2
                            }
                        }
                    }
                }

                Item {
                    height: 1
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                Item {
                    visible: !root.verticalMode
                    height: 1
                    Layout.fillWidth: true
                }

                TabBar {
                    id: tabRow

                    currentIndex: _settings.translator_mode ? 1 : 0
                    onCurrentIndexChanged: {
                        switch(currentIndex) {
                        case 0:
                            _settings.translator_mode = false
                            break;
                        case 1:
                            _settings.translator_mode = true
                            break;
                        }
                    }

                    TabButton {
                        width: implicitWidth
                        action: Action {
                            text: qsTr("Notepad")
                            shortcut: "Ctrl+N"
                            onTriggered: {
                                checked = true
                            }
                        }
                    }

                    TabButton {
                        width: implicitWidth
                        action: Action {
                            enabled: app.feature_translator
                            text: qsTr("Translator")
                            shortcut: "Ctrl+T"
                            onTriggered: {
                                checked = true
                            }
                        }
                    }
                }

                Item {
                    visible: root.verticalMode
                    height: 1
                    Layout.fillWidth: true
                }
            }

        }
    }

    Dialogs.FileDialog {
        id: fileReadDialog

        title: qsTr("Import from a file")
        nameFilters: [
            qsTr("All supported files") + " (*.txt *.srt *.ass *.ssa *.sub *.vtt *.wav *.mp3 *.ogg *.oga *.ogx *.opus *.spx *.flac *.m4a *.aac *.mp4 *.mkv *.ogv *.webm)",
            qsTr("All files") + " (*)"]
        folder: _settings.file_open_dir_url
        selectExisting: true
        selectMultiple: false
        onAccepted: {
            var file_path =
                _settings.file_path_from_url(fileReadDialog.fileUrl)
            appWin.openFile(file_path)
        }
    }
}

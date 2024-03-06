/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
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
                        id: settingMenuItem

                        text: qsTr("Settings")
                        icon.name: "document-properties-symbolic"
                        onClicked: appWin.openDialog("SettingsPage.qml")

                        Dot {
                            size: settingMenuItem.height / 5
                            visible: _settings.hint_addons
                            anchors {
                                right: settingMenuItem.right
                                rightMargin: appWin.padding
                                verticalCenter: settingMenuItem.verticalCenter
                            }
                        }
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

                Dot {
                    size: menuButton.height / 5
                    visible: _settings.hint_addons
                    anchors {
                        right: menuButton.right
                        rightMargin: size / 2
                        top: menuButton.top
                        topMargin: size / 2
                    }
                }
            }

            ToolButton {
                id: fileButton

                Layout.alignment: Qt.AlignLeft
                text: qsTr("File")
                onClicked: fileMenu.open()

                Menu {
                    id: fileMenu

                    y: fileButton.height

                    MenuItem {
                        text: qsTr("Import from a file")
                        icon.name: "document-open-symbolic"
                        enabled: !app.busy && app.state === DsnoteApp.StateIdle
                        onClicked: {
                            fileReadDialog.open()
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Import a note from a text file, subtitle file, video or audio file.") + " " +
                                      qsTr("If the imported file is an audio or video file, the audio is converted to text.")
                        hoverEnabled: true
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: qsTr("Export to a file")
                        icon.name: "document-save-symbolic"
                        enabled: app.note.length !== 0 && !app.busy && app.state === DsnoteApp.StateIdle
                        onClicked: {
                            appWin.openDialog("ExportFilePage.qml", {"translated": false})
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Export the current note to a text file, subtitle file or audio file.")
                        hoverEnabled: true
                    }

                    MenuItem {
                        text: qsTr("Export the translation to a file")
                        icon.name: "document-save-symbolic"
                        enabled: app.translated_text.length !== 0 && _settings.translator_mode && !app.busy && app.state === DsnoteApp.StateIdle
                        onClicked: {
                            appWin.openDialog("ExportFilePage.qml", {"translated": true})
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Export the translated note to a text file, subtitle file or audio file.")
                        hoverEnabled: true
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
                hoverEnabled: true
            }

            ToolButton {
                id: repairTextButton

                Layout.alignment: Qt.AlignLeft
                text: qsTr("Repair text")
                visible: _settings.show_repair_text && !_settings.translator_mode
                onClicked: rapairTextMenu.open()

                Menu {
                    id: rapairTextMenu

                    y: repairTextButton.height

                    MenuItem {
                        text: qsTr("Restore punctuation")
                        enabled: !app.busy && app.state === DsnoteApp.StateIdle &&
                                 app.feature_punctuator && app.ttt_punctuation_configured &&
                                 app.note.length !== 0
                        onClicked: app.restore_punctuation()
                    }

                    MenuItem {
                        text: qsTr("Restore diacritical marks (%1)").arg(qsTr("Arabic"))
                        enabled: !app.busy && app.state === DsnoteApp.StateIdle &&
                                 app.ttt_diacritizer_ar_configured &&
                                 app.note.length !== 0
                        onClicked: app.restore_diacritics_ar()
                    }

                    MenuItem {
                        text: qsTr("Restore diacritical marks (%1)").arg(qsTr("Hebrew"))
                        enabled: !app.busy && app.state === DsnoteApp.StateIdle &&
                                 app.feature_diacritizer_he && app.ttt_diacritizer_he_configured &&
                                 app.note.length !== 0
                        onClicked: app.restore_diacritics_he()
                    }
                }
            }

            ToolButton {
                id: voicesButton

                visible: (app.tts_ref_voice_needed && !_settings.translator_mode) ||
                         (_settings.translator_mode && (app.tts_for_in_mnt_ref_voice_needed || app.tts_for_out_mnt_ref_voice_needed))
                opacity: enabled ? 1.0 : 0.6
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Voice samples")
                onClicked: appWin.openDialog("VoiceMgmtPage.qml")

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

            Item {
                Layout.fillWidth: true
                height: 1
            }

            ToolButton {
                id: notepadButton

                Layout.alignment: Qt.AlignRight
                text: qsTr("Notepad")
                checkable: true
                checked: !_settings.translator_mode
                onClicked: {
                    if (_settings.translator_mode) {
                        _settings.translator_mode = false
                    }
                    translatorButton.checked = _settings.translator_mode
                    notepadButton.checked = !_settings.translator_mode
                }

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Switch to Notepad")
                hoverEnabled: true
            }

            ToolButton {
                id: translatorButton

                Layout.alignment: Qt.AlignRight
                text: qsTr("Translator")
                checkable: true
                checked: _settings.translator_mode
                onClicked: {
                    if (!_settings.translator_mode) {
                        _settings.translator_mode = true
                        _settings.hint_translator = false
                    }
                    translatorButton.checked = _settings.translator_mode
                    notepadButton.checked = !_settings.translator_mode
                }

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Switch to Translator")
                hoverEnabled: true
            }

            Connections {
                target: _settings
                onTranslator_mode_changed: {
                    translatorButton.checked = _settings.translator_mode
                    notepadButton.checked = !_settings.translator_mode
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

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
                        text: qsTr("Open a text file")
                        icon.name: "document-open-symbolic"
                        onClicked: {
                            textFileReadDialog.open()
                        }
                    }

                    MenuItem {
                        text: qsTr("Transcribe a file")
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
                        ToolTip.text: qsTr("Convert audio from an existing audio or video file into text.")
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: qsTr("Save to a text file")
                        icon.name: "document-save-symbolic"
                        enabled: app.note.length !== 0
                        onClicked: {
                            fileWriteDialog.translation = false
                            fileWriteDialog.open()
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Save the current note to a text file.")
                    }

                    MenuItem {
                        text: qsTr("Save the translation to a text file")
                        icon.name: "document-save-symbolic"
                        enabled: app.translated_text.length !== 0 && _settings.translator_mode
                        onClicked: {
                            fileWriteDialog.translation = true
                            fileWriteDialog.open()
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Save the translated note to a text file.")
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: qsTr("Export to audio file")
                        icon.name: "document-save-symbolic"
                        enabled: app.note.length !== 0 && app.tts_configured &&
                                 (app.state === DsnoteApp.StateListeningManual ||
                                  app.state === DsnoteApp.StateListeningAuto ||
                                  app.state === DsnoteApp.StateListeningSingleSentence ||
                                  app.state === DsnoteApp.StateIdle ||
                                  app.state === DsnoteApp.StatePlayingSpeech)
                        onClicked: {
                            appWin.openDialog("FileWritePage.qml", {"translated": false})
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Convert text from the current note into speech and save in an audio file.")
                    }

                    MenuItem {
                        text: qsTr("Export the translation to audio file")
                        icon.name: "document-save-symbolic"
                        enabled: app.translated_text.length !== 0 && _settings.translator_mode &&
                                 app.tts_configured && app.active_tts_model_for_out_mnt.length !== 0 &&
                                 (app.state === DsnoteApp.StateListeningManual ||
                                  app.state === DsnoteApp.StateListeningAuto ||
                                  app.state === DsnoteApp.StateListeningSingleSentence ||
                                  app.state === DsnoteApp.StateIdle ||
                                  app.state === DsnoteApp.StatePlayingSpeech)
                        onClicked: {
                            appWin.openDialog("FileWritePage.qml", {"translated": true})
                        }

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Convert translated text into speech and save in an audio file.")
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
        id: fileWriteDialog

        property bool translation: false

        title: qsTr("Save File")
        selectedNameFilter: {
            switch (_settings.translator_mode ? _settings.mnt_text_format : _settings.stt_tts_text_format) {
            case Settings.TextFormatSubRip: return qsTr("SRT Subtitles") + " (*.srt)"
            case Settings.TextFormatRaw:
            case Settings.TextFormatHtml:
            case Settings.TextFormatMarkdown:
                break;
            }
            return qsTr("Text") + " (*.txt)"
        }
        nameFilters: [ qsTr("Text") + " (*.txt)", qsTr("SRT Subtitles") + " (*.srt)", qsTr("All files") + " (*)"]
        folder: _settings.file_save_dir_url
        selectExisting: false
        selectMultiple: false
        onAccepted: {
            var file_path =
                _settings.file_path_from_url(fileWriteDialog.fileUrl)

            if (translation)
                app.save_note_to_file_translator(file_path)
            else
                app.save_note_to_file(file_path)
        }
    }

    Dialogs.FileDialog {
        id: textFileReadDialog

        title: qsTr("Open File")
        nameFilters: [
            qsTr("All supported files") + " (*.txt *.srt)",
            qsTr("All files") + " (*)"]
        folder: _settings.file_open_dir_url
        selectExisting: true
        selectMultiple: false
        onAccepted: {
            var file_path =
                _settings.file_path_from_url(textFileReadDialog.fileUrl)
            appWin.openTextFile(file_path)
        }
    }

    Dialogs.FileDialog {
        id: fileReadDialog

        title: qsTr("Open File")
        nameFilters: [
            qsTr("All supported files") + " (*.wav *.mp3 *.ogg *.oga *.ogx *.opus *.spx *.flac *.m4a *.aac *.mp4 *.mkv *.ogv *.webm)",
            qsTr("All files") + " (*)"]
        folder: _settings.file_audio_open_dir_url
        selectExisting: true
        selectMultiple: false
        onAccepted: {
            appWin.transcribeFile(fileReadDialog.fileUrl)
        }
    }
}

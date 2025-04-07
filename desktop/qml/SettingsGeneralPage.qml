/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Settings 1.0

ColumnLayout {
    id: root

    property bool verticalMode: parent ? parent.verticalMode : false

    CheckBox {
        checked: _settings.keep_last_note
        text: qsTranslate("SettingsPage", "Remember the last note")
        onCheckedChanged: {
            _settings.keep_last_note = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "The note will be saved automatically, so when you restart the app, your last note will always be available.")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.trans_rules_enabled
        text: qsTranslate("SettingsPage", "Show %1").arg("<i>" + qsTranslate("SettingsPage", "Rules") + "</i>")
        onCheckedChanged: {
            _settings.trans_rules_enabled = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "%1 allows you to create text transformations that can be applied after Speech to Text or before Text to Speech.").arg("<i>" + qsTranslate("SettingsPage", "Rules") + "</i>") + " " +
                      qsTranslate("SettingsPage", "With %1, you can easily and flexibly correct errors in decoded text or correct mispronounced words.").arg("<i>" + qsTranslate("SettingsPage", "Rules") + "</i>") + " " +
                      qsTranslate("SettingsPage", "To configure the rules you need, go to %1 on the main toolbar.").arg("<i>" + qsTranslate("SettingsPage", "Rules") + "</i>")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.show_repair_text
        text: qsTranslate("SettingsPage", "Show %1").arg("<i>" + qsTranslate("SettingsPage", "Repair text") + "</i>")
        onCheckedChanged: {
            _settings.show_repair_text = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Once enabled, a menu with text correction options appears on the main toolbar.")
        hoverEnabled: true
    }

    ComboBoxForm {
        label.text: qsTranslate("SettingsPage", "File import action")
        toolTip: qsTranslate("SettingsPage", "The action when importing a note from a file. You can add imported text to an existing note or replace an existing note.")
        comboBox {
            currentIndex: {
                if (_settings.file_import_action === Settings.FileImportActionAsk) return 0
                if (_settings.file_import_action === Settings.FileImportActionAppend) return 1
                if (_settings.file_import_action === Settings.FileImportActionReplace) return 2
                return 0
            }
            model: [
                qsTranslate("SettingsPage", "Ask whether to add or replace"),
                qsTranslate("SettingsPage", "Add to an existing note"),
                qsTranslate("SettingsPage", "Replace an existing note")
            ]
            onActivated: {
                if (index === 1) {
                    _settings.file_import_action = Settings.FileImportActionAppend
                } else if (index === 2) {
                    _settings.file_import_action = Settings.FileImportActionReplace
                } else {
                    _settings.file_import_action = Settings.FileImportActionAsk
                }
            }
        }
    }

    ComboBoxForm {
        label.text: qsTranslate("SettingsPage", "Text appending mode")
        toolTip: qsTranslate("SettingsPage", "Specifies where to add new text to a note.")
        comboBox {
            currentIndex: {
                if (_settings.insert_mode === Settings.InsertInLine) return 1
                if (_settings.insert_mode === Settings.InsertNewLine) return 2
                if (_settings.insert_mode === Settings.InsertAfterEmptyLine) return 3
                if (_settings.insert_mode === Settings.InsertAtCursor) return 0
                if (_settings.insert_mode === Settings.InsertReplace) return 4
                return 2
            }
            model: [
                qsTranslate("SettingsPage", "Add at the cursor position"),
                qsTranslate("SettingsPage", "Add to last line"),
                qsTranslate("SettingsPage", "Add after line break"),
                qsTranslate("SettingsPage", "Add after empty line"),
                qsTranslate("SettingsPage", "Replace an existing note")
            ]
            onActivated: {
                if (index === 0) {
                    _settings.insert_mode = Settings.InsertAtCursor
                } else if (index === 1) {
                    _settings.insert_mode = Settings.InsertInLine
                } else if (index === 2) {
                    _settings.insert_mode = Settings.InsertNewLine
                } else if (index === 3) {
                    _settings.insert_mode = Settings.InsertAfterEmptyLine
                } else if (index === 4) {
                    _settings.insert_mode = Settings.InsertReplace
                } else {
                    _settings.insert_mode = Settings.InsertNewLine
                }
            }
        }
    }

    Component {
        id: directoryDialog

        Dialogs.FileDialog {
            //id: directoryDialog
            title: qsTranslate("SettingsPage", "Select Directory")
            selectFolder: true
            selectExisting: true
            folder:  _settings.models_dir_url
            onAccepted: {
                _settings.models_dir_url = fileUrl
            }
        }
    }

    Loader {
        id: directoryDialogLoader

        function open() {
            if (item) item.open()
            else sourceComponent = directoryDialog
        }

        onLoaded: item.open()
    }

    CheckBox {
        checked: _settings.cache_policy === Settings.CacheRemove
        text: qsTranslate("SettingsPage", "Clear cache on close")
        onCheckedChanged: {
            _settings.cache_policy = checked ? Settings.CacheRemove : Settings.CacheNoRemove
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "When closing, delete all cached audio files.")
        hoverEnabled: true
    }

    TextFieldForm {
        label.text: qsTranslate("SettingsPage", "Location of language files")
        toolTip: qsTranslate("SettingsPage", "Directory where language files are downloaded to and stored.") +
                 (_settings.is_flatpak() ?
                      (" " + qsTranslate("SettingsPage", "Make sure that the Flatpak application has permissions to access the directory.")) :
                      "")
        textField {
            text: _settings.models_dir
            readOnly: true
        }
        button {
            text: qsTranslate("SettingsPage", "Change")
            onClicked: directoryDialogLoader.open()
        }
    }

    Item {
        Layout.fillHeight: true
    }
}

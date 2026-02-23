/* Copyright (C) 2024-2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts

import org.mkiol.dsnote.Settings 1.0

ColumnLayout {
    id: root

    property bool verticalMode: parent ? parent.verticalMode : false

    TextFieldForm {
        label.text: qsTranslate("SettingsPage", "Font in text editor")
        compact: true
        textField {
            text: {
                var font = _settings.notepad_font
                if (font.family.length == 0 || font.family == "0")
                    font.family = qsTranslate("SettingsPage", "Default")
                return font.family + " " + font.pointSize + "pt"
            }
            readOnly: true
        }
        button {
            text: qsTranslate("SettingsPage", "Change")
            onClicked: fontDialog.open()
        }
    }

    Dialogs.FontDialog {
        id: fontDialog

        title: qsTranslate("SettingsPage", "Please choose a font")
        currentFont: _settings.notepad_font
        onAccepted: {
            _settings.notepad_font = fontDialog.currentFont
        }
    }

    ComboBoxForm {
        label.text: qsTranslate("SettingsPage", "Language")
        comboBox {
            currentIndex: _settings.translate_ui ? 0 : 1
            model: [
                qsTranslate("SettingsPage", "Auto"),
                qsTranslate("SettingsPage", "English")
            ]
            onActivated: {
                _settings.translate_ui = (index === 0 ? true : false)
            }
        }
    }

    ComboBoxForm {
        label.text: qsTranslate("SettingsPage", "Show desktop notification")
        toolTip: qsTranslate("SettingsPage", "Show desktop notification while reading or listening.")
        comboBox {
            currentIndex: {
                switch(_settings.desktop_notification_policy) {
                case Settings.DesktopNotificationNever: return 0
                case Settings.DesktopNotificationWhenInacvtive: return 1
                case Settings.DesktopNotificationAlways: return 2
                }
                return 0
            }
            model: [
                qsTranslate("SettingsPage", "Never"),
                qsTranslate("SettingsPage", "When in background"),
                qsTranslate("SettingsPage", "Always")
            ]
            onActivated: {
                if (index === 0) {
                    _settings.desktop_notification_policy = Settings.DesktopNotificationNever
                } else if (index === 1) {
                    _settings.desktop_notification_policy = Settings.DesktopNotificationWhenInacvtive
                } else if (index === 2) {
                    _settings.desktop_notification_policy = Settings.DesktopNotificationAlways
                }
            }
        }
    }

    CheckBox {
        Layout.leftMargin: appWin.padding
        visible: _settings.desktop_notification_policy !== Settings.DesktopNotificationNever
        checked: _settings.desktop_notification_details
        text: qsTranslate("SettingsPage", "Include recognized or read text in notifications")
        onCheckedChanged: {
            _settings.desktop_notification_details = checked
        }
    }

    CheckBox {
        checked: _settings.use_tray
        text: qsTranslate("SettingsPage", "Use system tray icon")
        onCheckedChanged: {
            _settings.use_tray = checked
        }
    }

    CheckBox {
        Layout.leftMargin: appWin.padding
        visible: _settings.use_tray
        checked: _settings.start_in_tray
        text: qsTranslate("SettingsPage", "Start minimized to the system tray")
        onCheckedChanged: {
            _settings.start_in_tray = checked
        }
    }

    CheckBox {
        checked: !_settings.qt_style_auto
        text: qsTranslate("SettingsPage", "Use custom graphical style")
        onCheckedChanged: {
            _settings.qt_style_auto = !checked
        }
    }

    ComboBoxForm {
        indends: 1
        visible: !_settings.qt_style_auto
        label.text: qsTranslate("SettingsPage", "Graphical style")
        toolTip: qsTranslate("SettingsPage", "Application graphical interface style.") + " " +
                 qsTranslate("SettingsPage", "Change if you observe problems with incorrect colors under a dark theme.")
        comboBox {
            currentIndex: _settings.qt_style_idx
            model: _settings.qt_styles()
            onActivated: (index) => { _settings.qt_style_idx = index }
        }
    }

    Item {
        Layout.fillHeight: true
    }
}

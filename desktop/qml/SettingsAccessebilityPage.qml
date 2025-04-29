/* Copyright (C) 2024-2025 Michal Kosciesza <michal@mkiol.net>
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

    Component.onCompleted: {
        app.update_feature_statuses()
    }

    BusyIndicator {
        visible: app.busy
        running: visible
    }

    CheckBox {
        visible: app.feature_hotkeys
        checked: _settings.hotkeys_enabled
        text: qsTranslate("SettingsPage", "Use global keyboard shortcuts")
        onCheckedChanged: {
            _settings.hotkeys_enabled = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Shortcuts allow you to start or stop listening and reading using keyboard.") + " " +
                      qsTranslate("SettingsPage", "Speech to Text result can be appended to the current note, inserted into any active window (currently in focus) or copied to the clipboard.") + " " +
                      qsTranslate("SettingsPage", "Text to Speech reading can be from current note or from text in the clipboard.") + " " +
                      qsTranslate("SettingsPage", "Keyboard shortcuts function even when the application is not active (e.g. minimized or in the background).")
        hoverEnabled: true
    }

    Button {
        id: gsConfButton

        visible: _settings.hotkeys_enabled && app.feature_hotkeys && (_settings.hotkeys_type === Settings.HotkeysTypeX11 || _settings.is_kde())
        text: qsTranslate("SettingsPage", "Configure global keyboard shortcuts")
        Layout.leftMargin: appWin.padding
        onClicked: {
            switch(_settings.hotkeys_type) {
            case Settings.HotkeysTypeX11:
                appWin.openPopup(hotkeysEditDialog)
                break
            case Settings.HotkeysTypePortal:
                app.open_hotkeys_editor();
                break
            }
        }
    }

    TipMessage {
        color: palette.text
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_hotkeys && !gsConfButton.visible
        text: qsTranslate("SettingsPage", "Global keyboard shortcuts are managed through the %1.").arg("<i>XDG Desktop Portal</i>") + " " +
              qsTranslate("SettingsPage", "Use your desktop environment configuration tool to change key bindings.")
    }

    Component {
        id: hotkeysEditDialog

        HelpDialog {
            title: qsTranslate("SettingsPage", "Global keyboard shortcuts")
            Repeater {
                model: _settings.hotkeys_table
                ShortcutForm {
                    verticalMode: root.verticalMode
                    psize: (appWin.width / 2) - (4 * appWin.padding)
                    visible: _settings.hotkeys_enabled && app.feature_hotkeys
                    label: modelData[1]
                    text: modelData[2]
                    onTextChanged: _settings.set_hotkey(modelData[0], text);
                    onResetClicked: {
                        _settings.reset_hotkey(modelData[0])
                        text = modelData[2]
                    }
                }
            }
        }
    }

    CheckBox {
        checked: _settings.actions_api_enabled
        text: qsTranslate("SettingsPage", "Allow external applications to invoke actions")
        onCheckedChanged: {
            _settings.actions_api_enabled = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Action allow external application to invoke certain operation when %1 is running.")
                      .arg("<i>Speech Note</i>")
        hoverEnabled: true
    }

    TipMessage {
        color: palette.text
        indends: 1
        visible: _settings.actions_api_enabled
        text: "<p>" + qsTranslate("SettingsPage", "Action allows external application to invoke certain operation when %1 is running.").arg("<i>Speech Note</i>") + " " +
              qsTranslate("SettingsPage", "An action can be triggered via DBus call or with command-line option.") + " " +
              qsTranslate("SettingsPage", "The following actions are currently supported:") +
              "</p><ul>" +
              "<li><i>start-listening</i> - " + qsTranslate("SettingsPage", "Starts listening.") + "</li>" +
              "<li><i>start-listening-translate</i> - " + qsTranslate("SettingsPage", "Starts listening and always translate decoded text.") + "</li>" +
              "<li><i>start-listening-active-window</i> - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is inserted into the active window.") + "</li>" +
              "<li><i>start-listening-translate-active-window</i> - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is translated and inserted into the active window.") + "</li>" +
              "<li><i>start-listening-clipboard</i> - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is copied to the clipboard.") + "</li>" +
              "<li><i>start-listening-translate-clipboard</i> - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is translated and copied to the clipboard.") + "</li>" +
              "<li><i>stop-listening</i> - " + qsTranslate("SettingsPage", "Stops listening. The already captured voice is decoded into text.") + "</li>" +
              "<li><i>start-reading</i> - " + qsTranslate("SettingsPage", "Starts reading.") + "</li>" +
              "<li><i>start-reading-clipboard</i> (X11) - " + qsTranslate("SettingsPage", "Starts reading text from the clipboard.") + "</li>" +
              "<li><i>start-reading-text</i> - " + qsTranslate("SettingsPage", "Starts reading text from command-line option --text.") + "</li>" +
              "<li><i>pause-resume-reading</i> - " + qsTranslate("SettingsPage", "Pauses or resumes reading.") + "</li>" +
              "<li><i>cancel</i> - " + qsTranslate("SettingsPage", "Cancels any of the above operations.") + "</li>" +
              "<li><i>switch-to-next-stt-model</i> - " + qsTranslate("SettingsPage", "Switches to next STT model.") + "</li>" +
              "<li><i>switch-to-prev-stt-model</i> - " + qsTranslate("SettingsPage", "Switches to previous STT model.") + "</li>" +
              "<li><i>switch-to-next-tts-model</i> - " + qsTranslate("SettingsPage", "Switches to next TTS model.") + "</li>" +
              "<li><i>switch-to-prev-tts-model</i> - " + qsTranslate("SettingsPage", "Switches to previous TTS model.") + "</li>" +
              "<li><i>set-stt-model</i> - " + qsTranslate("SettingsPage", "Sets STT model.") + "</li>" +
              "<li><i>set-tts-model</i> - " + qsTranslate("SettingsPage", "Sets TTS model.") + "</li>" +
              "</ul>" +
              qsTranslate("SettingsPage", "For example, to trigger %1 action, execute the following command: %2.")
                      .arg("<i>start-listening</i>")
                      .arg(_settings.is_flatpak() ?
                        "<i>flatpak run net.mkiol.SpeechNote --action start-listening</i>" :
                        "<i>dsnote --action start-listening</i>")
    }

    TipMessage {
        color: "red"
        indends: 1
        visible: _settings.actions_api_enabled && !app.feature_fake_keyboard
        text: qsTranslate("SettingsPage", "Unable to connect to %1 daemon.")
                .arg("<i>ydotool</i>") + " " +
              qsTranslate("SettingsPage", "For %1 action to work, %2 daemon must be installed and running.")
                .arg("<i>start-listening-active-window</i>")
                .arg("<i>ydotool</i>") + (_settings.is_flatpak() ? (" " +
              qsTranslate("SettingsPage", "Also make sure that the Flatpak application has permission to access %1 daemon socket file.")
                        .arg("<i>ydotool</i>")) : "")
    }

    CheckBox {
        checked: _settings.use_toggle_for_hotkey
        text: qsTranslate("SettingsPage", "Toggle behavior for actions")
        onCheckedChanged: {
            _settings.use_toggle_for_hotkey = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Start listening/reading shortcuts or actions will also stop listening/reading if they are triggered while listening/reading is active.")
        hoverEnabled: true
    }

    Item {
        Layout.fillHeight: true
    }
}


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
        visible: app.feature_global_shortcuts
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
                      qsTranslate("SettingsPage", "Keyboard shortcuts function even when the application is not active (e.g. minimized or in the background).") + " " +
                      qsTranslate("SettingsPage", "This feature only works under X11.")
        hoverEnabled: true
    }

    CheckBox {
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        checked: _settings.use_toggle_for_hotkey
        text: qsTranslate("SettingsPage", "Toggle behavior")
        onCheckedChanged: {
            _settings.use_toggle_for_hotkey = checked
        }
        Layout.leftMargin: appWin.padding

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Start listening/reading shortcuts will also stop listening/reading if they are triggered while listening/reading is active.")
        hoverEnabled: true
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Start listening")
        text: _settings.hotkey_start_listening
        onTextChanged: _settings.hotkey_start_listening = text
        onResetClicked: {
            _settings.reset_hotkey_start_listening()
            text = _settings.hotkey_start_listening
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Start listening, always translate")
        text: _settings.hotkey_start_listening_translate
        onTextChanged: _settings.hotkey_start_listening_translate = text
        onResetClicked: {
            _settings.reset_hotkey_start_listening_translate()
            text = _settings.hotkey_start_listening_translate
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts && app.feature_text_active_window
        label: qsTranslate("SettingsPage", "Start listening, text to active window")
        text: _settings.hotkey_start_listening_active_window
        onTextChanged: _settings.hotkey_start_listening_active_window = text
        onResetClicked: {
            _settings.reset_hotkey_start_listening_active_window()
            text = _settings.hotkey_start_listening_active_window
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts && app.feature_text_active_window
        label: qsTranslate("SettingsPage", "Start listening, always translate, text to active window")
        text: _settings.hotkey_start_listening_translate_active_window
        onTextChanged: _settings.hotkey_start_listening_translate_active_window = text
        onResetClicked: {
            _settings.reset_hotkey_start_listening_translate_active_window()
            text = _settings.hotkey_start_listening_translate_active_window
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Start listening, text to clipboard")
        text: _settings.hotkey_start_listening_clipboard
        onTextChanged: _settings.hotkey_start_listening_clipboard = text
        onResetClicked: {
            _settings.reset_hotkey_start_listening_clipboard()
            text = _settings.hotkey_start_listening_clipboard
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Start listening, always translate, text to clipboard")
        text: _settings.hotkey_start_listening_translate_clipboard
        onTextChanged: _settings.hotkey_start_listening_translate_clipboard = text
        onResetClicked: {
            _settings.reset_hotkey_start_listening_translate_clipboard()
            text = _settings.hotkey_start_listening_translate_clipboard
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Stop listening")
        text: _settings.hotkey_stop_listening
        onTextChanged: _settings.hotkey_stop_listening = text
        onResetClicked: {
            _settings.reset_hotkey_stop_listening()
            text = _settings.hotkey_stop_listening
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Start reading")
        text: _settings.hotkey_start_reading
        onTextChanged: _settings.hotkey_start_reading = text
        onResetClicked: {
            _settings.reset_hotkey_start_reading()
            text = _settings.hotkey_start_reading
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Start reading text from clipboard")
        text: _settings.hotkey_start_reading_clipboard
        onTextChanged: _settings.hotkey_start_reading_clipboard = text
        onResetClicked: {
            _settings.reset_hotkey_start_reading_clipboard()
            text = _settings.hotkey_start_reading_clipboard
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Pause/Resume reading")
        text: _settings.hotkey_pause_resume_reading
        onTextChanged: _settings.hotkey_pause_resume_reading = text
        onResetClicked: {
            _settings.reset_hotkey_pause_resume_reading()
            text = _settings.hotkey_pause_resume_reading
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Cancel")
        text: _settings.hotkey_cancel
        onTextChanged: _settings.hotkey_cancel = text
        onResetClicked: {
            _settings.reset_hotkey_cancel()
            text = _settings.hotkey_cancel
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Switch to next STT model")
        text: _settings.hotkey_switch_to_next_stt_model
        onTextChanged: _settings.hotkey_switch_to_next_stt_model = text
        onResetClicked: {
            _settings.reset_hotkey_switch_to_next_stt_model()
            text = _settings.hotkey_switch_to_next_stt_model
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Switch to previous STT model")
        text: _settings.hotkey_switch_to_prev_stt_model
        onTextChanged: _settings.hotkey_switch_to_prev_stt_model = text
        onResetClicked: {
            _settings.reset_hotkey_switch_to_prev_stt_model()
            text = _settings.hotkey_switch_to_prev_stt_model
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Switch to next TTS model")
        text: _settings.hotkey_switch_to_next_tts_model
        onTextChanged: _settings.hotkey_switch_to_next_tts_model = text
        onResetClicked: {
            _settings.reset_hotkey_switch_to_next_tts_model()
            text = _settings.hotkey_switch_to_next_tts_model
        }
    }

    ShortcutForm {
        indends: 1
        visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
        label: qsTranslate("SettingsPage", "Switch to previous TTS model")
        text: _settings.hotkey_switch_to_prev_tts_model
        onTextChanged: _settings.hotkey_switch_to_prev_tts_model = text
        onResetClicked: {
            _settings.reset_hotkey_switch_to_prev_tts_model()
            text = _settings.hotkey_switch_to_prev_tts_model
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
              "<li><i>start-listening-active-window</i> (X11) - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is inserted into the active window.") + "</li>" +
              "<li><i>start-listening-translate-active-window</i> (X11) - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is translated and inserted into the active window.") + "</li>" +
              "<li><i>start-listening-clipboard</i> - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is copied to the clipboard.") + "</li>" +
              "<li><i>start-listening-translate-clipboard</i> - " + qsTranslate("SettingsPage", "Starts listening. The decoded text is translated and copied to the clipboard.") + "</li>" +
              "<li><i>stop-listening</i> - " + qsTranslate("SettingsPage", "Stops listening. The already captured voice is decoded into text.") + "</li>" +
              "<li><i>start-reading</i> - " + qsTranslate("SettingsPage", "Starts reading.") + "</li>" +
              "<li><i>start-reading-clipboard</i> (X11) - " + qsTranslate("SettingsPage", "Starts reading text from the clipboard.") + "</li>" +
              "<li><i>start-reading-text</i> (X11) - " + qsTranslate("SettingsPage", "Starts reading text from command-line option --text.") + "</li>" +
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

    Item {
        Layout.fillHeight: true
    }
}


/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ColumnLayout {
    id: root

    property alias noteTextArea: _noteTextArea
    property bool readOnly: false
    readonly property string placeholderText: qsTr("Neither Speech to Text nor Text to Speech model has been set up yet.") + " " +
                                              qsTr("Go to the %1 to download models for the languages you intend to use.")
                                                .arg("<i>" + qsTr("Languages and Models") + "</i>")

    Connections {
        target: app

        function onAvailable_stt_models_changed() { root.update() }
        function onAvailable_tts_models_changed() { root.update() }
        function onBusyChanged() { root.update() }
        function onStt_configuredChanged() { root.update() }
        function onTts_configuredChanged() { root.update() }
        function onActive_stt_model_changed() { root.update() }
        function onActive_tts_model_changed() { root.update() }
        function onNote_changed() { update() }
        function onLast_cursor_position_changed() {
            if (app.last_cursor_position === root.noteTextArea.textArea.cursorPosition) return
            root.noteTextArea.textArea.cursorPosition = app.last_cursor_position
        }
    }

    function update() {
        if (noteTextArea.textArea.text !== app.note) {
            noteTextArea.textArea.text = app.note
            noteTextArea.textArea.cursorPosition = noteTextArea.textArea.text.length
            noteTextArea.scrollToBottom()
        }

        if (!root.enabled || app.busy || service.busy) return;

        if (app.stt_configured) {
            listenReadCombos.first.combo.currentIndex = app.active_stt_model_idx
            listenReadCombos.first.check.checked = _settings.whisper_translate
        }

        if (app.tts_configured) {
            listenReadCombos.second.combo.currentIndex = app.active_tts_model_idx
        }
    }

    visible: opacity > 0.0
    opacity: enabled ? 1.0 : 0.0
    Behavior on opacity { OpacityAnimator { duration: 100 } }

    ScrollTextArea {
        id: _noteTextArea

        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.leftMargin: appWin.padding
        Layout.rightMargin: appWin.padding

        name: qsTr("Notepad")
        enabled: root.enabled
        canUndoFallback: app.can_undo_note
        canReadSelected: listenReadCombos.second.button.enabled
        canReadAll: canReadSelected
        showControlTags: canReadSelected
        showInsertIndicator: _settings.insert_mode === Settings.InsertAtCursor
        textArea {
            placeholderText: app.stt_configured || app.tts_configured ?
                                 qsTr("Type here or press %1 to make a note...")
                                    .arg("<i>" + qsTr("Listen") + "</i>") : ""
            readOnly: root.readOnly
            onTextChanged: app.note = root.noteTextArea.textArea.text
            onCursorPositionChanged: app.last_cursor_position = root.noteTextArea.textArea.cursorPosition
        }
        textFormatInvalid: {
            if (root.noteTextArea.textArea.text.length == 0) return false
            if (app.auto_text_format === DsnoteApp.AutoTextFormatSubRip)
                return _settings.stt_tts_text_format !== Settings.TextFormatSubRip
            else
                return _settings.stt_tts_text_format === Settings.TextFormatSubRip
        }
        textFormatCombo {
            currentIndex: {
                if (_settings.stt_tts_text_format === Settings.TextFormatRaw) return 0
                if (_settings.stt_tts_text_format === Settings.TextFormatSubRip) return 1
                if (_settings.stt_tts_text_format === Settings.TextFormatInlineTimestamp) return 2
                return 0
            }
            model: [
                qsTr("Plain text"),
                qsTr("SRT Subtitles"),
                qsTr("Inline timestamps")
            ]
            onActivated: {
                if (index === 0)
                    _settings.stt_tts_text_format = Settings.TextFormatRaw
                else if (index === 1)
                    _settings.stt_tts_text_format = Settings.TextFormatSubRip
                else if (index === 2)
                    _settings.stt_tts_text_format = Settings.TextFormatInlineTimestamp
            }
        }

        onCopyClicked: app.copy_to_clipboard()
        onClearClicked: {
            app.make_undo()
            root.noteTextArea.textArea.text = ""
        }
        onUndoFallbackClicked: app.undo_or_redu_note()
        onReadSelectedClicked: {
            app.play_speech_selected(start, end)
        }
    }

    DuoComboButton {
        id: listenReadCombos

        readonly property bool refVoiceNeeded: app.tts_ref_voice_needed && app.available_tts_ref_voice_names.length !== 0
        readonly property bool refPromptNeeded: app.tts_ref_prompt_needed && _settings.tts_voice_prompts.length !== 0

        Layout.fillWidth: true
        verticalMode: width < appWin.height * (app.tts_ref_voice_needed ? 1.4 : 1.0)
        first {
            icon.name: "audio-input-microphone-symbolic"
            enabled: app.stt_configured && (app.state === DsnoteApp.StateIdle ||
                                            app.state === DsnoteApp.StateListeningManual)
            comboToolTip: qsTr("Speech to Text model") + (app.stt_auto_lang_name.length > 0 ? (" | " + app.stt_auto_lang_name) : "")
            comboPlaceholderText: qsTr("No Speech to Text model")
            checkToolTip: qsTr("Translate to English")
            combo {
                model: app.available_stt_models
                onActivated: app.set_active_stt_model_idx(index)
                currentIndex: app.active_stt_model_idx
            }
            check {
                visible: app.stt_translate_needed
                checked: _settings.whisper_translate
                onClicked: {
                    _settings.whisper_translate = !_settings.whisper_translate
                }
            }
            buttonToolTip: qsTr("Listen") + " (Ctrl+Alt+Shift+L)"
            button {
                onClicked: {
                    if (_settings.speech_mode !== Settings.SpeechManual &&
                            app.state === DsnoteApp.StateIdle &&
                            app.task_state === DsnoteApp.TaskStateIdle)
                        app.listen()
                }
                onReleased: {
                    if (app.state === DsnoteApp.StateListeningManual || _settings.speech_mode === Settings.SpeechManual)
                        app.stop_listen()
                }
                onPressed: {
                    if (_settings.speech_mode === Settings.SpeechManual &&
                            app.state === DsnoteApp.StateIdle &&
                            app.task_state === DsnoteApp.TaskStateIdle) {
                        app.listen()
                    }
                }

                action: Action {
                    text: qsTr("Listen")
                    shortcut: "Ctrl+Alt+Shift+L"
                }
            }
        }

        second {
            icon.name: "audio-speakers-symbolic"
            enabled: app.tts_configured && app.state === DsnoteApp.StateIdle
            comboToolTip: listenReadCombos.second.comboRedBorder ?
                              qsTr("This model requires a voice profile.") + " " +
                              qsTr("Create one in %1.").arg("<i>" + qsTr("Voice profiles") + "</i>") : qsTr("Text to Speech model")
            combo2ToolTip: qsTr("Voice profile")
            combo3ToolTip: qsTr("Speech speed")
            comboPlaceholderText: qsTr("No Text to Speech model")
            combo2PlaceholderText: qsTr("No voice profile")
            comboRedBorder: (app.tts_ref_voice_needed && app.available_tts_ref_voice_names.length === 0) ||
                            (app.tts_ref_prompt_needed && _settings.tts_voice_prompts.length === 0)
            showSeparator: !listenReadCombos.verticalMode
            combo {
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle
                model: app.available_tts_models
                onActivated: app.set_active_tts_model_idx(index)
                currentIndex: app.active_tts_model_idx
            }
            combo2 {
                visible: listenReadCombos.refVoiceNeeded || listenReadCombos.refPromptNeeded
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle
                model: listenReadCombos.refVoiceNeeded ? app.available_tts_ref_voice_names :
                                                         _settings.tts_voice_prompt_names
                onActivated: {
                    if (listenReadCombos.refVoiceNeeded)
                        app.set_active_tts_ref_voice_idx(index)
                    else
                        _settings.tts_active_voice_prompt_idx = index
                }
                currentIndex: listenReadCombos.refVoiceNeeded ?
                                  app.active_tts_ref_voice_idx :
                                  _settings.tts_active_voice_prompt_idx
            }
            combo3 {
                visible: true
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle &&
                         (_settings.stt_tts_text_format !== Settings.TextFormatSubRip ||
                          _settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOff ||
                          _settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnDontFit)
                textRole: "text"
                valueRole: "value"
                currentIndex: _settings.speech_speed - 1;
                model: ListModel {
                    ListElement { text: "x 0.1"; value: 1 }
                    ListElement { text: "x 0.2"; value: 2 }
                    ListElement { text: "x 0.3"; value: 3 }
                    ListElement { text: "x 0.4"; value: 4 }
                    ListElement { text: "x 0.5"; value: 5 }
                    ListElement { text: "x 0.6"; value: 6 }
                    ListElement { text: "x 0.7"; value: 7 }
                    ListElement { text: "x 0.8"; value: 8 }
                    ListElement { text: "x 0.9"; value: 9 }
                    ListElement { text: "x 1.0"; value: 10 }
                    ListElement { text: "x 1.1"; value: 11 }
                    ListElement { text: "x 1.2"; value: 12 }
                    ListElement { text: "x 1.3"; value: 13 }
                    ListElement { text: "x 1.4"; value: 14 }
                    ListElement { text: "x 1.5"; value: 15 }
                    ListElement { text: "x 1.6"; value: 16 }
                    ListElement { text: "x 1.7"; value: 17 }
                    ListElement { text: "x 1.8"; value: 18 }
                    ListElement { text: "x 1.9"; value: 19 }
                    ListElement { text: "x 2.0"; value: 20 }
                }
                onActivated: _settings.speech_speed = index + 1
            }
            buttonToolTip: qsTr("Read") + " (Ctrl+Alt+Shift+R)"
            button {
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.note.length !== 0 &&
                         (!app.tts_ref_voice_needed || app.available_tts_ref_voice_names.length !== 0) &&
                         (!app.tts_ref_prompt_needed || _settings.tts_voice_prompts.length !== 0)
                action: Action {
                    text: qsTr("Read")
                    shortcut: "Ctrl+Alt+Shift+R"
                    onTriggered: app.play_speech()
                }
            }
        }
    }
}

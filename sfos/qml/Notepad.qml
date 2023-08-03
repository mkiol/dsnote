/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Settings 1.0
import harbour.dsnote.Dsnote 1.0

Column {
    id: root

    property bool verticalMode: true
    property double maxHeight: parent.height
    property alias noteTextArea: _noteTextArea
    property bool readOnly: false
    readonly property double textAreaHeight: root.maxHeight - listenReadCombos.itemHeight

    width: parent.width

    Connections {
        target: app

        onAvailable_stt_models_changed: root.update()
        onAvailable_tts_models_changed: root.update()
        onBusyChanged: root.update()
        onStt_configuredChanged: root.update()
        onTts_configuredChanged: root.update()
        onActive_stt_model_changed: root.update()
        onActive_tts_model_changed: root.update()
        onNote_changed: update()
    }

    function update() {
        if (!root.enabled || app.busy || service.busy) return;

        if (app.stt_configured) {
            listenReadCombos.first.combo.currentIndex = app.active_stt_model_idx
        }
        if (app.tts_configured) {
            listenReadCombos.second.combo.currentIndex = app.active_tts_model_idx
        }

        if (noteTextArea.textArea.text !== app.note) {
            noteTextArea.textArea.text = app.note
            noteTextArea.scrollToBottom()
        }
    }

    ScrollTextArea {
        id: _noteTextArea

        enabled: !root.readOnly
        width: parent.width
        height: root.textAreaHeight
        canUndo: app.can_undo_note
        canRedo: app.can_redo_note
        canClear: true
        placeholderLabel: app.stt_configured || app.tts_configured ?
                             qsTr("Type here or press %1 to make a note...")
                                .arg("<i>" + qsTr("Listen") + "</i>") : ""
        textArea {
            onTextChanged: {
                app.note = root.noteTextArea.textArea.text
            }
        }
        onClearClicked: {
            app.make_undo()
            root.noteTextArea.textArea.text = ""
        }
        onCopyClicked: app.copy_to_clipboard()
        onUndoClicked: app.undo_or_redu_note()

        PlaceholderLabel {
            enabled: !app.busy && !service.busy && app.connected &&
                     app.note.length === 0 && !app.stt_configured && !app.tts_configured &&
                     !_noteTextArea.textArea.focus
            text: qsTr("Neither Speech to Text nor Text to Speech model has been set up yet.") + " " +
                  qsTr("Go to the %1 to download models for the languages you intend to use.")
                    .arg("<i>" + qsTr("Languages") + "</i>")
        }
    }

    DuoComboButton {
        id: listenReadCombos

        visible: !root.noteTextArea.textArea.focus
        verticalMode: root.verticalMode
        width: parent.width
        first {
            enabled: app.stt_configured && (app.state === DsnoteApp.StateIdle ||
                                            app.state === DsnoteApp.StateListeningManual)
            comboModel: app.available_stt_models
            comboPlaceholderText: qsTr("No Speech to Text model")
            combo {
                label: qsTr("Speech to Text")
                enabled: listenReadCombos.first.enabled &&
                         !listenReadCombos.first.off &&
                         app.state === DsnoteApp.StateIdle
                currentIndex: app.active_stt_model_idx
                onCurrentIndexChanged: {
                    app.set_active_stt_model_idx(
                                listenReadCombos.first.combo.currentIndex)
                }
            }
            button {
                text: qsTr("Listen")
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
            }
        }
        second {
            enabled: app.tts_configured && app.state === DsnoteApp.StateIdle
            comboModel: app.available_tts_models
            comboPlaceholderText: qsTr("No Text to Speech model")
            combo {
                label: qsTr("Text to Speech")
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle
                currentIndex: app.active_stt_model_idx
                onCurrentIndexChanged: {
                    app.set_active_tts_model_idx(
                                listenReadCombos.second.combo.currentIndex)
                }
            }
            button {
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.note.length !== 0
                text: qsTr("Read")
                onClicked: {
                    app.play_speech()
                }
            }
        }
    }
}

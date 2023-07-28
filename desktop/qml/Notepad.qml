/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ColumnLayout {
    id: root

    property alias noteTextArea: _noteTextArea
    property bool readOnly: false

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

    visible: opacity > 0.0
    opacity: enabled ? 1.0 : 0.0
    Behavior on opacity { OpacityAnimator { duration: 100 } }

    Frame {
        Layout.fillHeight: true
        Layout.fillWidth: true
        background: Item {}
        bottomPadding: 0

        ScrollTextArea {
            id: _noteTextArea

            anchors.fill: parent
            enabled: root.enabled
            canUndoFallback: app.can_undo_note
            textArea {
                placeholderText: app.stt_configured || app.tts_configured ?
                                     qsTr("Type here or press %1 to make a note...")
                                        .arg("<i>" + qsTr("Listen") + "</i>") : ""
                readOnly: root.readOnly
                onTextChanged: {
                    app.note = root.noteTextArea.textArea.text
                }
            }
            onCopyClicked: app.copy_to_clipboard()
            onClearClicked: {
                app.make_undo()
                root.noteTextArea.textArea.text = ""
            }
            onUndoFallbackClicked: app.undo_or_redu_note()
        }

        PlaceholderLabel {
            enabled: app.note.length === 0 && !app.stt_configured && !app.tts_configured
            text: qsTr("Neither Speech to Text nor Text to Speech model has been set up yet.") + " " +
                  qsTr("Go to the %1 to download models for the languages you intend to use.")
                    .arg("<i>" + qsTr("Languages") + "</i>")
        }
    }

    DuoComboButton {
        id: listenReadCombos

        Layout.fillWidth: true
        verticalMode: width < appWin.verticalWidthThreshold
        first {
            icon.name: "audio-input-microphone-symbolic"
            enabled: app.stt_configured && (app.state === DsnoteApp.StateIdle ||
                                            app.state === DsnoteApp.StateListeningManual)
            comboToolTip: qsTr("Speech to Text model")
            comboPlaceholderText: qsTr("No Speech to Text model")
            combo {
                model: app.available_stt_models
                onActivated: app.set_active_stt_model_idx(index)
                currentIndex: app.active_stt_model_idx
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
            icon.name: "audio-speakers-symbolic"
            enabled: app.tts_configured && app.state === DsnoteApp.StateIdle
            comboToolTip: qsTr("Text to Speech model")
            comboPlaceholderText: qsTr("No Text to Speech model")
            combo {
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle
                model: app.available_tts_models
                onActivated: app.set_active_tts_model_idx(index)
                currentIndex: app.active_tts_model_idx
            }
            button {
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.note.length !== 0
                text: qsTr("Read")
                onClicked: app.play_speech()
            }
        }
    }
}

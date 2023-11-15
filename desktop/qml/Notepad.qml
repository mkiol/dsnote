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
    readonly property string placeholderText: qsTr("Neither Speech to Text nor Text to Speech model has been set up yet.") + " " +
                                              qsTr("Go to the %1 to download models for the languages you intend to use.")
                                                .arg("<i>" + qsTr("Languages") + "</i>")

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
        leftPadding: appWin.padding
        rightPadding: appWin.padding
        topPadding: appWin.padding

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
            comboToolTip: app.tts_ref_voice_needed && app.available_tts_ref_voices.length === 0 ?
                              qsTr("This model requires a voice sample.") + " " +
                              qsTr("Create one in %1 menu").arg("<i>" + qsTr("Voice samples") + "</i>") : qsTr("Text to Speech model")
            combo2ToolTip: qsTr("Voice sample")
            combo3ToolTip: qsTr("Speech speed")
            comboPlaceholderText: qsTr("No Text to Speech model")
            combo2PlaceholderText: qsTr("No voice sample")
            combo {
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle
                model: app.available_tts_models
                onActivated: app.set_active_tts_model_idx(index)
                currentIndex: app.active_tts_model_idx
                palette.buttonText: app.tts_ref_voice_needed && app.available_tts_ref_voices.length === 0 ? "red" : palette.buttonText
            }
            combo2 {
                visible: app.tts_ref_voice_needed && app.available_tts_ref_voices.length !== 0
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle
                model: app.available_tts_ref_voices
                onActivated: app.set_active_tts_ref_voice_idx(index)
                currentIndex: app.active_tts_ref_voice_idx
            }
            combo3 {
                visible: true
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.state === DsnoteApp.StateIdle
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

            button {
                enabled: listenReadCombos.second.enabled &&
                         !listenReadCombos.second.off &&
                         app.note.length !== 0 &&
                         (!app.tts_ref_voice_needed || app.available_tts_ref_voices.length !== 0)
                text: qsTr("Read")
                onClicked: app.play_speech()
            }
        }
    }
}

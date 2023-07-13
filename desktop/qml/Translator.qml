/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ColumnLayout {
    id: root

    property alias noteTextArea: _noteTextArea
    property alias translatedNoteTextArea: _translatedNoteTextArea
    property bool readOnly: false
    readonly property bool canCancelMnt: app.state === DsnoteApp.StateTranslating
    readonly property int _mntComboSize: Math.max(mntInCombo.first.combo.implicitWidth,
                                                  mntOutCombo.first.combo.implicitWidth)

    Connections {
        target: app

        onAvailable_mnt_langs_changed: root.update()
        onAvailable_mnt_out_langs_changed: root.update()
        onAvailable_tts_models_for_in_mnt_changed: root.update()
        onAvailable_tts_models_for_out_mnt_changed: root.update()
        onBusyChanged: root.update()
        onActive_mnt_lang_changed: root.update()
        onActive_mnt_out_lang_changed: root.update()
        onActive_tts_model_for_in_mnt_changed: root.update()
        onActive_tts_model_for_out_mnt_changed: update()
        onNote_changed: update()
        onTranslated_text_changed: update()
    }

    function update() {
        if (!root.enabled || app.busy || service.busy) return;

        if (app.mnt_configured) {
            mntInCombo.first.combo.currentIndex = app.active_mnt_lang_idx
            mntOutCombo.first.combo.currentIndex = app.active_mnt_out_lang_idx
        }
        if (app.mnt_configured && app.tts_configured) {
            mntInCombo.second.combo.currentIndex = app.active_tts_model_for_in_mnt_idx
            mntOutCombo.second.combo.currentIndex = app.active_tts_model_for_out_mnt_idx
        }

        if (noteTextArea.textArea.text !== app.note) {
            noteTextArea.textArea.text = app.note
            noteTextArea.scrollToBottom()
        }
        if (translatedNoteTextArea.textArea.text !== app.translated_text) {
            translatedNoteTextArea.textArea.text = app.translated_text
            translatedNoteTextArea.scrollToBottom()
        }
    }

    GridLayout {
        id: grid

        property bool verticalMode: width < appWin.verticalWidthThreshold

        columns: verticalMode ? 1 : 2
        Layout.fillHeight: true
        Layout.fillWidth: true

        ColumnLayout {
            id: mntInColumn

            Layout.fillHeight: true
            Layout.fillWidth: true

            Frame {
                Layout.fillHeight: true
                Layout.fillWidth: true
                background: Item {}
                bottomPadding: 0
                rightPadding: grid.verticalMode ? horizontalPadding : 0

                ScrollTextArea {
                    id: _noteTextArea

                    anchors.fill: parent
                    enabled: root.enabled
                    canUndoFallback: app.can_undo_note
                    textArea {
                        placeholderText: qsTr("Type here text to translate from...")
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
                id: mntInCombo

                Layout.fillWidth: true
                verticalMode: width < appWin.verticalWidthThreshold
                first {
                    enabled: app.mnt_configured && app.state === DsnoteApp.StateIdle
                    comboToolTip: qsTr("Pick the language to translate from.")
                    comboPlaceholderText: qsTr("No Translator model")
                    comboPrefWidth: root._mntComboSize
                    comboFillWidth: false
                    combo {
                        model: app.available_mnt_langs
                        onActivated: app.set_active_mnt_lang_idx(index)
                        currentIndex: app.active_mnt_lang_idx
                    }
                    frame {
                        rightPadding: grid.verticalMode || !mntInCombo.verticalMode ?
                                          mntInCombo.first.frame.horizontalPadding : 0
                    }
                }
                second {
                    icon.name: "audio-speakers-symbolic"
                    enabled: app.mnt_configured && app.tts_configured && app.state === DsnoteApp.StateIdle
                    comboToolTip: qsTr("Text to Speech model for language to translate from...")
                    comboPlaceholderText: qsTr("No Text to Speech model")
                    comboFillWidth: true
                    combo {
                        model: app.available_tts_models_for_in_mnt
                        enabled: mntInCombo.second.enabled &&
                                 !mntInCombo.second.off &&
                                 app.state === DsnoteApp.StateIdle
                        onActivated: app.set_active_tts_model_for_in_mnt_idx(index)
                        currentIndex: app.active_tts_model_for_in_mnt_idx
                    }
                    frame {
                        rightPadding: grid.verticalMode ? mntInCombo.second.frame.horizontalPadding : 0
                    }
                    button {
                        text: qsTr("Read")
                        enabled: mntInCombo.second.enabled &&
                                 !mntInCombo.second.off &&
                                 app.note.length !== 0
                        onClicked: app.play_speech_translator(false)
                    }
                }
            }
        }

        ColumnLayout {
            id: mntOutColumn

            Layout.preferredHeight: mntInColumn.implicitHeight
            Layout.preferredWidth: mntInColumn.implicitWidth

            Frame {
                Layout.fillHeight: true
                Layout.fillWidth: true
                background: Item {}
                bottomPadding: 0
                leftPadding: grid.verticalMode ? horizontalPadding : 0

                ScrollTextArea {
                    id: _translatedNoteTextArea

                    anchors.fill: parent
                    enabled: root.enabled
                    opacity: enabled ? 0.8 : 0.0
                    canClear: false
                    canUndo: false
                    canRedo: false
                    canPaste: false
                    textArea {
                        //placeholderText: qsTr("Translation")
                        readOnly: root.readOnly || app.translated_text.length === 0
                        onTextChanged: {
                            app.translated_text = root.translatedNoteTextArea.textArea.text
                        }
                    }
                    onCopyClicked: app.copy_translation_to_clipboard()
                }
            }

            DuoComboButton {
                id: mntOutCombo

                Layout.fillWidth: true
                verticalMode: width < appWin.verticalWidthThreshold
                first {
                    enabled: app.mnt_configured && app.state === DsnoteApp.StateIdle
                    comboToolTip: qsTr("Pick the language to translate into.")
                    comboPlaceholderText: qsTr("No Translator model")
                    comboPrefWidth: root._mntComboSize
                    comboFillWidth: false
                    combo {
                        model: app.available_mnt_out_langs
                        onActivated: app.set_active_mnt_out_lang_idx(index)
                        currentIndex: app.active_mnt_out_lang_idx
                    }
                    frame {
                        leftPadding: grid.verticalMode ? mntOutCombo.first.frame.horizontalPadding : 0
                    }
                }
                second {
                    icon.name: "audio-speakers-symbolic"
                    enabled: app.mnt_configured && app.tts_configured &&
                             app.state === DsnoteApp.StateIdle
                    comboToolTip: qsTr("Text to Speech model for language to translate into.")
                    comboPlaceholderText: qsTr("No Text to Speech model")
                    comboFillWidth: true
                    combo {
                        enabled: mntOutCombo.second.enabled &&
                                 !mntOutCombo.second.off &&
                                 app.state === DsnoteApp.StateIdle
                        model: app.available_tts_models_for_out_mnt
                        onActivated: app.set_active_tts_model_for_out_mnt_idx(index)
                        currentIndex: app.active_tts_model_for_out_mnt_idx
                    }
                    frame {
                        leftPadding: grid.verticalMode || !mntOutCombo.verticalMode ?
                                         mntOutCombo.second.frame.horizontalPadding : 0
                    }
                    button {
                        text: qsTr("Read")
                        enabled: mntOutCombo.second.enabled &&
                                 !mntOutCombo.second.off &&
                                 app.translated_text.length !== 0 &&
                                 app.state !== DsnoteApp.StateTranslating
                        onClicked: app.play_speech_translator(true)
                    }
                }
            }
        }
    }

    Frame {
        Layout.alignment: Qt.AlignHCenter
        background: Item {}

        RowLayout {
            Button {
                id: translateButton
                enabled: (app.state === DsnoteApp.StateIdle && !_settings.translate_when_typing)
                         || root.canCancelMnt
                text: root.canCancelMnt ? qsTr("Cancel") : qsTr("Translate")
                onClicked: {
                    if (root.canCancelMnt) app.cancel()
                    else app.translate()
                }
            }

            Switch {
                enabled: app.state === DsnoteApp.StateIdle
                text: qsTr("Translate as you type")
                checked: _settings.translate_when_typing
                onClicked: {
                    _settings.translate_when_typing = !_settings.translate_when_typing
                }
            }
        }
    }
}

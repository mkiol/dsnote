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

Item {
    id: root

    property bool verticalMode: true
    property double maxHeight: parent.height
    property alias noteTextArea: _noteTextArea
    property alias translatedNoteTextArea: _translatedNoteTextArea
    property bool readOnly: false
    readonly property bool canCancelMnt: app.state === DsnoteApp.StateTranslating
    readonly property double textAreaHeight: root.verticalMode ?
                                        (root.maxHeight -
                                         mntInCombo.itemHeight - mntOutCombo.itemHeight -
                                         translatorButton.height) / 2 :
                                        root.maxHeight -
                                        Math.max(mntInCombo.itemHeight, mntOutCombo.itemHeight) -
                                        translatorButton.height

    width: parent.width
    height: app.mnt_configured ? column.height : appWin.height

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
        onActive_tts_model_for_out_mnt_changed: root.update()
        onNote_changed: root.update()
        onTranslated_text_changed: root.update()
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

    PlaceholderLabel {
        enabled: !app.mnt_configured
        text: qsTr("Translator model has not been set up yet.") + " " +
              qsTr("Go to the %1 to download models for the languages you intend to use.")
                .arg("<i>" + qsTr("Languages") + "</i>")
    }

    Column {
        id: column

        width: parent.width
        visible: app.mnt_configured

        Grid {
            enabled: root.enabled
            columns: root.verticalMode ? 1 : 2
            width: parent.width

            Column {
                id: mntInColumn

                width: root.verticalMode ? parent.width : parent.width / 2

                ScrollTextArea {
                    id: _noteTextArea

                    enabled: !root.readOnly
                    width: parent.width
                    height: root.textAreaHeight
                    canUndo: app.can_undo_note
                    canRedo: app.can_redo_note
                    canClear: true
                    placeholderLabel: qsTr("Type here text to translate from...")
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
                }

                DuoComboButton {
                    id: mntInCombo

                    visible: !root.noteTextArea.textArea.highlighted && !root.translatedNoteTextArea.textArea.highlighted
                    verticalMode: true
                    width: parent.width
                    first {
                        enabled: app.mnt_configured && app.state === DsnoteApp.StateIdle
                        comboModel: app.available_mnt_langs
                        comboPlaceholderText: qsTr("No Translator model")
                        combo {
                            label: qsTr("Translate from")
                            currentIndex: app.active_mnt_lang_idx
                            onCurrentIndexChanged: {
                                app.set_active_mnt_lang_idx(
                                            mntInCombo.first.combo.currentIndex)
                            }
                        }
                        expandedHeight: root.verticalMode || !mntOutCombo.first.combo.menu ? 0 : mntOutCombo.first.combo.menu.height
                        height: root.verticalMode ? mntInCombo.first.implicitHeight :
                                                    Math.max(mntOutCombo.first.implicitHeight, mntInCombo.first.implicitHeight)
                    }

                    second {
                        enabled: app.mnt_configured && app.tts_configured && app.state === DsnoteApp.StateIdle
                        comboModel: app.available_tts_models_for_in_mnt
                        comboPlaceholderText: qsTr("No Text to Speech model")
                        combo {
                            label: qsTr("Text to Speech")
                            enabled: mntInCombo.second.enabled &&
                                     !mntInCombo.second.off &&
                                     app.state === DsnoteApp.StateIdle
                            currentIndex: app.active_tts_model_for_in_mnt_idx
                            onCurrentIndexChanged: {
                                app.set_active_tts_model_for_in_mnt_idx(
                                            mntInCombo.second.combo.currentIndex)
                            }
                        }
                        expandedHeight: root.verticalMode || !mntOutCombo.second.combo.menu ? 0 : mntOutCombo.second.combo.menu.height
                        height: root.verticalMode ? mntInCombo.second.implicitHeight :
                                                    Math.max(mntOutCombo.second.implicitHeight, mntInCombo.second.implicitHeight)

                        button {
                            enabled: mntInCombo.second.enabled &&
                                     !mntInCombo.second.off &&
                                     app.note.length !== 0
                            text: qsTr("Read")
                            onClicked: {
                                app.play_speech_translator(false)
                            }
                        }
                    }
                }
            }

            Column {
                id: mntOutColumn

                width: root.verticalMode ? parent.width : parent.width / 2

                ScrollTextArea {
                    id: _translatedNoteTextArea

                    enabled: !root.readOnly && app.mnt_configured && app.translated_text.length !== 0
                             && app.state !== DsnoteApp.StateTranslating
                    width: parent.width
                    height: root.textAreaHeight
                    highlighted: true
                    canClear: false
                    textArea {
                        //placeholderText: qsTr("Translation")
                        onTextChanged: {
                            app.translated_text = root.translatedNoteTextArea.textArea.text
                        }
                    }
                    onClearClicked: root.translatedNoteTextArea.textArea.text = ""
                    onCopyClicked: app.copy_translation_to_clipboard()

                    BusyIndicator {
                        anchors.centerIn: parent
                        size: BusyIndicatorSize.Large
                        running: app.state === DsnoteApp.StateTranslating
                    }
                }

                DuoComboButton {
                    id: mntOutCombo

                    visible: !root.noteTextArea.textArea.highlighted && !root.translatedNoteTextArea.textArea.highlighted
                    verticalMode: true
                    width: parent.width

                    first {
                        enabled: app.mnt_configured && app.state === DsnoteApp.StateIdle
                        comboModel: app.available_mnt_out_langs
                        comboPlaceholderText: qsTr("No Translator model")
                        combo {
                            label: qsTr("Translate to")
                            currentIndex: app.active_mnt_out_lang_idx
                            onCurrentIndexChanged: {
                                app.set_active_mnt_out_lang_idx(
                                            mntOutCombo.first.combo.currentIndex)
                            }
                        }
                        expandedHeight: root.verticalMode || !mntInCombo.first.combo.menu ? 0 : mntInCombo.first.combo.menu.height
                        height: root.verticalMode ? mntOutCombo.first.implicitHeight :
                                                    Math.max(mntOutCombo.first.implicitHeight, mntInCombo.first.implicitHeight)
                    }

                    second {
                        enabled: app.mnt_configured && app.tts_configured &&
                                 app.state === DsnoteApp.StateIdle
                        comboModel: app.available_tts_models_for_out_mnt
                        comboPlaceholderText: qsTr("No Text to Speech model")
                        combo {
                            label: qsTr("Text to Speech")
                            enabled: mntOutCombo.second.enabled &&
                                     !mntOutCombo.second.off &&
                                     app.state === DsnoteApp.StateIdle
                            currentIndex: app.active_tts_model_for_out_mnt_idx
                            onCurrentIndexChanged: {
                                app.set_active_tts_model_for_out_mnt_idx(
                                            mntOutCombo.second.combo.currentIndex)
                            }
                        }
                        expandedHeight: root.verticalMode || !mntInCombo.second.combo.menu ? 0 : mntInCombo.second.combo.menu.height
                        height: root.verticalMode ? mntOutCombo.second.implicitHeight :
                                                    Math.max(mntOutCombo.second.implicitHeight, mntInCombo.second.implicitHeight)

                        button {
                            enabled: mntOutCombo.second.enabled &&
                                     !mntOutCombo.second.off &&
                                     app.translated_text.length !== 0 &&
                                     app.state !== DsnoteApp.StateTranslating
                            text: qsTr("Read")
                            onClicked: {
                                app.play_speech_translator(true)
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: translatorButton
            width: parent.width
            height: Math.max(_translatorButton.height, translatorTextSwitch.height) + 2 * Theme.paddingMedium

            Row {
                anchors.centerIn: parent

                Button {
                    id: _translatorButton

                    anchors.verticalCenter: parent.verticalCenter
                    enabled: (app.state === DsnoteApp.StateIdle && !_settings.translate_when_typing) || root.canCancelMnt
                    text: root.canCancelMnt ? qsTr("Cancel") : qsTr("Translate")
                    onClicked: {
                        if (root.canCancelMnt) app.cancel()
                        else app.translate()
                    }
                    preferredWidth: translatorTextSwitch._label.implicitWidth > root.width - (Theme.buttonWidthSmall + 2 * Theme.horizontalPageMargin) ?
                                        Theme.buttonWidthExtraSmall : Theme.buttonWidthSmall
                }

                TextSwitch {
                    id: translatorTextSwitch

                    _label {
                        width: Math.min(root.width - _translatorButton.implicitWidthh - 2 * Theme.horizontalPageMargin, translatorTextSwitch._label.implicitWidth)
                        font.pixelSize: verticalMode ? Theme.fontSizeExtraSmall : Theme.fontSizeSmall
                    }

                    width: Theme.itemSizeExtraSmall + translatorTextSwitch._label.width
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: app.state === DsnoteApp.StateIdle
                    checked: _settings.translate_when_typing
                    automaticCheck: false
                    text: qsTr("Translate as you type")
                    onClicked: {
                        _settings.translate_when_typing = !_settings.translate_when_typing
                    }
                }
            }
        }
    }
}

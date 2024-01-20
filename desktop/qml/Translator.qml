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
    property alias translatedNoteTextArea: _translatedNoteTextArea
    property bool readOnly: false
    readonly property bool canCancelMnt: app.state === DsnoteApp.StateTranslating
    readonly property int _mntComboSize: Math.max(mntInCombo.first.combo.implicitWidth,
                                                  mntOutCombo.first.combo.implicitWidth)
    readonly property string placeholderText: qsTr("Translator model has not been set up yet.") + " " +
                                              qsTr("Go to the %1 to download models for the languages you intend to use.")
                                                .arg("<i>" + qsTr("Languages") + "</i>")
    readonly property alias verticalMode: grid.verticalMode

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
        if (noteTextArea.textArea.text !== app.note) {
            noteTextArea.textArea.text = app.note
            noteTextArea.scrollToBottom()
        }

        if (translatedNoteTextArea.textArea.text !== app.translated_text) {
            translatedNoteTextArea.textArea.text = app.translated_text
            translatedNoteTextArea.scrollToBottom()
        }

        if (!root.enabled || app.busy || service.busy) return;

        if (app.mnt_configured) {
            mntInCombo.first.combo.currentIndex = app.active_mnt_lang_idx
            mntOutCombo.first.combo.currentIndex = app.active_mnt_out_lang_idx
        }
        if (app.mnt_configured && app.tts_configured) {
            mntInCombo.second.combo.currentIndex = app.active_tts_model_for_in_mnt_idx
            mntOutCombo.second.combo.currentIndex = app.active_tts_model_for_out_mnt_idx
        }
    }

    visible: opacity > 0.0
    opacity: enabled ? 1.0 : 0.0
    Behavior on opacity { OpacityAnimator { duration: 100 } }

    GridLayout {
        id: grid

        property bool verticalMode: width < (appWin.verticalWidthThreshold *
                            (app.tts_for_in_mnt_ref_voice_needed || app.tts_for_out_mnt_ref_voice_needed ? 1.4 : 1.0))

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
                leftPadding: appWin.padding
                topPadding: appWin.padding
                rightPadding: grid.verticalMode ? appWin.padding : 0
                enabled: app.mnt_configured

                ScrollTextArea {
                    id: _noteTextArea

                    enabled: !root.readOnly && app.mnt_configured
                    anchors.fill: parent
                    canUndoFallback: app.can_undo_note
                    textArea {
                        placeholderText: app.mnt_configured ? qsTr("Type here text to translate from...") : ""
                        onTextChanged: {
                            app.note = root.noteTextArea.textArea.text
                        }
                    }
                    textFormatInvalid: {
                        if (root.noteTextArea.textArea.text.length == 0) return false
                        if (app.auto_text_format === DsnoteApp.AutoTextFormatSubRip)
                            return _settings.mnt_text_format !== Settings.TextFormatSubRip
                        else
                            return _settings.mnt_text_format === Settings.TextFormatSubRip
                    }
                    textFormatCombo {
                        currentIndex: {
                            if (_settings.mnt_text_format === Settings.TextFormatRaw) return 0
                            if (_settings.mnt_text_format === Settings.TextFormatHtml) return 1
                            if (_settings.mnt_text_format === Settings.TextFormatMarkdown) return 2
                            if (_settings.mnt_text_format === Settings.TextFormatSubRip) return 3
                            return 0
                        }
                        model: [
                            qsTr("Plain text"),
                            "HTML",
                            "Markdown",
                            qsTr("SRT Subtitles")
                        ]
                        onActivated: {
                            if (index === 0)
                                _settings.mnt_text_format = Settings.TextFormatRaw
                            else if (index === 1)
                                _settings.mnt_text_format = Settings.TextFormatHtml
                            else if (index === 2)
                                _settings.mnt_text_format = Settings.TextFormatMarkdown
                            else if (index === 3)
                                _settings.mnt_text_format = Settings.TextFormatSubRip
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
                        leftPadding: appWin.padding
                        rightPadding: grid.verticalMode || !mntInCombo.verticalMode ?
                                          appWin.padding : 0
                    }
                }
                second {
                    icon.name: "audio-speakers-symbolic"
                    enabled: app.mnt_configured && app.tts_configured && app.state === DsnoteApp.StateIdle
                    comboToolTip: app.tts_for_in_mnt_ref_voice_needed && app.available_tts_ref_voices.length === 0 ?
                                      qsTr("This model requires a voice sample.") + " " +
                                      qsTr("Create one in %1.").arg("<i>" + qsTr("Voice samples") + "</i>") :
                                      qsTr("Text to Speech model for language to translate from.")
                    comboPlaceholderText: qsTr("No Text to Speech model")
                    combo2PlaceholderText: qsTr("No voice sample")
                    combo2ToolTip: qsTr("Voice sample")
                    comboFillWidth: true
                    comboRedBorder: !mntInCombo.second.off && app.tts_for_in_mnt_ref_voice_needed && app.available_tts_ref_voices.length === 0
                    showSeparator: !mntInCombo.verticalMode
                    combo {
                        model: app.available_tts_models_for_in_mnt
                        enabled: mntInCombo.second.enabled &&
                                 !mntInCombo.second.off &&
                                 app.state === DsnoteApp.StateIdle
                        onActivated: app.set_active_tts_model_for_in_mnt_idx(index)
                        currentIndex: app.active_tts_model_for_in_mnt_idx
                    }
                    combo2 {
                        visible: app.tts_for_in_mnt_ref_voice_needed && app.available_tts_ref_voices.length !== 0
                        enabled: mntInCombo.second.enabled &&
                                 !mntInCombo.second.off &&
                                 app.state === DsnoteApp.StateIdle
                        model: app.available_tts_ref_voices
                        onActivated: app.set_active_tts_for_in_mnt_ref_voice_idx(index)
                        currentIndex: app.active_tts_for_in_mnt_ref_voice_idx
                    }
                    frame {
                        leftPadding: appWin.padding
                        rightPadding: grid.verticalMode ? appWin.padding : 0
                    }
                    button {
                        text: qsTr("Read")
                        enabled: mntInCombo.second.enabled &&
                                 !mntInCombo.second.off &&
                                 app.note.length !== 0 &&
                                 (!app.tts_for_in_mnt_ref_voice_needed || app.available_tts_ref_voices.length !== 0)
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
                rightPadding: appWin.padding
                topPadding: appWin.padding
                leftPadding: grid.verticalMode ? appWin.padding : 0

                ScrollTextArea {
                    id: _translatedNoteTextArea

                    enabled: !root.readOnly && app.mnt_configured && app.translated_text.length !== 0
                    anchors.fill: parent
                    textColor: {
                        var c = palette.text
                        return Qt.rgba(c.r, c.g, c.b, 0.8)
                    }
                    canClear: false
                    canUndo: false
                    canRedo: false
                    canPaste: false
                    textArea {
                        onTextChanged: {
                            app.translated_text = root.translatedNoteTextArea.textArea.text
                        }
                    }
                    onCopyClicked: app.copy_translation_to_clipboard()
                }

                PlaceholderLabel {
                    enabled: !app.mnt_configured
                    text: root.placeholderText
                    color: _translatedNoteTextArea.textArea.color
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
                        rightPadding: appWin.padding
                        leftPadding: grid.verticalMode ? appWin.padding : 0
                    }
                }
                second {
                    icon.name: "audio-speakers-symbolic"
                    enabled: app.mnt_configured && app.tts_configured &&
                             app.state === DsnoteApp.StateIdle
                    comboToolTip: app.tts_for_out_mnt_ref_voice_needed && app.available_tts_ref_voices.length === 0 ?
                                      qsTr("This model requires a voice sample.") + " " +
                                      qsTr("Create one in %1 menu").arg("<i>" + qsTr("Voice samples") + "</i>") :
                                      qsTr("Text to Speech model for language to translate into.")
                    comboPlaceholderText: qsTr("No Text to Speech model")
                    combo2PlaceholderText: qsTr("No voice sample")
                    combo2ToolTip: qsTr("Voice sample")
                    comboFillWidth: true
                    comboRedBorder: !mntOutCombo.second.off && app.tts_for_out_mnt_ref_voice_needed && app.available_tts_ref_voices.length === 0
                    showSeparator: !mntOutCombo.verticalMode
                    combo {
                        enabled: mntOutCombo.second.enabled &&
                                 !mntOutCombo.second.off &&
                                 app.state === DsnoteApp.StateIdle
                        model: app.available_tts_models_for_out_mnt
                        onActivated: app.set_active_tts_model_for_out_mnt_idx(index)
                        currentIndex: app.active_tts_model_for_out_mnt_idx
                    }
                    combo2 {
                        visible: app.tts_for_out_mnt_ref_voice_needed && app.available_tts_ref_voices.length !== 0
                        enabled: mntInCombo.second.enabled &&
                                 !mntInCombo.second.off &&
                                 app.state === DsnoteApp.StateIdle
                        model: app.available_tts_ref_voices
                        onActivated: app.set_active_tts_for_out_mnt_ref_voice_idx(index)
                        currentIndex: app.active_tts_for_out_mnt_ref_voice_idx
                    }
                    frame {
                        rightPadding: appWin.padding
                        leftPadding: grid.verticalMode || !mntOutCombo.verticalMode ?
                                         appWin.padding : 0
                    }
                    button {
                        text: qsTr("Read")
                        enabled: mntOutCombo.second.enabled &&
                                 !mntOutCombo.second.off &&
                                 app.translated_text.length !== 0 &&
                                 app.state !== DsnoteApp.StateTranslating &&
                                 (!app.tts_for_out_mnt_ref_voice_needed || app.available_tts_ref_voices.length !== 0)
                        onClicked: app.play_speech_translator(true)
                    }
                }
            }
        }
    }

    Frame {
        visible: app.mnt_configured
        Layout.alignment: Qt.AlignHCenter
        background: Item {}
        bottomPadding: 0
        topPadding: 0
        rightPadding: appWin.padding
        leftPadding: appWin.padding

        GridLayout {
            columns: root.verticalMode ? (mntOutCombo.verticalMode ? 1 : 2) : 2
            columnSpacing: appWin.padding
            rowSpacing: appWin.padding

            RowLayout {
                Layout.alignment: Qt.AlignCenter

                Button {
                    id: translateButton
                    enabled: app.state === DsnoteApp.StateIdle && !_settings.translate_when_typing
                    text: qsTr("Translate")
                    onClicked: {
                        app.translate()
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

                ToolSeparator {
                    visible: !root.verticalMode
                }
            }

            Switch {
                Layout.alignment: Qt.AlignCenter
                enabled: app.state === DsnoteApp.StateIdle &&
                         _settings.mnt_text_format !== Settings.TextFormatSubRip
                text: qsTr("Clean up the text")
                checked: _settings.mnt_clean_text
                onClicked: {
                    _settings.mnt_clean_text = !_settings.mnt_clean_text
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Remove duplicate whitespaces and extra line breaks in the text before translation.") + " " +
                              qsTr("If the input text is incorrectly formatted, this option may improve the translation quality.")
                hoverEnabled: true
            }
        }
    }
}

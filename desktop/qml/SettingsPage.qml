/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
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

DialogPage {
    id: root

    property bool verticalMode: bar.implicitWidth > (root.width - 2 * appWin.padding)
    property var features: app.features_availability()

    title: qsTr("Settings")

    footerLabel {
        visible: _settings.restart_required
        text: qsTr("Restart the application to apply changes.")
        color: "red"
    }

    Connections {
        target: app
        onFeatures_availability_updated: {
            root.features = app.features_availability()
        }
    }

    TabBar {
        id: bar

        visible: !root.verticalMode
        Layout.fillWidth: true
        onCurrentIndexChanged: comboBar.currentIndex = currentIndex

        TabButton {
            text: qsTr("User Interface")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Speech to Text")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Text to Speech")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Accessibility")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Other")
            width: implicitWidth
        }
    }

    ColumnLayout {
        visible: root.verticalMode
        Layout.fillWidth: true

        ComboBox {
            id: comboBar

            Layout.fillWidth: true
            onCurrentIndexChanged: bar.currentIndex = currentIndex

            model: [
                qsTr("User Interface"),
                qsTr("Speech to Text"),
                qsTr("Text to Speech"),
                qsTr("Accessibility"),
                qsTr("Other")
            ]
        }

        HorizontalLine{}
    }

    StackLayout {
        property alias verticalMode: root.verticalMode

        Layout.fillWidth: true
        Layout.topMargin: appWin.padding
        currentIndex: bar.currentIndex

        ColumnLayout {
            id: userInterfaceTab

            CheckBox {
                checked: _settings.keep_last_note
                text: qsTr("Remember the last note")
                onCheckedChanged: {
                    _settings.keep_last_note = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("The note will be saved automatically, so when you restart the app, your last note will always be available.")
                hoverEnabled: true
            }

            ComboBoxForm {
                label.text: qsTr("File import action")
                toolTip: qsTr("The action when importing a note from a file. You can add imported text to an existing note or replace an existing note.")
                comboBox {
                    currentIndex: {
                        if (_settings.file_import_action === Settings.FileImportActionAsk) return 0
                        if (_settings.file_import_action === Settings.FileImportActionAppend) return 1
                        if (_settings.file_import_action === Settings.FileImportActionReplace) return 2
                        return 0
                    }
                    model: [
                        qsTr("Ask whether to add or replace"),
                        qsTr("Add to an existing note"),
                        qsTr("Replace an existing note")
                    ]
                    onActivated: {
                        if (index === 1) {
                            _settings.file_import_action = Settings.FileImportActionAppend
                        } else if (index === 2) {
                            _settings.file_import_action = Settings.FileImportActionReplace
                        } else {
                            _settings.file_import_action = Settings.FileImportActionAsk
                        }
                    }
                }
            }

            ComboBoxForm {
                label.text: qsTr("Text appending style")
                toolTip: qsTr("Text is appended to the note in the same line or after line break.")
                comboBox {
                    currentIndex: {
                        if (_settings.insert_mode === Settings.InsertInLine) return 0
                        if (_settings.insert_mode === Settings.InsertNewLine) return 1
                        if (_settings.insert_mode === Settings.InsertAfterEmptyLine) return 2
                        return 0
                    }
                    model: [
                        qsTr("In line"),
                        qsTr("After line break"),
                        qsTr("After empty line")
                    ]
                    onActivated: {
                        if (index === 0) {
                            _settings.insert_mode = Settings.InsertInLine
                        } else if (index === 1) {
                            _settings.insert_mode = Settings.InsertNewLine
                        } else if (index === 2) {
                            _settings.insert_mode = Settings.InsertAfterEmptyLine
                        } else {
                            _settings.insert_mode = Settings.InsertInLine
                        }
                    }
                }
            }

            TextFieldForm {
                label.text: qsTr("Font in text editor")
                compact: true
                textField {
                    text: {
                        var font = _settings.notepad_font
                        if (font.family.length == 0 || font.family == "0")
                            font.family = qsTr("Default")
                        return font.family + " " + font.pointSize + "pt"
                    }
                    readOnly: true
                }
                button {
                    text: qsTr("Change")
                    onClicked: fontDialog.open()
                }
            }

            Dialogs.FontDialog {
                id: fontDialog

                title: qsTr("Please choose a font")
                font: _settings.notepad_font
                onAccepted: {
                    _settings.notepad_font = fontDialog.font
                }
            }

            ComboBoxForm {
                label.text: qsTr("Show desktop notification")
                toolTip: qsTr("Show desktop notification while reading or listening.")
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
                        qsTr("Never"),
                        qsTr("When in background"),
                        qsTr("Always")
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
                text: qsTr("Include recognized or read text in notifications")
                onCheckedChanged: {
                    _settings.desktop_notification_details = checked
                }
            }

            CheckBox {
                checked: _settings.use_tray
                text: qsTr("Use system tray icon")
                onCheckedChanged: {
                    _settings.use_tray = checked
                }
            }

            CheckBox {
                Layout.leftMargin: appWin.padding
                visible: _settings.use_tray
                checked: _settings.start_in_tray
                text: qsTr("Start minimized to the system tray")
                onCheckedChanged: {
                    _settings.start_in_tray = checked
                }
            }

            CheckBox {
                checked: _settings.show_repair_text
                text: qsTr("Show %1 options").arg("<i>" + qsTr("Repair text") + "</i>")
                onCheckedChanged: {
                    _settings.show_repair_text = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Once enabled, a menu with text correction options appears on the main toolbar.")
                hoverEnabled: true
            }

            CheckBox {
                checked: !_settings.qt_style_auto
                text: qsTr("Use custom graphical style")
                onCheckedChanged: {
                    _settings.qt_style_auto = !checked
                }
            }

            ComboBoxForm {
                indends: 1
                visible: !_settings.qt_style_auto
                label.text: qsTr("Graphical style")
                toolTip: qsTr("Application graphical interface style.") + " " +
                         qsTr("Change if you observe problems with incorrect colors under a dark theme.")
                comboBox {
                    currentIndex: _settings.qt_style_idx
                    model: _settings.qt_styles()
                    onActivated: _settings.qt_style_idx = index
                }
            }
        }

        ColumnLayout {
            id: speechToTextTab

            // property bool has_audio_sources: app.audio_sources.length > 1

            ComboBoxForm {
                // visible: speechToTextTab.has_audio_sources
                label.text: qsTr("Audio input device")
                toolTip: qsTr("Select preferred audio input device.")
                comboBox {
                    currentIndex: app.audio_source_idx
                    model: app.audio_sources
                    onActivated: {
                        app.audio_source_idx = index
                    }
                }
            }

            // TipMessage {
            //     indends: 1
            //     visible: !speechToTextTab.has_audio_sources
            //     text: qsTr("No audio source could be found.") + " " +
            //           qsTr("Make sure the microphone is properly connected.")
            // }

            ComboBoxForm {
                // visible: speechToTextTab.has_audio_sources
                label.text: qsTr("Listening mode")
                toolTip: "<i>" + qsTr("One sentence") + "</i>" + " — " + qsTr("Clicking on the %1 button starts listening, which ends when the first sentence is recognized.")
                         .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                         "<i>" + qsTr("Press and hold") + "</i>" + " — " + qsTr("Pressing and holding the %1 button enables listening. When you stop holding, listening will turn off.")
                         .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                         "<i>" + qsTr("Always on") + "</i>" + " — " + qsTr("After clicking on the %1 button, listening is always turn on.")
                         .arg("<i>" + qsTr("Listen") + "</i>")
                comboBox {
                    currentIndex: {
                        if (_settings.speech_mode === Settings.SpeechSingleSentence) return 0
                        if (_settings.speech_mode === Settings.SpeechAutomatic) return 2
                        return 1
                    }
                    model: [
                        qsTr("One sentence"),
                        qsTr("Press and hold"),
                        qsTr("Always on")
                    ]
                    onActivated: {
                        if (index === 0) {
                            _settings.speech_mode = Settings.SpeechSingleSentence
                        } else if (index === 2) {
                            _settings.speech_mode = Settings.SpeechAutomatic
                        } else {
                            _settings.speech_mode = Settings.SpeechManual
                        }
                    }
                }
            }

            CheckBox {
                id: puncCheckBox

                visible: app.feature_punctuator
                checked: _settings.restore_punctuation
                text: qsTr("Restore punctuation")
                onCheckedChanged: {
                    _settings.restore_punctuation = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Enable advanced punctuation restoration after speech recognition. To make it work, " +
                                   "make sure you have enabled %1 model for your language.")
                              .arg("<i>" + qsTr("Punctuation") + "</i>") + " " +
                              qsTr("When this option is enabled model initialization takes much longer and memory usage is much higher.") + " " +
                              qsTr("This option only works with models that do not natively support punctuation.")
                hoverEnabled: true
            }

            TipMessage {
                indends: 1
                visible: app.feature_punctuator &&
                         _settings.restore_punctuation &&
                         !app.ttt_punctuation_configured
                text: qsTr("To make %1 work, download %2 model.")
                      .arg("<i>" + qsTr("Restore punctuation") + "</i>").arg("<i>" + qsTr("Punctuation") + "</i>")
            }

            CheckBox {
                id: statsCheckBox

                checked: _settings.stt_insert_stats
                text: qsTr("Insert statistics")
                onCheckedChanged: {
                    _settings.stt_insert_stats = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Inserts processing related information to the text, such as processing time and audio length.") + " " +
                              qsTr("This option can be useful for comparing the performance of different models, engines and their parameters.") + " " +
                              qsTr("This option does not work with all engines.")
                hoverEnabled: true
            }

            SectionLabel {
                text: qsTr("Subtitles")
            }

            SpinBoxForm {
                label.text: qsTr("Minimum segment duration")
                toolTip: qsTr("Set the minimum duration (in seconds) of the subtitle segment.") + " " +
                         qsTr("This option only works with %1 and %2 models.").arg("<i>DeepSpeech/Coqui</i>").arg("<i>Vosk</i>")
                spinBox {
                    from: 1
                    to: 60
                    stepSize: 1
                    value: _settings.sub_min_segment_dur
                    onValueChanged: {
                        _settings.sub_min_segment_dur = spinBox.value;
                    }
                }
            }

            CheckBox {
                checked: _settings.sub_break_lines
                text: qsTr("Break text lines")
                onCheckedChanged: {
                    _settings.sub_break_lines = checked
                }
            }

            SpinBoxForm {
                indends: 1
                visible: _settings.sub_break_lines
                label.text: qsTr("Minimum line length")
                spinBox {
                    from: 1
                    to: 1000
                    stepSize: 1
                    value: _settings.sub_min_line_length
                    onValueChanged: {
                        _settings.sub_min_line_length = spinBox.value;
                        if (_settings.sub_max_line_length < spinBox.value)
                            _settings.sub_max_line_length = spinBox.value
                    }
                }
            }

            SpinBoxForm {
                indends: 1
                visible: _settings.sub_break_lines
                label.text: qsTr("Maximum line length")
                spinBox {
                    from: 1
                    to: 1000
                    stepSize: 1
                    value: _settings.sub_max_line_length
                    onValueChanged: {
                        _settings.sub_max_line_length = spinBox.value;
                        if (_settings.sub_min_line_length > spinBox.value)
                            _settings.sub_min_line_length = spinBox.value;
                    }
                }
            }

            SectionLabel {
                text: qsTr("Engine options")
            }

            TabBar {
                id: sttEnginesBar

                Layout.fillWidth: true
                currentIndex: app.feature_fasterwhisper_stt ? _settings.settings_stt_engine_idx : 0
                onCurrentIndexChanged: _settings.settings_stt_engine_idx = currentIndex

                TabButton {
                    text: "WhisperCpp"
                    width: implicitWidth
                }

                TabButton {
                    enabled: app.feature_fasterwhisper_stt
                    text: "FasterWhisper"
                    width: implicitWidth
                }
            }

            StackLayout {
                property alias verticalMode: root.verticalMode

                Layout.fillWidth: true
                Layout.topMargin: appWin.padding
                currentIndex: sttEnginesBar.currentIndex

                ColumnLayout {
                    id: whispercppTab

                    SpinBoxForm {
                        id: whispercppThreadsSpinBox

                        label.text: qsTr("Number of simultaneous threads")
                        toolTip: qsTr("Set the maximum number of simultaneous CPU threads.") + " " +
                                 qsTr("A higher value does not necessarily speed up decoding.")
                        spinBox {
                            from: 0
                            to: 32
                            stepSize: 1
                            value: _settings.whispercpp_cpu_threads < 1 ? 1 : _settings.whispercpp_cpu_threads > 32 ? 32 : _settings.whispercpp_cpu_threads
                            textFromValue: function(value) { return value.toString() }
                            valueFromText: function(text) { return parseInt(text); }
                            onValueChanged: {
                                _settings.whispercpp_cpu_threads = spinBox.value;
                            }
                        }
                    }

                    SpinBoxForm {
                        id: whispercppBeamSpinBox

                        label.text: qsTr("Beam search width")
                        toolTip: qsTr("A higher value may improve quality, but decoding time may also increase.")
                        spinBox {
                            from: 0
                            to: 100
                            stepSize: 1
                            value: _settings.whispercpp_beam_search < 1 ? 1 : _settings.whispercpp_beam_search > 100 ? 100 : _settings.whispercpp_beam_search
                            textFromValue: function(value) { return value.toString() }
                            valueFromText: function(text) { return parseInt(text); }
                            onValueChanged: {
                                _settings.whispercpp_beam_search = spinBox.value;
                            }
                        }
                    }

                    ComboBoxForm {
                        id: whispercppContextSizeComboBox

                        label.text: qsTr("Audio context size")
                        toolTip: qsTr("When %1 is set, the size is adjusted dynamically for each audio chunk.").arg("<i>" + qsTr("Dynamic") + "</i>") + " " +
                                 qsTr("When %1 is set, the default fixed size is used.").arg("<i>" + qsTr("Default") + "</i>") + " " +
                                 qsTr("To define a custom size, use the %1 option.").arg("<i>" + qsTr("Custom") + "</i>") + " " +
                                 qsTr("A smaller value speeds up decoding, but can have a negative impact on accuracy.")
                        comboBox {
                            currentIndex: {
                                switch(_settings.whispercpp_audioctx_size) {
                                case Settings.OptionAuto: return 0
                                case Settings.OptionDefault: return 1
                                case Settings.OptionCustom: return 2
                                }
                                return 0
                            }
                            model: [
                                qsTr("Dynamic"),
                                qsTr("Default"),
                                qsTr("Custom")
                            ]
                            onActivated: {
                                if (index === 0) {
                                    _settings.whispercpp_audioctx_size = Settings.OptionAuto
                                } else if (index === 1) {
                                    _settings.whispercpp_audioctx_size = Settings.OptionDefault
                                } else if (index === 2) {
                                    _settings.whispercpp_audioctx_size = Settings.OptionCustom
                                }
                            }
                        }
                    }

                    SpinBoxForm {
                        id: whispercppContextSizeSpinBox

                        indends: 1
                        visible: _settings.whispercpp_audioctx_size === Settings.OptionCustom
                        label.text: qsTr("Size")
                        spinBox {
                            from: 1
                            to: 3000
                            stepSize: 1
                            value: _settings.whispercpp_audioctx_size_value
                            onValueChanged: {
                                _settings.whispercpp_audioctx_size_value = spinBox.value;
                            }
                        }
                        toolTip: qsTr("A smaller value speeds up decoding, but can have a negative impact on accuracy.")
                    }

                    CheckBox {
                        id: whispercppFlashAttnCheckBox

                        checked: _settings.whispercpp_gpu_flash_attn
                        text: qsTr("Use Flash Attention")
                        onCheckedChanged: {
                            _settings.whispercpp_gpu_flash_attn = checked
                        }

                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Flash Attention may reduce the time of decoding when using GPU acceleration.") + " " +
                                      qsTr("Disable this option if you observe problems.")
                        hoverEnabled: true
                    }

                    GpuComboBox {
                        id: whispercppGpuComboBox

                        enabled: _settings.hw_accel_supported() && app.feature_whispercpp_gpu
                        devices: _settings.whispercpp_gpu_devices
                        device_index: _settings.whispercpp_gpu_device_idx
                        use_gpu: _settings.whispercpp_use_gpu
                        onUse_gpuChanged: _settings.whispercpp_use_gpu = use_gpu
                        onDevice_indexChanged: _settings.whispercpp_gpu_device_idx = device_index
                    }

                    // Button {
                    //     text: qsTr("Reset to default values")
                    //     Layout.alignment: Qt.AlignHCenter

                    //     onClicked: {
                    //         _settings.reset_whispercpp_options()
                    //         whispercppGpuComboBox.use_gpu = _settings.whispercpp_use_gpu
                    //         whispercppFlashAttnCheckBox.checked = _settings.whispercpp_gpu_flash_attn
                    //         whispercppBeamSpinBox.spinBox.value = _settings.whispercpp_beam_search
                    //         whispercppThreadsSpinBox.spinBox.value = _settings.whispercpp_cpu_threads
                    //         whispercppContextSizeComboBox.comboBox.currentIndex = 0
                    //         whispercppContextSizeSpinBox.spinBox.value = 1500
                    //     }
                    // }
                }

                ColumnLayout {
                    id: fasterwhisperTab

                    SpinBoxForm {
                        id: fasterwhisperThreadsSpinBox

                        label.text: qsTr("Number of simultaneous threads")
                        toolTip: qsTr("Set the maximum number of simultaneous CPU threads.") + " " +
                                 qsTr("A higher value does not necessarily speed up decoding.")
                        spinBox {
                            from: 0
                            to: 32
                            stepSize: 1
                            value: _settings.fasterwhisper_cpu_threads < 1 ? 1 : _settings.fasterwhisper_cpu_threads > 32 ? 32 : _settings.fasterwhisper_cpu_threads
                            textFromValue: function(value) { return value.toString() }
                            valueFromText: function(text) { return parseInt(text); }
                            onValueChanged: {
                                _settings.fasterwhisper_cpu_threads = spinBox.value;
                            }
                        }
                    }

                    SpinBoxForm {
                        id: fasterwhisperBeamSpinBox

                        label.text: qsTr("Beam search width")
                        toolTip: qsTr("A higher value may improve quality, but decoding time may also increase.")
                        spinBox {
                            from: 0
                            to: 100
                            stepSize: 1
                            value: _settings.fasterwhisper_beam_search < 1 ? 1 : _settings.fasterwhisper_beam_search > 100 ? 100 : _settings.fasterwhisper_beam_search
                            textFromValue: function(value) { return value.toString() }
                            valueFromText: function(text) { return parseInt(text); }
                            onValueChanged: {
                                _settings.fasterwhisper_beam_search = spinBox.value;
                            }
                        }
                    }

                    CheckBox {
                        id: fasterwhisperFlashAttnCheckBox

                        checked: _settings.fasterwhisper_gpu_flash_attn
                        text: qsTr("Use Flash Attention")
                        onCheckedChanged: {
                            _settings.fasterwhisper_gpu_flash_attn = checked
                        }

                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Flash Attention may reduce the time of decoding when using GPU acceleration.") + " " +
                                      qsTr("Disable this option if you observe problems.")
                        hoverEnabled: true
                    }

                    GpuComboBox {
                        id: fasterwhisperGpuComboBox

                        enabled: _settings.hw_accel_supported() && app.feature_fasterwhisper_gpu
                        devices: _settings.fasterwhisper_gpu_devices
                        device_index: _settings.fasterwhisper_gpu_device_idx
                        use_gpu: _settings.fasterwhisper_use_gpu
                        onUse_gpuChanged: _settings.fasterwhisper_use_gpu = use_gpu
                        onDevice_indexChanged: _settings.fasterwhisper_gpu_device_idx = device_index
                    }

                    // Button {
                    //     text: qsTr("Reset to default values")
                    //     Layout.alignment: Qt.AlignHCenter

                    //     onClicked: {
                    //         _settings.reset_fasterwhisper_options()
                    //         fasterwhisperGpuComboBox.use_gpu = _settings.fasterwhisper_use_gpu
                    //         fasterwhisperFlashAttnCheckBox.checked = _settings.fasterwhisper_gpu_flash_attn
                    //         fasterwhisperBeamSpinBox.spinBox.value = _settings.fasterwhisper_beam_search
                    //         fasterwhisperThreadsSpinBox.spinBox.value = _settings.fasterwhisper_cpu_threads
                    //     }
                    // }
                }
            }
        }

        ColumnLayout {
            id: textToSpeechTab

            CheckBox {
                checked: _settings.diacritizer_enabled
                text: qsTr("Restore diacritical marks before speech synthesis")
                onCheckedChanged: {
                    _settings.diacritizer_enabled = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("This works only for Arabic and Hebrew languages.")
                hoverEnabled: true
            }

            TipMessage {
                indends: 1
                visible: _settings.diacritizer_enabled &&
                         app.feature_coqui_tts &&
                         !app.feature_diacritizer_he
                text: qsTr("Diacritics restoration for Hebrew language is not available.")
            }

            CheckBox {
                checked: _settings.tts_split_into_sentences
                text: qsTr("Split text into sentences")
                onCheckedChanged: {
                    _settings.tts_split_into_sentences = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("The text will be divided into sentences and speech synthesis for each sentence will be performed in parallel.") + " " +
                              qsTr("This speeds up reading, but in some models the naturalness of speech may be reduced.")
                hoverEnabled: true
            }

            CheckBox {
                checked: _settings.tts_use_engine_speed_control
                text: qsTr("Use engine speed control")
                onCheckedChanged: {
                    _settings.tts_use_engine_speed_control = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("If the TTS engine supports speed control, it will be used.") + " " +
                              qsTr("When this option is disabled, speed manipulation takes place during audio post-processing.") + " " +
                              qsTr("The actual speed after audio post-processing is much more predictable, but the naturalness of speech may be reduced.")
                hoverEnabled: true
            }

            CheckBox {
                checked: _settings.tts_tag_mode === Settings.TtsTagModeSupport
                text: qsTr("Use control tags")
                onCheckedChanged: {
                    _settings.tts_tag_mode = checked ?
                                Settings.TtsTagModeSupport :
                                Settings.TtsTagModeIgnore;
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Control tags allow you to dynamically change the speed of synthesized text or add silence between sentences.") + " " +
                              qsTr("When this option is disabled, tags are ignored.")
                hoverEnabled: true
            }

            TipMessage {
                color: palette.text
                indends: 1
                visible: _settings.tts_tag_mode === Settings.TtsTagModeSupport
                text: "<p>" + qsTr("Control tags allow you to dynamically change the speed of synthesized text or add silence between sentences.") + " " +
                      qsTr("To use control tags, insert %1 into the text.")
                        .arg("<i>{tag-name: value}</i>") + " " +
                      qsTr("The following control tags are currently supported:") +
                      "</p><ul>" +
                      "<li><i>{speed: X}</i> - " + qsTr("Changes speed.") + " " + qsTr("%1 is a floating-point number in the range from 0.1 to 2.0.").arg("<i>X</i>") + "</li>" +
                      "<li><i>{silence: Xy}</i> - " + qsTr("Inserts silence.") + " " + qsTr("%1 is a floating-point number and %2 is an unit name (%3).").arg("<i>X</i>").arg("<i>y</i>").arg("<i>ms</i>, <i>s</i>, <i>m</i>")+ "</li>" +
                      "</ul>" +
                      "Examples of usage: <i>{speed: 0.5} {speed: 2.0} {silence: 100ms} {silence: 1.5s} {silence: 2m}</i>"
            }

            SectionLabel {
                text: qsTr("Subtitles")
            }

            ComboBoxForm {
                label.text: qsTr("Sync speech with timestamps")
                toolTip: "<i>" + qsTr("Don't sync") + "</i>" + " — " + qsTr("Subtitle timestamps are ignored when reading or exporting to a file.") + "<br/> " +
                         "<i>" + qsTr("Sync but don't adjust speed") + "</i>" + " — " + qsTr("Speech is synchronized according to timestamps.") + "<br/> " +
                         "<i>" + qsTr("Sync and increase speed to fit") + "</i>" + " — " + qsTr("Speech is synchronized according to timestamps. The speed is adjusted automatically so that the duration of the speech is never longer than the duration of the subtitle segment.")  + "<br/> " +
                         "<i>" + qsTr("Sync and increase or decrease speed to fit") + "</i>" + " — " + qsTr("Speech is synchronized according to timestamps. The speed is adjusted automatically so that the duration of the speech is exactly the same as the duration of the subtitle segment.")
                comboBox {
                    currentIndex: {
                        if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOff) return 0
                        if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnDontFit) return 1
                        if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnFitOnlyIfLonger) return 2
                        if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnAlwaysFit) return 3
                        return 1
                    }
                    model: [
                        qsTr("Don't sync"),
                        qsTr("Sync but don't adjust speed"),
                        qsTr("Sync and increase speed to fit"),
                        qsTr("Sync and increase or decrease speed to fit")
                    ]
                    onActivated: {
                        if (index === 1) {
                            _settings.tts_subtitles_sync = Settings.TtsSubtitleSyncOnDontFit
                        } else if (index === 2) {
                            _settings.tts_subtitles_sync = Settings.TtsSubtitleSyncOnFitOnlyIfLonger
                        } else if (index === 3) {
                            _settings.tts_subtitles_sync = Settings.TtsSubtitleSyncOnAlwaysFit
                        } else {
                            _settings.tts_subtitles_sync = Settings.TtsSubtitleSyncOff
                        }
                    }
                }
            }

            TipMessage {
                indends: 1
                color: palette.text
                visible: _settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnFitOnlyIfLonger ||
                         _settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnAlwaysFit
                text: qsTr("When SRT Subtitles text format is set, changing the speech speed is disabled because the speed will be adjusted automatically.")
            }

            SectionLabel {
                text: qsTr("Engine options")
                visible: app.feature_coqui_tts || app.feature_whisperspeech_tts
            }

            TabBar {
                id: ttsEnginesBar

                Layout.fillWidth: true
                currentIndex: !app.feature_coqui_tts ? 1 : !app.feature_whisperspeech_tts ? 0 : _settings.settings_tts_engine_idx
                onCurrentIndexChanged: _settings.settings_tts_engine_idx = currentIndex
                visible: app.feature_coqui_tts || app.feature_whisperspeech_tts

                TabButton {
                    text: "Coqui"
                    enabled: app.feature_coqui_tts
                    width: implicitWidth
                }

                TabButton {
                    text: "WhisperSpeech"
                    enabled: app.feature_whisperspeech_tts
                    width: implicitWidth
                }
            }

            StackLayout {
                property alias verticalMode: root.verticalMode

                Layout.fillWidth: true
                Layout.topMargin: appWin.padding
                currentIndex: ttsEnginesBar.currentIndex

                ColumnLayout {
                    id: coquiTab

                    GpuComboBox {
                        enabled: app.feature_coqui_gpu
                        devices: _settings.coqui_gpu_devices
                        device_index: _settings.coqui_gpu_device_idx
                        use_gpu: _settings.coqui_use_gpu
                        onUse_gpuChanged: _settings.coqui_use_gpu = use_gpu
                        onDevice_indexChanged: _settings.coqui_gpu_device_idx = device_index
                    }
                }

                ColumnLayout {
                    id: whisperspeechTab

                    GpuComboBox {
                        enabled: app.feature_whisperspeech_gpu
                        devices: _settings.whisperspeech_gpu_devices
                        device_index: _settings.whisperspeech_gpu_device_idx
                        use_gpu: _settings.whisperspeech_use_gpu
                        onUse_gpuChanged: _settings.whisperspeech_use_gpu = use_gpu
                        onDevice_indexChanged: _settings.whisperspeech_gpu_device_idx = device_index
                    }
                }
            }
        }

        ColumnLayout {
            id: accessibilityTab

            CheckBox {
                visible: app.feature_global_shortcuts
                checked: _settings.hotkeys_enabled
                text: qsTr("Use global keyboard shortcuts")
                onCheckedChanged: {
                    _settings.hotkeys_enabled = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Shortcuts allow you to start or stop listening and reading using keyboard.") + " " +
                              qsTr("Speech to Text result can be appended to the current note, inserted into any active window (currently in focus) or copied to the clipboard.") + " " +
                              qsTr("Text to Speech reading can be from current note or from text in the clipboard.") + " " +
                              qsTr("Keyboard shortcuts function even when the application is not active (e.g. minimized or in the background).") + " " +
                              qsTr("This feature only works under X11.")
                hoverEnabled: true
            }

            CheckBox {
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                checked: _settings.use_toggle_for_hotkey
                text: qsTr("Toggle behavior")
                onCheckedChanged: {
                    _settings.use_toggle_for_hotkey = checked
                }
                Layout.leftMargin: appWin.padding

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Start listening/reading shortcuts will also stop listening/reading if they are triggered while listening/reading is active.")
                hoverEnabled: true
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Start listening")
                textField {
                    text: _settings.hotkey_start_listening
                    onTextChanged: _settings.hotkey_start_listening = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts && app.feature_text_active_window
                label.text: qsTr("Start listening, text to active window")
                textField {
                    text: _settings.hotkey_start_listening_active_window
                    onTextChanged: _settings.hotkey_start_listening_active_window = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Start listening, text to clipboard")
                textField {
                    text: _settings.hotkey_start_listening_clipboard
                    onTextChanged: _settings.hotkey_start_listening_clipboard = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Stop listening")
                textField {
                    text: _settings.hotkey_stop_listening
                    onTextChanged: _settings.hotkey_stop_listening = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Start reading")
                textField {
                    text: _settings.hotkey_start_reading
                    onTextChanged: _settings.hotkey_start_reading = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Start reading text from clipboard")
                textField {
                    text: _settings.hotkey_start_reading_clipboard
                    onTextChanged: _settings.hotkey_start_reading_clipboard = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Pause/Resume reading")
                textField {
                    text: _settings.hotkey_pause_resume_reading
                    onTextChanged: _settings.hotkey_pause_resume_reading = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Cancel")
                textField {
                    text: _settings.hotkey_cancel
                    onTextChanged: _settings.hotkey_cancel = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Switch to next STT model")
                textField {
                    text: _settings.hotkey_switch_to_next_stt_model
                    onTextChanged: _settings.hotkey_switch_to_next_stt_model = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Switch to previous STT model")
                textField {
                    text: _settings.hotkey_switch_to_prev_stt_model
                    onTextChanged: _settings.hotkey_switch_to_prev_stt_model = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Switch to next TTS model")
                textField {
                    text: _settings.hotkey_switch_to_next_tts_model
                    onTextChanged: _settings.hotkey_switch_to_next_tts_model = text
                }
            }

            TextFieldForm {
                indends: 1
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts
                label.text: qsTr("Switch to previous TTS model")
                textField {
                    text: _settings.hotkey_switch_to_prev_tts_model
                    onTextChanged: _settings.hotkey_switch_to_prev_tts_model = text
                }
            }

            CheckBox {
                checked: _settings.actions_api_enabled
                text: qsTr("Allow external applications to invoke actions")
                onCheckedChanged: {
                    _settings.actions_api_enabled = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Action allow external application to invoke certain operation when %1 is running.")
                              .arg("<i>Speech Note</i>")
                hoverEnabled: true
            }

            TipMessage {
                color: palette.text
                indends: 1
                visible: _settings.actions_api_enabled
                text: "<p>" + qsTr("Action allows external application to invoke certain operation when %1 is running.").arg("<i>Speech Note</i>") + " " +
                      qsTr("An action can be triggered via DBus call or with command-line option.") + " " +
                      qsTr("The following actions are currently supported:") +
                      "</p><ul>" +
                      "<li><i>start-listening</i> - " + qsTr("Starts listening.") + "</li>" +
                      "<li><i>start-listening-active-window</i> (X11) - " + qsTr("Starts listening. The decoded text is inserted into the active window.") + "</li>" +
                      "<li><i>start-listening-clipboard</i> - " + qsTr("Starts listening. The decoded text is copied to the clipboard.") + "</li>" +
                      "<li><i>stop-listening</i> - " + qsTr("Stops listening. The already captured voice is decoded into text.") + "</li>" +
                      "<li><i>start-reading</i> - " + qsTr("Starts reading.") + "</li>" +
                      "<li><i>start-reading-clipboard</i> (X11) - " + qsTr("Starts reading text from the clipboard.") + "</li>" +
                      "<li><i>pause-resume-reading</i> - " + qsTr("Pauses or resumes reading.") + "</li>" +
                      "<li><i>cancel</i> - " + qsTr("Cancels any of the above operations.") + "</li>" +
                      "<li><i>switch-to-next-stt-model</i> - " + qsTr("Switches to next STT model.") + "</li>" +
                      "<li><i>switch-to-prev-stt-model</i> - " + qsTr("Switches to previous STT model.") + "</li>" +
                      "<li><i>switch-to-next-tts-model</i> - " + qsTr("Switches to next TTS model.") + "</li>" +
                      "<li><i>switch-to-prev-tts-model</i> - " + qsTr("Switches to previous TTS model.") + "</li>" +
                      "<li><i>set-stt-model</i> - " + qsTr("Sets STT model.") + "</li>" +
                      "<li><i>set-tts-model</i> - " + qsTr("Sets TTS model.") + "</li>" +
                      "</ul>" +
                      qsTr("For example, to trigger %1 action, execute the following command: %2.")
                              .arg("<i>start-listening</i>")
                              .arg(_settings.is_flatpak() ?
                                "<i>flatpak run net.mkiol.SpeechNote --action start-listening</i>" :
                                "<i>dsnote --action start-listening</i>")
            }
        }

        ColumnLayout {
            id: otherTab

            SectionLabel {
                text: qsTr("Storage")
            }

            TextFieldForm {
                label.text: qsTr("Location of language files")
                toolTip: qsTr("Directory where language files are downloaded to and stored.")
                textField {
                    text: _settings.models_dir
                    readOnly: true
                }
                button {
                    text: qsTr("Change")
                    onClicked: directoryDialog.open()
                }
            }

            CheckBox {
                checked: _settings.cache_policy === Settings.CacheRemove
                text: qsTr("Clear cache on close")
                onCheckedChanged: {
                    _settings.cache_policy = checked ? Settings.CacheRemove : Settings.CacheNoRemove
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("When closing, delete all cached audio files.")
                hoverEnabled: true
            }

            SectionLabel {
                text: qsTr("Availability of optional features")
            }

            ColumnLayout {
                spacing: appWin.padding
                Layout.bottomMargin: appWin.padding

                BusyIndicator {
                    visible: featureRepeter.model.length === 0
                    running: visible
                }

                Repeater {
                    id: featureRepeter

                    model: root.features

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2 * appWin.padding

                        Label {
                            text: modelData[1]
                        }

                        Label {
                            font.bold: true
                            text: modelData[0] ? "✔️" : "✖️"
                            color: modelData[0] ? "green" : "red"
                        }
                    }
                }
            }

            Button {
                id: advancedButton

                Layout.alignment: Qt.AlignHCenter
                checkable: true
                checked: false
                text: checked ? qsTr("Hide advanced settings") : qsTr("Show advanced settings")
            }

            ColumnLayout {
                id: advancedTab

                visible: advancedButton.checked
                width: root.width

                SectionLabel {
                    text: qsTr("CPU options")
                }

                SpinBoxForm {
                    label.text: qsTr("Number of simultaneous threads")
                    toolTip: qsTr("Set the maximum number of simultaneous CPU threads.")
                    spinBox {
                        from: 0
                        to: 32
                        stepSize: 1
                        value: _settings.num_threads < 0 ? 0 : _settings.num_threads > 32 ? 32 : _settings.num_threads
                        textFromValue: function(value) {
                            return value < 1 ? qsTr("Auto") : value.toString()
                        }
                        valueFromText: function(text) {
                            if (text === qsTr("Auto")) return 0
                            return parseInt(text);
                        }
                        onValueChanged: {
                            _settings.num_threads = spinBox.value;
                        }
                    }
                }

                SectionLabel {
                    visible: _settings.hw_accel_supported()
                    text: qsTr("Hardware acceleration options")
                }

                CheckBox {
                    visible: _settings.hw_accel_supported()
                    checked: _settings.hw_scan_cuda
                    text: qsTr("Use %1").arg("NVIDIA CUDA")
                    onCheckedChanged: {
                        _settings.hw_scan_cuda = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find NVIDIA CUDA compatible graphic cards in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.hw_accel_supported()
                    checked: _settings.hw_scan_hip
                    text: qsTr("Use %1").arg("AMD ROCm")
                    onCheckedChanged: {
                        _settings.hw_scan_hip = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find AMD ROCm compatible graphic cards in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.hw_accel_supported()
                    checked: _settings.hw_scan_hip
                    text: qsTr("Use %1").arg("AMD ROCm")
                    onCheckedChanged: {
                        _settings.hw_scan_hip = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find AMD ROCm compatible graphic cards in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.hw_accel_supported()
                    checked: _settings.hw_scan_openvino
                    text: qsTr("Use %1").arg("OpenVINO")
                    onCheckedChanged: {
                        _settings.hw_scan_openvino = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find OpenVINO compatible hardware in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.hw_accel_supported()
                    checked: _settings.hw_scan_opencl
                    text: qsTr("Use %1").arg("OpenCL")
                    onCheckedChanged: {
                        _settings.hw_scan_opencl = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find OpenCL compatible graphic cards in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.hw_accel_supported()
                    checked: _settings.hw_scan_opencl_legacy
                    text: qsTr("Use %1").arg("OpenCL (Clover)")
                    onCheckedChanged: {
                        _settings.hw_scan_opencl_legacy = checked
                    }
                }

                CheckBox {
                    visible: _settings.hw_accel_supported()
                    checked: _settings.gpu_override_version
                    text: qsTr("Override GPU version")
                    onCheckedChanged: {
                        _settings.gpu_override_version = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Override AMD GPU version.") + " " +
                                  qsTr("Enable this option if you observe problems when using GPU acceleration with AMD graphics card.")
                    hoverEnabled: true
                }

                TextFieldForm {
                    indends: 1
                    visible: _settings.hw_accel_supported() && _settings.gpu_override_version
                    label.text: qsTr("Version")
                    toolTip: qsTr("Value has the same effect as %1 environment variable.").arg("<i>HSA_OVERRIDE_GFX_VERSION</i>")
                    textField {
                        text: _settings.gpu_overrided_version
                        onTextChanged: _settings.gpu_overrided_version = text
                    }
                    button {
                        text: qsTr("Reset")
                        onClicked: _settings.gpu_overrided_version = ""
                    }
                }

                SectionLabel {
                    visible: _settings.is_xcb()
                    text: "X11"
                }

                TextFieldForm {
                    visible: _settings.is_xcb()
                    label.text: qsTr("Compose file")
                    toolTip: qsTr("X11 compose file used in %1.").arg("<i>" + qsTr("Insert into active window") + "</i>")
                    textField {
                        text: _settings.x11_compose_file
                        onTextChanged: _settings.x11_compose_file = text
                    }
                }

                SectionLabel {
                    text: qsTr("Libraries")
                }

                CheckBox {
                    checked: _settings.py_feature_scan
                    text: qsTr("Use Python libriaries")
                    onCheckedChanged: {
                        _settings.py_feature_scan = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Check the presence of the required Python libraries.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true

                }

                TextFieldForm {
                    id: pyTextField

                    indends: 1
                    visible: _settings.py_feature_scan && !_settings.is_flatpak()
                    label.text: qsTr("Location of Python libraries")
                    toolTip: qsTr("Python libraries directory (%1).").arg("<i>PYTHONPATH</i>") + " " + qsTr("Leave blank to use the default value.") + " " +
                             qsTr("This option may be useful if you use %1 module to manage Python libraries.").arg("<i>venv</i>")
                    textField {
                        text: _settings.py_path
                        placeholderText: qsTr("Leave blank to use the default value.")
                    }
                    button {
                        text: qsTr("Save")
                        onClicked: _settings.py_path = pyTextField.textField.text
                    }
                }
            }
        }
    }

    Dialogs.FileDialog {
        id: directoryDialog
        title: qsTr("Select Directory")
        selectFolder: true
        selectExisting: true
        folder:  _settings.models_dir_url
        onAccepted: {
            _settings.models_dir_url = fileUrl
        }
    }
}

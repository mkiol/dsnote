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
    // property bool has_audio_sources: app.audio_sources.length > 1

    ComboBoxForm {
        // visible: speechToTextTab.has_audio_sources
        label.text: qsTranslate("SettingsPage", "Audio input device")
        toolTip: qsTranslate("SettingsPage", "Select preferred audio input device.")
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
    //     text: qsTranslate("SettingsPage", "No audio source could be found.") + " " +
    //           qsTranslate("SettingsPage", "Make sure the microphone is properly connected.")
    // }

    ComboBoxForm {
        // visible: speechToTextTab.has_audio_sources
        label.text: qsTranslate("SettingsPage", "Listening mode")
        toolTip: "<i>" + qsTranslate("SettingsPage", "One sentence") + "</i>" + " — " + qsTranslate("SettingsPage", "Clicking on the %1 button starts listening, which ends when the first sentence is recognized.")
                 .arg("<i>" + qsTranslate("SettingsPage", "Listen") + "</i>") + "<br/>" +
                 "<i>" + qsTranslate("SettingsPage", "Press and hold") + "</i>" + " — " + qsTranslate("SettingsPage", "Pressing and holding the %1 button enables listening. When you stop holding, listening will turn off.")
                 .arg("<i>" + qsTranslate("SettingsPage", "Listen") + "</i>") + "<br/>" +
                 "<i>" + qsTranslate("SettingsPage", "Always on") + "</i>" + " — " + qsTranslate("SettingsPage", "After clicking on the %1 button, listening is always turn on.")
                 .arg("<i>" + qsTranslate("SettingsPage", "Listen") + "</i>")
        comboBox {
            currentIndex: {
                if (_settings.speech_mode === Settings.SpeechSingleSentence) return 0
                if (_settings.speech_mode === Settings.SpeechAutomatic) return 2
                return 1
            }
            model: [
                qsTranslate("SettingsPage", "One sentence"),
                qsTranslate("SettingsPage", "Press and hold"),
                qsTranslate("SettingsPage", "Always on")
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
        text: qsTranslate("SettingsPage", "Restore punctuation")
        onCheckedChanged: {
            _settings.restore_punctuation = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Enable advanced punctuation restoration after speech recognition. To make it work, " +
                           "make sure you have enabled %1 model for your language.")
                      .arg("<i>" + qsTranslate("SettingsPage", "Punctuation") + "</i>") + " " +
                      qsTranslate("SettingsPage", "When this option is enabled model initialization takes much longer and memory usage is much higher.") + " " +
                      qsTranslate("SettingsPage", "This option only works with models that do not natively support punctuation.")
        hoverEnabled: true
    }

    TipMessage {
        indends: 1
        visible: app.feature_punctuator &&
                 _settings.restore_punctuation &&
                 !app.ttt_punctuation_configured
        text: qsTranslate("SettingsPage", "To make %1 work, download %2 model.")
              .arg("<i>" + qsTranslate("SettingsPage", "Restore punctuation") + "</i>").arg("<i>" + qsTranslate("SettingsPage", "Punctuation") + "</i>")
    }

    CheckBox {
        checked: _settings.stt_echo
        text: qsTranslate("SettingsPage", "Echo mode")
        onCheckedChanged: {
            _settings.stt_echo = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "After processing, the decoded text will be immediately read out using the currently set Text to Speech model.")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.stt_use_note_as_prompt
        text: qsTranslate("SettingsPage", "Use note as context")
        onCheckedChanged: {
            _settings.stt_use_note_as_prompt = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Use an existing note as the initial context in decoding.") + " " +
                      qsTranslate("SettingsPage", "This option only works with %1 and %2 models.").arg("<i>WhisperCpp</i>").arg("<i>FasterWhisper</i>")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.stt_clear_mic_audio_when_decoding
        text: qsTranslate("SettingsPage", "Pause listening while processing")
        onCheckedChanged: {
            _settings.stt_clear_mic_audio_when_decoding = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Temporarily pause listening for the duration of audio processing.") + " " +
                      qsTranslate("SettingsPage", "This option can be useful when %1 is %2.")
                        .arg("<i>" + qsTranslate("SettingsPage", "Listening mode") + "</i>")
                        .arg("<i>" + qsTranslate("SettingsPage", "Always on") + "</i>")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.stt_play_beep
        text: qsTranslate("SettingsPage", "Play tone when starting and stopping listening")
        onCheckedChanged: {
            _settings.stt_play_beep = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Play an audible tone when starting and stopping listening.")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.stt_insert_stats
        text: qsTranslate("SettingsPage", "Insert statistics")
        onCheckedChanged: {
            _settings.stt_insert_stats = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Inserts processing related information to the text, such as processing time and audio length.") + " " +
                      qsTranslate("SettingsPage", "This option can be useful for comparing the performance of different models, engines and their parameters.") + " " +
                      qsTranslate("SettingsPage", "This option does not work with all engines.")
        hoverEnabled: true
    }

    SectionLabel {
        text: qsTranslate("SettingsPage", "Subtitles")
    }

    SpinBoxForm {
        label.text: qsTranslate("SettingsPage", "Minimum segment duration")
        toolTip: qsTranslate("SettingsPage", "Set the minimum duration (in seconds) of the subtitle segment.") + " " +
                 qsTranslate("SettingsPage", "This option only works with %1 and %2 models.").arg("<i>DeepSpeech/Coqui</i>").arg("<i>Vosk</i>")
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
        text: qsTranslate("SettingsPage", "Break text lines")
        onCheckedChanged: {
            _settings.sub_break_lines = checked
        }
    }

    SpinBoxForm {
        indends: 1
        visible: _settings.sub_break_lines
        label.text: qsTranslate("SettingsPage", "Minimum line length")
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
        label.text: qsTranslate("SettingsPage", "Maximum line length")
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
        text: qsTranslate("SettingsPage", "Engine options")
        visible: sttEnginesBar.visible
    }

    TabBar {
        id: sttEnginesBar

        Layout.fillWidth: true
        currentIndex: !app.feature_whispercpp_stt ? 1 :
                      !app.feature_fasterwhisper_stt ? 0 :
                      _settings.settings_stt_engine_idx
        onCurrentIndexChanged: _settings.settings_stt_engine_idx = currentIndex
        visible: app.feature_whispercpp_stt || app.feature_fasterwhisper_stt

        TabButton {
            enabled: app.feature_whispercpp_stt
            text: "WhisperCpp"
            width: implicitWidth
        }

        TabButton {
            enabled: app.feature_fasterwhisper_stt
            text: "FasterWhisper"
            width: implicitWidth
        }

        Accessible.name: qsTranslate("SettingsPage", "Engine options")
    }

    StackLayout {
        property alias verticalMode: root.verticalMode

        Layout.fillWidth: true
        Layout.topMargin: appWin.padding
        currentIndex: sttEnginesBar.currentIndex
        visible: sttEnginesBar.visible

        ColumnLayout {
            id: whispercppTab

            visible: app.feature_whispercpp_stt

            ComboBoxForm {
                label.text: qsTranslate("SettingsPage", "Profile")
                toolTip: qsTranslate("SettingsPage", "Profiles allow you to change the processing parameters in the engine.") + " " +
                         qsTranslate("SettingsPage", "You can set the parameters to get the fastest processing (%1) or the highest accuracy (%2).")
                         .arg("<i>" + qsTranslate("SettingsPage", "Best performance") + "</i>")
                         .arg("<i>" + qsTranslate("SettingsPage", "Best quality") + "</i>") + " " +
                         qsTranslate("SettingsPage", "If you want to manually set individual engine parameters, select %1.")
                         .arg("<i>" + qsTranslate("SettingsPage", "Custom") + "</i>")
                comboBox {
                    currentIndex: {
                        switch(_settings.whispercpp_profile) {
                        case Settings.EngineProfilePerformance: return 0
                        case Settings.EngineProfileQuality: return 1
                        case Settings.EngineProfileCustom: return 2
                        }
                        return 0
                    }
                    model: [
                        qsTranslate("SettingsPage", "Best performance"),
                        qsTranslate("SettingsPage", "Best quality"),
                        qsTranslate("SettingsPage", "Custom")
                    ]
                    onActivated: {
                        if (index === 0) {
                            _settings.whispercpp_profile = Settings.EngineProfilePerformance
                        } else if (index === 1) {
                            _settings.whispercpp_profile = Settings.EngineProfileQuality
                        } else if (index === 2) {
                            _settings.whispercpp_profile = Settings.EngineProfileCustom
                        } else {
                            _settings.whispercpp_profile = Settings.EngineProfilePerformance
                        }
                    }
                }
            }

            SpinBoxForm {
                id: whispercppThreadsSpinBox

                visible: _settings.whispercpp_profile === Settings.EngineProfileCustom
                label.text: qsTranslate("SettingsPage", "Number of simultaneous threads")
                toolTip: qsTranslate("SettingsPage", "Set the maximum number of simultaneous CPU threads.") + " " +
                         qsTranslate("SettingsPage", "A higher value does not necessarily speed up decoding.")
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
                button {
                    icon.name: "edit-reset-symbolic"
                    display: root.verticalMode ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
                    text: qsTranslate("SettingsPage", "Reset")
                    onClicked: _settings.reset_whispercpp_cpu_threads()
                }
            }

            SpinBoxForm {
                id: whispercppBeamSpinBox

                visible: _settings.whispercpp_profile === Settings.EngineProfileCustom
                label.text: qsTranslate("SettingsPage", "Beam search width")
                toolTip: qsTranslate("SettingsPage", "A higher value may improve quality, but decoding time may also increase.")
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
                button {
                    icon.name: "edit-reset-symbolic"
                    display: root.verticalMode ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
                    text: qsTranslate("SettingsPage", "Reset")
                    onClicked: _settings.reset_whispercpp_beam_search()
                }
            }

            ComboBoxForm {
                id: whispercppContextSizeComboBox

                visible: _settings.whispercpp_profile === Settings.EngineProfileCustom
                label.text: qsTranslate("SettingsPage", "Audio context size")
                toolTip: qsTranslate("SettingsPage", "When %1 is set, the size is adjusted dynamically for each audio chunk.").arg("<i>" + qsTranslate("SettingsPage", "Dynamic") + "</i>") + " " +
                         qsTranslate("SettingsPage", "When %1 is set, the default fixed size is used.").arg("<i>" + qsTranslate("SettingsPage", "Default") + "</i>") + " " +
                         qsTranslate("SettingsPage", "To define a custom size, use the %1 option.").arg("<i>" + qsTranslate("SettingsPage", "Custom") + "</i>") + " " +
                         qsTranslate("SettingsPage", "A smaller value speeds up decoding, but can have a negative impact on accuracy.")
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
                        qsTranslate("SettingsPage", "Dynamic"),
                        qsTranslate("SettingsPage", "Default"),
                        qsTranslate("SettingsPage", "Custom")
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
                button {
                    icon.name: "edit-reset-symbolic"
                    display: root.verticalMode ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
                    text: qsTranslate("SettingsPage", "Reset")
                    onClicked: {
                        _settings.reset_whispercpp_audioctx_size()
                        _settings.reset_whispercpp_audioctx_size_value()
                    }
                }
            }

            SpinBoxForm {
                id: whispercppContextSizeSpinBox

                indends: 1
                visible: _settings.whispercpp_profile === Settings.EngineProfileCustom &&
                         _settings.whispercpp_audioctx_size === Settings.OptionCustom
                label.text: qsTranslate("SettingsPage", "Size")
                spinBox {
                    from: 1
                    to: 3000
                    stepSize: 1
                    value: _settings.whispercpp_audioctx_size_value
                    onValueChanged: {
                        _settings.whispercpp_audioctx_size_value = spinBox.value;
                    }
                }
                button {
                    icon.name: "edit-reset-symbolic"
                    display: root.verticalMode ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
                    text: qsTranslate("SettingsPage", "Reset")
                    onClicked: _settings.reset_whispercpp_audioctx_size_value()
                }
                toolTip: qsTranslate("SettingsPage", "A smaller value speeds up decoding, but can have a negative impact on accuracy.")
            }

            CheckBox {
                id: whispercppFlashAttnCheckBox

                visible: _settings.whispercpp_profile === Settings.EngineProfileCustom
                checked: _settings.whispercpp_gpu_flash_attn
                text: qsTranslate("SettingsPage", "Use Flash Attention")
                onCheckedChanged: {
                    _settings.whispercpp_gpu_flash_attn = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTranslate("SettingsPage", "Flash Attention may reduce the time of decoding when using GPU acceleration.") + " " +
                              qsTranslate("SettingsPage", "Disable this option if you observe problems.")
                hoverEnabled: true
            }

            CheckBox {
                id: whispercppAutolangSupCheckBox

                checked: _settings.whispercpp_autolang_with_sup
                text: qsTranslate("SettingsPage", "Use %1 model for automatic language detection").arg("<i>Tiny</i>")
                onCheckedChanged: {
                    _settings.whispercpp_autolang_with_sup = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTranslate("SettingsPage", "In automatic language detection, the %1 model is used instead of the selected model.").arg("<i>Tiny</i>") + " " +
                              qsTranslate("SettingsPage", "This reduces processing time, but the automatically detected language may be incorrect.")
                hoverEnabled: true
            }

            GpuComboBox {
                id: whispercppGpuComboBox

                visible: _settings.hw_accel_supported() && app.feature_whispercpp_gpu
                devices: _settings.whispercpp_gpu_devices
                device_index: _settings.whispercpp_gpu_device_idx
                use_gpu: _settings.whispercpp_use_gpu
                onUse_gpuChanged: _settings.whispercpp_use_gpu = use_gpu
                onDevice_indexChanged: _settings.whispercpp_gpu_device_idx = device_index
            }
        }

        ColumnLayout {
            id: fasterwhisperTab

            visible: app.feature_fasterwhisper_stt

            ComboBoxForm {
                label.text: qsTranslate("SettingsPage", "Profile")
                toolTip: qsTranslate("SettingsPage", "Profiles allow you to change the processing parameters in the engine.") + " " +
                         qsTranslate("SettingsPage", "You can set the parameters to get the fastest processing (%1) or the highest accuracy (%2).")
                         .arg("<i>" + qsTranslate("SettingsPage", "Best performance") + "</i>")
                         .arg("<i>" + qsTranslate("SettingsPage", "Best quality") + "</i>") + " " +
                         qsTranslate("SettingsPage", "If you want to manually set individual engine parameters, select %1.")
                         .arg("<i>" + qsTranslate("SettingsPage", "Custom") + "</i>")
                comboBox {
                    currentIndex: {
                        switch(_settings.fasterwhisper_profile) {
                        case Settings.EngineProfilePerformance: return 0
                        case Settings.EngineProfileQuality: return 1
                        case Settings.EngineProfileCustom: return 2
                        }
                        return 0
                    }
                    model: [
                        qsTranslate("SettingsPage", "Best performance"),
                        qsTranslate("SettingsPage", "Best quality"),
                        qsTranslate("SettingsPage", "Custom")
                    ]
                    onActivated: {
                        if (index === 0) {
                            _settings.fasterwhisper_profile = Settings.EngineProfilePerformance
                        } else if (index === 1) {
                            _settings.fasterwhisper_profile = Settings.EngineProfileQuality
                        } else if (index === 2) {
                            _settings.fasterwhisper_profile = Settings.EngineProfileCustom
                        } else {
                            _settings.fasterwhisper_profile = Settings.EngineProfilePerformance
                        }
                    }
                }
            }

            SpinBoxForm {
                id: fasterwhisperThreadsSpinBox

                visible: _settings.fasterwhisper_profile === Settings.EngineProfileCustom
                label.text: qsTranslate("SettingsPage", "Number of simultaneous threads")
                toolTip: qsTranslate("SettingsPage", "Set the maximum number of simultaneous CPU threads.") + " " +
                         qsTranslate("SettingsPage", "A higher value does not necessarily speed up decoding.")
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
                button {
                    icon.name: "edit-reset-symbolic"
                    display: root.verticalMode ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
                    text: qsTranslate("SettingsPage", "Reset")
                    onClicked: _settings.reset_fasterwhisper_cpu_threads()
                }
            }

            SpinBoxForm {
                id: fasterwhisperBeamSpinBox

                visible: _settings.fasterwhisper_profile === Settings.EngineProfileCustom
                label.text: qsTranslate("SettingsPage", "Beam search width")
                toolTip: qsTranslate("SettingsPage", "A higher value may improve quality, but decoding time may also increase.")
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
                button {
                    icon.name: "edit-reset-symbolic"
                    display: root.verticalMode ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
                    text: qsTranslate("SettingsPage", "Reset")
                    onClicked: _settings.reset_fasterwhisper_beam_search()
                }
            }

            CheckBox {
                id: fasterwhisperFlashAttnCheckBox

                visible: _settings.fasterwhisper_profile === Settings.EngineProfileCustom
                checked: _settings.fasterwhisper_gpu_flash_attn
                text: qsTranslate("SettingsPage", "Use Flash Attention")
                onCheckedChanged: {
                    _settings.fasterwhisper_gpu_flash_attn = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTranslate("SettingsPage", "Flash Attention may reduce the time of decoding when using GPU acceleration.") + " " +
                              qsTranslate("SettingsPage", "Disable this option if you observe problems.")
                hoverEnabled: true
            }

            GpuComboBox {
                id: fasterwhisperGpuComboBox

                visible: _settings.hw_accel_supported() && app.feature_fasterwhisper_gpu
                devices: _settings.fasterwhisper_gpu_devices
                device_index: _settings.fasterwhisper_gpu_device_idx
                use_gpu: _settings.fasterwhisper_use_gpu
                onUse_gpuChanged: _settings.fasterwhisper_use_gpu = use_gpu
                onDevice_indexChanged: _settings.fasterwhisper_gpu_device_idx = device_index
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }
}


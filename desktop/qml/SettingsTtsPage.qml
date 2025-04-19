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

    CheckBox {
        checked: _settings.diacritizer_enabled
        text: qsTranslate("SettingsPage", "Restore diacritical marks before speech synthesis")
        onCheckedChanged: {
            _settings.diacritizer_enabled = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "This works only for Arabic and Hebrew languages.")
        hoverEnabled: true
    }

    TipMessage {
        indends: 1
        visible: _settings.diacritizer_enabled &&
                 app.feature_coqui_tts &&
                 !app.feature_diacritizer_he
        text: qsTranslate("SettingsPage", "Diacritics restoration for Hebrew language is not available.")
    }

    CheckBox {
        checked: _settings.tts_split_into_sentences
        text: qsTranslate("SettingsPage", "Split text into sentences")
        onCheckedChanged: {
            _settings.tts_split_into_sentences = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "The text will be divided into sentences and speech synthesis for each sentence will be performed in parallel.") + " " +
                      qsTranslate("SettingsPage", "This speeds up reading, but in some models the naturalness of speech may be reduced.")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.tts_use_engine_speed_control
        text: qsTranslate("SettingsPage", "Use engine speed control")
        onCheckedChanged: {
            _settings.tts_use_engine_speed_control = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "If the TTS engine supports speed control, it will be used.") + " " +
                      qsTranslate("SettingsPage", "When this option is disabled, speed manipulation takes place during audio post-processing.") + " " +
                      qsTranslate("SettingsPage", "The actual speed after audio post-processing is much more predictable, but the naturalness of speech may be reduced.")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.tts_normalize_audio
        text: qsTranslate("SettingsPage", "Normalize audio")
        onCheckedChanged: {
            _settings.tts_normalize_audio = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "The volume of the audio will be normalized.") + " " +
                      qsTranslate("SettingsPage", "The volume is normalized independently for each sentence, which can lead to unstable volume levels in different sentences.") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe this problem.")
        hoverEnabled: true
    }

    CheckBox {
        checked: _settings.tts_tag_mode === Settings.TtsTagModeSupport
        text: qsTranslate("SettingsPage", "Use control tags")
        onCheckedChanged: {
            _settings.tts_tag_mode = checked ?
                        Settings.TtsTagModeSupport :
                        Settings.TtsTagModeIgnore;
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Control tags allow you to dynamically change the speed of synthesized text or add silence between sentences.") + " " +
                      qsTranslate("SettingsPage", "When this option is disabled, tags are ignored.")
        hoverEnabled: true
    }

    TipMessage {
        color: palette.text
        indends: 1
        visible: _settings.tts_tag_mode === Settings.TtsTagModeSupport
        text: "<p>" + qsTranslate("SettingsPage", "Control tags allow you to dynamically change the speed of synthesized text or add silence between sentences.") + " " +
              qsTranslate("SettingsPage", "To use control tags, insert %1 into the text.")
                .arg("<i>{tag-name: value}</i>") + " " +
              qsTranslate("SettingsPage", "The following control tags are currently supported:") +
              "</p><ul>" +
              "<li><i>{speed: X}</i> - " + qsTranslate("SettingsPage", "Changes speed.") + " " + qsTranslate("SettingsPage", "%1 is a floating-point number in the range from 0.1 to 2.0.").arg("<i>X</i>") + "</li>" +
              "<li><i>{silence: Xy}</i> - " + qsTranslate("SettingsPage", "Inserts silence.") + " " + qsTranslate("SettingsPage", "%1 is a floating-point number and %2 is an unit name (%3).").arg("<i>X</i>").arg("<i>y</i>").arg("<i>ms</i>, <i>s</i>, <i>m</i>")+ "</li>" +
              "</ul>" +
              "Examples of usage: <i>{speed: 0.5} {speed: 2.0} {silence: 100ms} {silence: 1.5s} {silence: 2m}</i>"
    }

    SectionLabel {
        text: qsTranslate("SettingsPage", "Subtitles")
    }

    ComboBoxForm {
        label.text: qsTranslate("SettingsPage", "Sync speech with timestamps")
        toolTip: "<i>" + qsTranslate("SettingsPage", "Don't sync") + "</i>" + " — " + qsTranslate("SettingsPage", "Subtitle timestamps are ignored when reading or exporting to a file.") + "<br/> " +
                 "<i>" + qsTranslate("SettingsPage", "Sync but don't adjust speed") + "</i>" + " — " + qsTranslate("SettingsPage", "Speech is synchronized according to timestamps.") + "<br/> " +
                 "<i>" + qsTranslate("SettingsPage", "Sync and increase speed to fit") + "</i>" + " — " + qsTranslate("SettingsPage", "Speech is synchronized according to timestamps. The speed is adjusted automatically so that the duration of the speech is never longer than the duration of the subtitle segment.")  + "<br/> " +
                 "<i>" + qsTranslate("SettingsPage", "Sync and increase or decrease speed to fit") + "</i>" + " — " + qsTranslate("SettingsPage", "Speech is synchronized according to timestamps. The speed is adjusted automatically so that the duration of the speech is exactly the same as the duration of the subtitle segment.")
        comboBox {
            currentIndex: {
                if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOff) return 0
                if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnDontFit) return 1
                if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnFitOnlyIfLonger) return 2
                if (_settings.tts_subtitles_sync === Settings.TtsSubtitleSyncOnAlwaysFit) return 3
                return 1
            }
            model: [
                qsTranslate("SettingsPage", "Don't sync"),
                qsTranslate("SettingsPage", "Sync but don't adjust speed"),
                qsTranslate("SettingsPage", "Sync and increase speed to fit"),
                qsTranslate("SettingsPage", "Sync and increase or decrease speed to fit")
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
        text: qsTranslate("SettingsPage", "When SRT Subtitles text format is set, changing the speech speed is disabled because the speed will be adjusted automatically.")
    }

    BusyIndicator {
        visible: app.busy && !ttsEnginesBar.visible
        running: visible
    }

    SectionLabel {
        text: qsTranslate("SettingsPage", "Engine options")
        visible: ttsEnginesBar.visible
    }

    TabBar {
        id: ttsEnginesBar

        Layout.fillWidth: true
        currentIndex: {
            var idx = _settings.settings_tts_engine_idx
            var coqui_enable = app.feature_coqui_tts && app.feature_coqui_gpu;
            var parler_enable = app.feature_parler_tts && app.feature_parler_gpu
            var f5_enable = app.feature_f5_tts && app.feature_f5_gpu
            var kokoro_enable = app.feature_kokoro_tts && app.feature_kokoro_gpu
            var whisperspeech_enable = app.feature_whisperspeech_tts && app.feature_whisperspeech_gpu

            // set idx if engine enabled
            if (idx === 0 && coqui_enable) return idx
            if (idx === 1 && parler_enable) return idx
            if (idx === 2 && f5_enable) return idx
            if (idx === 3 && kokoro_enable) return idx
            if (idx === 4 && whisperspeech_enable) return idx

            // set default
            if (coqui_enable) return 0
            if (parler_enable) return 1
            if (f5_enable) return 2
            if (kokoro_enable) return 3
            if (whisperspeech_enable) return 4

            return 0
        }

        onCurrentIndexChanged: _settings.settings_tts_engine_idx = currentIndex
        visible: (app.feature_coqui_tts && app.feature_coqui_gpu) ||
                 (app.feature_whisperspeech_tts && app.feature_whisperspeech_gpu) ||
                 (app.feature_parler_tts && app.feature_parler_gpu) ||
                 (app.feature_f5_tts && app.feature_f5_gpu) ||
                 (app.feature_kokoro_tts && app.feature_kokoro_gpu)

        TabButton {
            text: "Coqui"
            enabled: app.feature_coqui_tts && app.feature_coqui_gpu
            width: implicitWidth
        }

        TabButton {
            text: "Parler-TTS"
            enabled: app.feature_parler_tts && app.feature_parler_gpu
            width: implicitWidth
        }

        TabButton {
            text: "F5-TTS"
            enabled: app.feature_f5_tts && app.feature_f5_gpu
            width: implicitWidth
        }

        TabButton {
            text: "Kokoro"
            enabled: app.feature_kokoro_tts && app.feature_kokoro_gpu
            width: implicitWidth
        }

        TabButton {
            text: "WhisperSpeech"
            enabled: app.feature_whisperspeech_tts && app.feature_whisperspeech_gpu
            width: implicitWidth
        }

        Accessible.name: qsTranslate("SettingsPage", "Engine options")
    }

    StackLayout {
        property alias verticalMode: root.verticalMode

        Layout.fillWidth: true
        Layout.topMargin: appWin.padding
        currentIndex: ttsEnginesBar.currentIndex
        visible: ttsEnginesBar.visible

        ColumnLayout {
            id: coquiTab

            visible: app.feature_coqui_tts && app.feature_coqui_gpu

            GpuComboBox {
                devices: _settings.coqui_gpu_devices
                device_index: _settings.coqui_gpu_device_idx
                use_gpu: _settings.coqui_use_gpu
                onUse_gpuChanged: _settings.coqui_use_gpu = use_gpu
                onDevice_indexChanged: _settings.coqui_gpu_device_idx = device_index
            }
        }

        ColumnLayout {
            id: parlerTab

            visible: app.feature_parler_tts && app.feature_parler_gpu

            GpuComboBox {
                devices: _settings.parler_gpu_devices
                device_index: _settings.parler_gpu_device_idx
                use_gpu: _settings.parler_use_gpu
                onUse_gpuChanged: _settings.parler_use_gpu = use_gpu
                onDevice_indexChanged: _settings.parler_gpu_device_idx = device_index
            }
        }

        ColumnLayout {
            id: f5Tab

            visible: app.feature_f5_tts && app.feature_f5_gpu

            GpuComboBox {
                devices: _settings.f5_gpu_devices
                device_index: _settings.f5_gpu_device_idx
                use_gpu: _settings.f5_use_gpu
                onUse_gpuChanged: _settings.f5_use_gpu = use_gpu
                onDevice_indexChanged: _settings.f5_gpu_device_idx = device_index
            }
        }

        ColumnLayout {
            id: kokoroTab

            visible: app.feature_kokoro_tts && app.feature_kokoro_gpu

            GpuComboBox {
                devices: _settings.kokoro_gpu_devices
                device_index: _settings.kokoro_gpu_device_idx
                use_gpu: _settings.kokoro_use_gpu
                onUse_gpuChanged: _settings.kokoro_use_gpu = use_gpu
                onDevice_indexChanged: _settings.kokoro_gpu_device_idx = device_index
            }
        }

        ColumnLayout {
            id: whisperspeechTab

            visible: app.feature_whisperspeech_tts && app.feature_whisperspeech_gpu

            GpuComboBox {
                devices: _settings.whisperspeech_gpu_devices
                device_index: _settings.whisperspeech_gpu_device_idx
                use_gpu: _settings.whisperspeech_use_gpu
                onUse_gpuChanged: _settings.whisperspeech_use_gpu = use_gpu
                onDevice_indexChanged: _settings.whisperspeech_gpu_device_idx = device_index
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }
}


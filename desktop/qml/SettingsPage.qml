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

    property bool verticalMode: width < appWin.verticalWidthThreshold
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
        width: parent.width
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

            Dot {
                visible: _settings.hint_addons
                size: parent.height / 5

                anchors {
                    right: parent.right
                    rightMargin: size / 2
                    top: parent.top
                    topMargin: size / 2
                }
            }
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
        width: root.width
        Layout.topMargin: appWin.padding

        currentIndex: bar.currentIndex

        ColumnLayout {
            id: userInterfaceTab

            width: root.width

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

            GridLayout {
                id: grid

                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    text: qsTr("File import action")
                }
                ComboBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
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

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The action when importing a note from a file. You can add imported text to an existing note or replace an existing note.")
                    hoverEnabled: true
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Text appending style")
                }
                ComboBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
                    currentIndex: {
                        if (_settings.insert_mode === Settings.InsertInLine) return 0
                        if (_settings.insert_mode === Settings.InsertNewLine) return 1
                        return 0
                    }
                    model: [
                        qsTr("In line"),
                        qsTr("After line break")
                    ]
                    onActivated: {
                        if (index === 0) {
                            _settings.insert_mode = Settings.InsertInLine
                        } else if (index === 1) {
                            _settings.insert_mode = Settings.InsertNewLine
                        } else {
                            _settings.insert_mode = Settings.InsertInLine
                        }
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Text is appended to the note in the same line or after line break.")
                    hoverEnabled: true
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Font size in text editor")
                    wrapMode: Text.Wrap
                }
                SpinBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
                    from: 4
                    to: 25
                    stepSize: 1
                    value: _settings.font_size < 5 ? 4 : _settings.font_size
                    textFromValue: function(value) {
                        return value < 5 ? qsTr("Auto") : value.toString() + " px"
                    }
                    valueFromText: function(text) {
                        if (text === qsTr("Auto")) return 4
                        return parseInt(text);
                    }
                    onValueChanged: {
                        _settings.font_size = value;
                    }
                    Component.onCompleted: {
                        contentItem.color = palette.text
                    }
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Show desktop notification")
                }
                ComboBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
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

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Show desktop notification while reading or listening.")
                    hoverEnabled: true
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
                checked: !_settings.qt_style_auto
                text: qsTr("Use custom graphical style")
                onCheckedChanged: {
                    _settings.qt_style_auto = !checked
                }
            }

            GridLayout {
                visible: !_settings.qt_style_auto
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.leftMargin: appWin.padding
                    Layout.fillWidth: true
                    text: qsTr("Graphical style")
                    wrapMode: Text.Wrap
                }
                ComboBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    currentIndex: _settings.qt_style_idx
                    model: _settings.qt_styles()
                    onActivated: _settings.qt_style_idx = index

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Application graphical interface style.") + " " +
                                  qsTr("Change if you observe problems with incorrect colors under a dark theme.")
                    hoverEnabled: true
                }
            }
        }

        ColumnLayout {
            id: speechToTextTab

            width: root.width

            GridLayout {
                visible: _settings.audio_inputs.length > 1
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    text: qsTr("Audio source")
                }
                ComboBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
                    currentIndex: _settings.audio_input_idx
                    model: _settings.audio_inputs
                    onActivated: {
                        _settings.audio_input_idx = index
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Select preferred audio source.")
                    hoverEnabled: true
                }
            }

            InlineMessage {
                color: "red"
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
                visible: _settings.audio_inputs.length <= 1

                Label {
                    color: "red"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("No audio source could be found.") + " " + qsTr("Make sure the microphone is properly connected.")
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Listening mode")
                }
                ComboBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
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

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: "<i>" + qsTr("One sentence") + "</i>" + " — " + qsTr("Clicking on the %1 button starts listening, which ends when the first sentence is recognized.")
                                    .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                                  "<i>" + qsTr("Press and hold") + "</i>" + " — " + qsTr("Pressing and holding the %1 button enables listening. When you stop holding, listening will turn off.")
                                    .arg("<i>" + qsTr("Listen") + "</i>") + "<br/>" +
                                  "<i>" + qsTr("Always on") + "</i>" + " — " + qsTr("After clicking on the %1 button, listening is always turn on.")
                                    .arg("<i>" + qsTr("Listen") + "</i>")
                    hoverEnabled: true
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

            InlineMessage {
                color: "red"
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
                visible: app.feature_punctuator &&
                         _settings.restore_punctuation &&
                         !app.ttt_configured

                Label {
                    color: "red"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("To make %1 work, download %2 model.")
                            .arg("<i>" + qsTr("Restore punctuation") + "</i>").arg("<i>" + qsTr("Punctuation") + "</i>")
                }
            }

            CheckBox {
                visible: _settings.gpu_supported() && app.feature_gpu_stt
                checked: _settings.stt_use_gpu
                text: qsTr("Use GPU acceleration")
                onCheckedChanged: {
                    _settings.stt_use_gpu = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("If a suitable graphics card is found in the system, it will be used to accelerate processing.") + " " +
                              qsTr("GPU hardware acceleration significantly reduces the time of decoding.") + " " +
                              qsTr("Disable this option if you observe problems when using Speech to Text.")
                hoverEnabled: true
            }

            InlineMessage {
                color: "red"
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
                visible: _settings.gpu_supported() &&
                         app.feature_gpu_stt &&
                         _settings.stt_use_gpu &&
                         _settings.gpu_devices_stt.length <= 1

                Label {
                    color: "red"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("A suitable graphics card could not be found.")
                }
            }

            GridLayout {
                visible: _settings.gpu_supported() &&
                         app.feature_gpu_stt &&
                         _settings.stt_use_gpu &&
                         _settings.gpu_devices_stt.length > 1
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    wrapMode: Text.Wrap
                    Layout.leftMargin: appWin.padding
                    Layout.fillWidth: true
                    text: qsTr("Graphics card")
                }
                ComboBox {
                    id: sttGpuCombo

                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    currentIndex: _settings.gpu_device_idx_stt
                    model: _settings.gpu_devices_stt
                    onActivated: {
                        _settings.gpu_device_idx_stt = index
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Select preferred graphics card for hardware acceleration.")
                }
            }

            InlineMessage {
                color: palette.text
                Layout.fillWidth: true
                Layout.leftMargin: verticalMode ? 2 * appWin.padding : appWin.padding
                visible: _settings.gpu_supported() &&
                         app.feature_gpu_stt &&
                         _settings.stt_use_gpu &&
                         _settings.gpu_devices_stt.length > 1 &&
                         sttGpuCombo.displayText.search("ROCm") !== -1


                Label {
                    color: palette.text
                    Layout.fillWidth: true
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    text: qsTr("Tip: If you observe problems with GPU acceleration, try to enable %1 option.")
                          .arg("<i>" + qsTr("Other") + "</i> &rarr; <i>" +
                                       qsTr("Override GPU version") + "</i>");
                }
            }

            SectionLabel {
                text: qsTr("Subtitles")
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Minimum segment duration")
                    wrapMode: Text.Wrap
                }

                SpinBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
                    from: 1
                    to: 60
                    stepSize: 1
                    value: _settings.sub_min_segment_dur
                    onValueChanged: {
                        _settings.sub_min_segment_dur = value;
                    }
                    Component.onCompleted: {
                        contentItem.color = palette.text
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set the minimum duration (in seconds) of the subtitle segment.") + " " +
                                  qsTr("This option only works with %1 and %2 models.").arg("<i>DeepSpeech/Coqui</i>").arg("<i>Vosk</i>")
                    hoverEnabled: true
                }
            }

            CheckBox {
                checked: _settings.sub_break_lines
                text: qsTr("Break text lines")
                onCheckedChanged: {
                    _settings.sub_break_lines = checked
                }
            }

            GridLayout {
                visible: _settings.sub_break_lines
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Minimum line length")
                    wrapMode: Text.Wrap
                }

                SpinBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    from: 1
                    to: 1000
                    stepSize: 1
                    value: _settings.sub_min_line_length
                    onValueChanged: {
                        _settings.sub_min_line_length = value;
                        if (_settings.sub_max_line_length < value)
                            _settings.sub_max_line_length = value;
                    }
                    Component.onCompleted: {
                        contentItem.color = palette.text
                    }
                }
            }

            GridLayout {
                visible: _settings.sub_break_lines
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Maximum line length")
                    wrapMode: Text.Wrap
                }

                SpinBox {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    from: 1
                    to: 1000
                    stepSize: 1
                    value: _settings.sub_max_line_length
                    onValueChanged: {
                        _settings.sub_max_line_length = value;
                        if (_settings.sub_min_line_length > value)
                            _settings.sub_min_line_length = value;
                    }
                    Component.onCompleted: {
                        contentItem.color = palette.text
                    }
                }
            }
        }

        ColumnLayout {
            id: textToSpeechTab

            width: root.width

            CheckBox {
                checked: _settings.diacritizer_enabled
                text: qsTr("Restore diacritics before speech synthesis")
                onCheckedChanged: {
                    _settings.diacritizer_enabled = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("This works only for Arabic and Hebrew languages.")
                hoverEnabled: true
            }

            InlineMessage {
                color: "red"
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
                visible: _settings.diacritizer_enabled &&
                         app.feature_coqui_tts &&
                         !app.feature_diacritizer_he

                Label {
                    color: "red"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("Diacritics restoration for Hebrew language is not available.")
                }
            }

            CheckBox {
                checked: _settings.tts_use_gpu
                visible: _settings.gpu_supported() && app.feature_gpu_tts
                text: qsTr("Use GPU acceleration")
                onCheckedChanged: {
                    _settings.tts_use_gpu = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("If a suitable graphics card is found in the system, it will be used to accelerate processing.") + " " +
                              qsTr("GPU hardware acceleration significantly reduces the time of speech synthesis.") + " " +
                              qsTr("Disable this option if you observe problems when using Text to Speech.")
                hoverEnabled: true
            }

            InlineMessage {
                color: "red"
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
                visible: _settings.gpu_supported() &&
                         app.feature_gpu_tts &&
                         _settings.tts_use_gpu &&
                         _settings.gpu_devices_tts.length <= 1

                Label {
                    color: "red"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("A suitable graphics card could not be found.")
                }
            }

            GridLayout {
                visible: _settings.gpu_supported() &&
                         app.feature_gpu_tts &&
                         _settings.tts_use_gpu &&
                         _settings.gpu_devices_tts.length > 1
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    wrapMode: Text.Wrap
                    Layout.leftMargin: appWin.padding
                    Layout.fillWidth: true
                    text: qsTr("Graphics card")
                }
                ComboBox {
                    id: ttsGpuCombo

                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    currentIndex: _settings.gpu_device_idx_tts
                    model: _settings.gpu_devices_tts
                    onActivated: {
                        _settings.gpu_device_idx_tts = index
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Select preferred graphics card for hardware acceleration.")
                    hoverEnabled: true
                }
            }

            InlineMessage {
                color: palette.text
                Layout.fillWidth: true
                Layout.leftMargin: verticalMode ? 2 * appWin.padding : appWin.padding
                visible: _settings.gpu_supported() &&
                         app.feature_gpu_tts &&
                         _settings.tts_use_gpu &&
                         _settings.gpu_devices_tts.length > 1 &&
                         ttsGpuCombo.displayText.search("ROCm") !== -1

                Label {
                    color: palette.text
                    Layout.fillWidth: true
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    text: qsTr("Tip: If you observe problems with GPU acceleration, try to enable %1 option.")
                          .arg("<i>" + qsTr("Other") + "</i> &rarr; <i>" +
                                       qsTr("Override GPU version") + "</i>");
                }
            }

            CheckBox {
                checked: _settings.show_repair_text
                text: qsTr("Show %1 option").arg("<i>" + qsTr("Repair text") + "</i>")
                onCheckedChanged: {
                    _settings.show_repair_text = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Text repair always happens before Text to Speech processing, but with this option you can see what exactly is being sent to the engine.") + " " +
                              qsTr("For example, this can be useful when you want to check text after restoring diacritical marks.")
                hoverEnabled: true
            }

            SectionLabel {
                text: qsTr("Subtitles")
            }

            CheckBox {
                checked: _settings.tts_subtitles_sync
                text: qsTr("Sync speech with timestamps")
                onCheckedChanged: {
                    _settings.tts_subtitles_sync = checked
                }

                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("When reading or exporting subtitles to file, synchronize the generated speech with the subtitle timestamps.") + " " +
                              qsTr("This may be useful for creating voiceovers.")
                hoverEnabled: true
            }
        }

        ColumnLayout {
            id: accessibilityTab

            width: root.width

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

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Start listening")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_start_listening
                    onTextChanged: _settings.hotkey_start_listening = text
                    color: palette.text
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts && app.feature_text_active_window

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Start listening, text to active window")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_start_listening_active_window
                    onTextChanged: _settings.hotkey_start_listening_active_window = text
                    color: palette.text
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Start listening, text to clipboard")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_start_listening_clipboard
                    onTextChanged: _settings.hotkey_start_listening_clipboard = text
                    color: palette.text
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Stop listening")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_stop_listening
                    onTextChanged: _settings.hotkey_stop_listening = text
                    color: palette.text
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Start reading")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_start_reading
                    onTextChanged: _settings.hotkey_start_reading = text
                    color: palette.text
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Start reading text from clipboard")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_start_reading_clipboard
                    onTextChanged: _settings.hotkey_start_reading_clipboard = text
                    color: palette.text
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Pause/Resume reading")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_pause_resume_reading
                    onTextChanged: _settings.hotkey_pause_resume_reading = text
                    color: palette.text
                }
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 2
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding
                visible: _settings.hotkeys_enabled && app.feature_global_shortcuts

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: appWin.padding
                    text: qsTr("Cancel")
                }
                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                    Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                    text: _settings.hotkey_cancel
                    onTextChanged: _settings.hotkey_cancel = text
                    color: palette.text
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
                ToolTip.text: qsTr("Action allow external application to invoke certain operation when %1 is running.").arg("<i>Speech Note</i>")
                hoverEnabled: true
            }


            InlineMessage {
                color: palette.text
                Layout.leftMargin: appWin.padding
                Layout.fillWidth: true
                visible: _settings.actions_api_enabled

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: "<p>" + qsTr("Action allows external application to invoke certain operation when %1 is running.").arg("<i>Speech Note</i>") + " " +
                          qsTr("An action can be triggered via DBus call or with command-line option.") + " " +
                          qsTr("The following actions are currently supported:") +
                          "</p><ul>" +
                          "<li><i>start-listening</i> - " + qsTr("Starts listening.") + "</li>" +
                          "<li><i>start-listening-active-window</i> (X11) - " + qsTr("Starts listening. The decoded text is inserted into the active window.") + "</li>" +
                          "<li><i>start-listening-clipboard</i> - " + qsTr("Starts listening. The decoded text is copied to the clipboard.") + "</li>" +
                          "<li><i>stop-listening</i> - " + qsTr("Stops listening. The already captured voice is decoded into text.") + "</li>" +
                          "<li><i>start-reading</i> - " + qsTr("Starts reading.") + "</li>" +
                          "<li><i>start-reading-clipboard</i> - " + qsTr("Starts reading text from the clipboard.") + "</li>" +
                          "<li><i>pause-resume-reading</i> - " + qsTr("Pauses or resumes reading.") + "</li>" +
                          "<li><i>cancel</i> - " + qsTr("Cancels any of the above operations.") + "</li>" +
                          "</ul>" +
                          qsTr("For example, to trigger %1 action, execute the following command: %2.")
                                  .arg("<i>start-listening</i>")
                                  .arg(_settings.is_flatpak() ?
                                    "<i>flatpak run net.mkiol.SpeechNote --action start-listening</i>" :
                                    "<i>dsnote --action start-listening</i>")
                }
            }
        }

        ColumnLayout {
            id: otherTab

            width: root.width

            SectionLabel {
                text: qsTr("Storage")
            }

            GridLayout {
                columns: root.verticalMode ? 1 : 3
                columnSpacing: appWin.padding
                rowSpacing: appWin.padding

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Location of language files")
                }

                TextField {
                    Layout.fillWidth: verticalMode
                    Layout.preferredWidth: verticalMode ? grid.width : (grid.width / 2 - langFileButton.width - appWin.padding)
                    Layout.leftMargin: verticalMode ? appWin.padding : 0
                    text: _settings.models_dir
                    color: palette.text
                    readOnly: true

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Directory where language files are downloaded to and stored.")
                    hoverEnabled: true
                }

                Button {
                    id: langFileButton

                    text: qsTr("Change")
                    onClicked: directoryDialog.open()
                    Layout.leftMargin: verticalMode ? appWin.padding : 0

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Directory where language files are downloaded to and stored.")
                    hoverEnabled: true
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

            InlineMessage {
                color: "red"
                closable: true
                Layout.fillWidth: true
                visible: _settings.hint_addons
                onCloseClicked: _settings.hint_addons = false

                Label {
                    color: "red"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("The Flatpak add-on for GPU acceleration is not installed.")
                }

                Label {
                    color: "red"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("To enable GPU acceleration, install either %1 add-on for AMD graphics card or %2 add-on for NVIDIA graphics card.")
                        .arg("<i><b>net.mkiol.SpeechNote.Addon.amd</b></i>")
                        .arg("<i><b>net.mkiol.SpeechNote.Addon.nvidia</b></i>")
                }
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

                GridLayout {
                    columns: root.verticalMode ? 1 : 2
                    columnSpacing: appWin.padding
                    rowSpacing: appWin.padding

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Number of simultaneous threads")
                        wrapMode: Text.Wrap
                    }

                    SpinBox {
                        Layout.fillWidth: verticalMode
                        Layout.preferredWidth: verticalMode ? grid.width : grid.width / 2
                        Layout.leftMargin: verticalMode ? appWin.padding : 0
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
                            _settings.num_threads = value;
                        }
                        Component.onCompleted: {
                            contentItem.color = palette.text
                        }

                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set the maximum number of simultaneous CPU threads.")
                        hoverEnabled: true
                    }
                }

                SectionLabel {
                    visible: _settings.gpu_supported()
                    text: qsTr("Graphics card options")
                }

                CheckBox {
                    visible: _settings.gpu_supported()
                    checked: _settings.gpu_scan_cuda
                    text: qsTr("Use %1").arg("NVIDIA CUDA")
                    onCheckedChanged: {
                        _settings.gpu_scan_cuda = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find NVIDIA CUDA compatible graphic cards in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.gpu_supported()
                    checked: _settings.gpu_scan_hip
                    text: qsTr("Use %1").arg("AMD ROCm")
                    onCheckedChanged: {
                        _settings.gpu_scan_hip = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find AMD ROCm compatible graphic cards in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.gpu_supported()
                    checked: _settings.gpu_scan_opencl
                    text: qsTr("Use %1").arg("OpenCL")
                    onCheckedChanged: {
                        _settings.gpu_scan_opencl = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Try to find OpenCL compatible graphic cards in the system.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true
                }

                CheckBox {
                    visible: _settings.gpu_supported()
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

                GridLayout {
                    columns: root.verticalMode ? 1 : 3
                    columnSpacing: appWin.padding
                    rowSpacing: appWin.padding
                    visible: _settings.gpu_supported() && _settings.gpu_override_version

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: appWin.padding
                        text: qsTr("Version")
                    }
                    TextField {
                        Layout.fillWidth: verticalMode
                        Layout.preferredWidth: verticalMode ? grid.width : (grid.width / 2 - gpuOverrideResetButton.width - appWin.padding)
                        Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                        text: _settings.gpu_overrided_version
                        onTextChanged: _settings.gpu_overrided_version = text
                        color: palette.text

                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Value has the same effect as %1 environment variable.").arg("<i>HSA_OVERRIDE_GFX_VERSION</i>")
                        hoverEnabled: true
                    }
                    Button {
                        id: gpuOverrideResetButton

                        icon.name: "edit-undo-symbolic"
                        onClicked: _settings.gpu_overrided_version = ""
                        Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0

                        ToolTip.visible: hovered
                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.text: qsTr("Reset to default value.")
                        hoverEnabled: true
                    }
                }

                SectionLabel {
                    text: qsTr("Libraries")
                }

                CheckBox {
                    checked: _settings.py_feature_scan
                    text: qsTr("Check Python dependencies")
                    onCheckedChanged: {
                        _settings.py_feature_scan = checked
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Check the presence of the required Python libraries.") + " " +
                                  qsTr("Disable this option if you observe problems when launching the application.")
                    hoverEnabled: true

                }

                GridLayout {
                    visible: _settings.py_feature_scan && !_settings.is_flatpak()
                    columns: root.verticalMode ? 1 : 3
                    columnSpacing: appWin.padding
                    rowSpacing: appWin.padding

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Location of Python libraries")
                        Layout.leftMargin: appWin.padding
                    }

                    TextField {
                        id: pyTextField

                        Layout.fillWidth: verticalMode
                        Layout.preferredWidth: verticalMode ? grid.width : (grid.width / 2 - pySaveButton.width - appWin.padding)
                        Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                        text: _settings.py_path
                        color: palette.text
                        placeholderText: qsTr("Leave blank to use the default value.")

                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Python libraries directory (%1).").arg("<i>PYTHONPATH</i>") + " " + qsTr("Leave blank to use the default value.") + " " +
                                      qsTr("This option may be useful if you use %1 module to manage Python libraries.").arg("<i>venv</i>")
                        hoverEnabled: true
                    }

                    Button {
                        id: pySaveButton

                        text: qsTr("Save")
                        Layout.leftMargin: verticalMode ? 2 * appWin.padding : 0
                        onClicked: _settings.py_path = pyTextField.text

                        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Save changes")
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

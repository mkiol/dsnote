/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0

ColumnLayout {
    id: root

    readonly property alias opened: filterButton.checked
    property var models_model: service.models_model
    readonly property alias filtering_visible: filterButton.checked

    onOpenedChanged: reset()

    function open() {
        filterButton.checked = true
    }

    function close() {
        filterButton.checked = false
    }

    function reset() {
        modelsSearchTextField.text = ""
        modelType_checkBox.checked = false
        engine_checkBox.checked = false
        speed_checkBox.checked = false
        quality_checkBox.checked = false
        caps_checkBox.checked = false
        models_model.resetRoleFilterFlags()
        models_model.resetFeatureFilterFlags()
    }

    function update(index) {
        if (modelType_checkBox.checked) {
            switch(index) {
            case 0: root.models_model.roleFilterFlags = ModelsListModel.RoleStt; break
            case 1: root.models_model.roleFilterFlags = ModelsListModel.RoleTts; break
            case 2: root.models_model.roleFilterFlags = ModelsListModel.RoleMnt; break
            case 3: root.models_model.roleFilterFlags = ModelsListModel.RoleOther; break
            }
        } else {
            root.models_model.roleFilterFlags =  ModelsListModel.RoleDefault
        }

        if (engine_checkBox.checked) {
            switch(index) {
            case 0: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineSttApril; break
            case 1: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineSttDs; break
            case 2: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineSttFasterWhisper; break
            case 3: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineSttVosk; break
            case 4: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineSttWhisper; break
            case 5: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineTtsCoqui; break
            case 6: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineTtsEspeak; break
            case 7: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineTtsMimic3; break
            case 8: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineTtsPiper; break
            case 9: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineTtsRhvoice; break
            case 10: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineTtsWhisperSpeech; break
            case 11: root.models_model.featureFilterFlags = ModelsListModel.FeatureEngineOther; break
            }
        } else if (speed_checkBox.checked) {
            switch(index) {
            case 0: root.models_model.featureFilterFlags = ModelsListModel.FeatureFastProcessing; break
            case 1: root.models_model.featureFilterFlags = ModelsListModel.FeatureMediumProcessing; break
            case 2: root.models_model.featureFilterFlags = ModelsListModel.FeatureSlowProcessing; break
            }
        } else if (quality_checkBox.checked) {
            switch(index) {
            case 0: root.models_model.featureFilterFlags = ModelsListModel.FeatureQualityHigh; break
            case 1: root.models_model.featureFilterFlags = ModelsListModel.FeatureQualityMedium; break
            case 2: root.models_model.featureFilterFlags = ModelsListModel.FeatureQualityLow; break
            }
        } else if (caps_checkBox.checked) {
            switch(index) {
            case 0: root.models_model.featureFilterFlags = ModelsListModel.FeatureSttIntermediateResults; break
            case 1: root.models_model.featureFilterFlags = ModelsListModel.FeatureSttPunctuation; break
            case 2: root.models_model.featureFilterFlags = ModelsListModel.FeatureTtsVoiceCloning; break
            }
        } else {
            root.models_model.featureFilterFlags = ModelsListModel.FeatureDefault
        }
    }

    Connections {
        target: root.models_model
        onFilterChanged: modelsSearchTextField.text = root.models_model.filter
    }

    RowLayout {
        spacing: appWin.padding * 0.5

        Item {
            Layout.fillWidth: true
        }

        TextField {
            id: modelsSearchTextField

            text: root.models_model.filter
            placeholderText: qsTr("Type to search")
            verticalAlignment: Qt.AlignVCenter
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            Layout.preferredWidth: 2 * appWin.buttonWithIconWidth
            onTextChanged: root.models_model.filter = text.toLowerCase().trim()
            color: palette.text
            Accessible.name: qsTr("Model search")

            TextContextMenu {}
        }

        Button {
            id: modelsSearchClearButton

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            icon.name: "edit-clear-symbolic"
            text: qsTr("Clear text")
            display: AbstractButton.IconOnly
            onClicked: modelsSearchTextField.text = ""

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Clear text")
            hoverEnabled: true
        }

        Item {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            width: filterButton.width + 2
            height: filterButton.height + 2

            // Rectangle {
            //     anchors.fill: parent
            //     visible: root.models_model.featureFilterFlags !== ModelsListModel.FeatureDefault ||
            //              root.models_model.roleFilterFlags !== ModelsListModel.RoleDefault
            //     height: 1
            //     radius: 5
            //     width: filterButton.width
            //     color: filterButton.palette.highlight
            //     opacity: 0.5
            // }

            Button {
                id: filterButton

                flat: true
                icon.name: "filter-symbolic"
                text: qsTr("Filter options for models")
                display: AbstractButton.IconOnly
                checkable: true
                anchors.centerIn: parent
                implicitHeight: appWin.buttonHeightShort

                Accessible.description: checked ? qsTr("Hide filter options") : qsTr("Show filter options")
                Accessible.name: text

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: checked ? qsTr("Hide filter options") : qsTr("Show filter options")
                hoverEnabled: true
            }
        }
    }

    RowLayout {
        visible: root.filtering_visible

        Item {
            Layout.fillWidth: true
        }

        GridLayout {
            columns: 4

            readonly property int maxComboWidth: Math.max(modelType_combo.implicitWidth,
                                                          engine_combo.implicitWidth, speed_combo.implicitWidth,
                                                          quality_combo.implicitWidth, caps_combo.implicitWidth)

            // modelType

            CheckBox {
                id: modelType_checkBox

                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                checked: false

                onCheckStateChanged: {
                    if (checked) {
                        modelType_checkBox.checked = true
                        engine_checkBox.checked = false
                        speed_checkBox.checked = false
                        quality_checkBox.checked = false
                        caps_checkBox.checked = false
                    }

                    root.update(modelType_combo.currentIndex)
                }

                Accessible.name: qsTr("Filtering by model type")
            }

            Label {
                text: qsTr("Model type")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            ComboBox {
                id: modelType_combo

                enabled: modelType_checkBox.checked
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                Layout.preferredWidth: parent.maxComboWidth
                hoverEnabled: true

                model: [
                    qsTr("Speech to Text"),
                    qsTr("Text to Speech"),
                    qsTr("Translator"),
                    qsTr("Other")
                ]

                onCurrentIndexChanged: root.update(currentIndex)

                Accessible.name: qsTr("Model types")
            }

            Button {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                flat: true
                icon.name: "help-about-symbolic"
                implicitHeight: appWin.buttonHeightShort

                ToolTip.visible: hovered || down
                ToolTip.delay: 0
                ToolTip.text: qsTr("Filter by the model type.")
                hoverEnabled: true
            }

            // engine

            CheckBox {
                id: engine_checkBox

                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                checked: false

                onCheckStateChanged: {
                    if (checked) {
                        modelType_checkBox.checked = false
                        engine_checkBox.checked = true
                        speed_checkBox.checked = false
                        quality_checkBox.checked = false
                        caps_checkBox.checked = false
                    }

                    root.update(engine_combo.currentIndex)
                }

                Accessible.name: qsTr("Filtering by engine type")
            }

            Label {
                text: qsTr("Engine")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            ComboBox {
                id: engine_combo

                enabled: engine_checkBox.checked
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                Layout.preferredWidth: parent.maxComboWidth
                hoverEnabled: true

                model: [
                    "April-ASR STT",
                    "DeepSpeech STT",
                    "FasterWhisper STT",
                    "Vosk STT",
                    "WhisperCpp STT",
                    "Coqui TTS",
                    "eSpeak TTS",
                    "Mimic3 TTS",
                    "Piper TTS",
                    "RHVoice TTS",
                    "WhisperSpeech TTS",
                    qsTr("Other")
                ]

                onCurrentIndexChanged: root.update(currentIndex)

                Accessible.name: qsTr("Engine types")
            }

            Button {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                flat: true
                icon.name: "help-about-symbolic"
                implicitHeight: appWin.buttonHeightShort

                ToolTip.visible: hovered || down
                ToolTip.delay: 0
                ToolTip.text: qsTr("Filter by the engine type.")
                hoverEnabled: true
            }

            // speed

            CheckBox {
                id: speed_checkBox

                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                checked: false

                onCheckStateChanged: {
                    if (checked) {
                        modelType_checkBox.checked = false
                        engine_checkBox.checked = false
                        speed_checkBox.checked = true
                        quality_checkBox.checked = false
                        caps_checkBox.checked = false
                    }

                    root.update(speed_combo.currentIndex)
                }

                Accessible.name: qsTr("Filtering by speed")
            }

            Label {
                text: qsTr("Processing speed")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            ComboBox {
                id: speed_combo

                enabled: speed_checkBox.checked
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                Layout.preferredWidth: parent.maxComboWidth
                hoverEnabled: true

                model: [
                    qsTr("Fast"),
                    qsTr("Medium"),
                    qsTr("Slow")
                ]

                onCurrentIndexChanged: root.update(currentIndex)

                Accessible.name: qsTr("Speed levels")
            }

            Button {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                flat: true
                icon.name: "help-about-symbolic"
                implicitHeight: appWin.buttonHeightShort

                ToolTip.visible: hovered || down
                ToolTip.delay: 0
                ToolTip.text: qsTr("Filter by the processing power required by the model.") + " " +
                              qsTr("Fast model works well even on older hardware.") + " " +
                              qsTr("Slow model needs new CPU and sometimes works only when GPU acceleration is enabled.")
                hoverEnabled: true
            }

            // quality

            CheckBox {
                id: quality_checkBox

                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                checked: false

                onCheckStateChanged: {
                    if (checked) {
                        modelType_checkBox.checked = false
                        engine_checkBox.checked = false
                        speed_checkBox.checked = false
                        quality_checkBox.checked = true
                        caps_checkBox.checked = false
                    }

                    root.update(quality_combo.currentIndex)
                }

                Accessible.name: qsTr("Filtering by quality")
            }

            Label {
                text: qsTr("Quality")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            ComboBox {
                id: quality_combo

                enabled: quality_checkBox.checked
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                Layout.preferredWidth: parent.maxComboWidth
                hoverEnabled: true

                model: [
                    qsTr("High"),
                    qsTr("Medium"),
                    qsTr("Low")
                ]

                onCurrentIndexChanged: root.update(currentIndex)

                Accessible.name: qsTr("Quality levels")
            }

            Button {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                flat: true
                icon.name: "help-about-symbolic"
                implicitHeight: appWin.buttonHeightShort

                ToolTip.visible: hovered || down
                ToolTip.delay: 0
                ToolTip.text: qsTr("Filter by the quality of the output produced by the model.") + " " +
                              qsTr("In case of STT, it is the accuracy of speech recognition.") + " " +
                              qsTr("In case of TTS, it is the naturalness of synthesized voice.")
                hoverEnabled: true
            }

            // caps

            CheckBox {
                id: caps_checkBox

                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                checked: false

                onCheckStateChanged: {
                    if (checked) {
                        modelType_checkBox.checked = false
                        engine_checkBox.checked = false
                        speed_checkBox.checked = false
                        quality_checkBox.checked = false
                        caps_checkBox.checked = true
                    }

                    root.update(caps_combo.currentIndex)
                }

                Accessible.name: qsTr("Filtering by additional capabilities")
            }

            Label {
                text: qsTr("Additional capabilities")
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }

            ComboBox {
                id: caps_combo

                enabled: caps_checkBox.checked
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                Layout.preferredWidth: parent.maxComboWidth
                hoverEnabled: true

                model: [
                    qsTr("Intermediate Results") + " STT",
                    qsTr("Punctuation") + " STT",
                    qsTr("Voice Cloning") + " TTS",

                ]

                onCurrentIndexChanged: root.update(currentIndex)

                Accessible.name: qsTr("Additional capabilities")
            }

            Button {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                flat: true
                icon.name: "help-about-symbolic"
                implicitHeight: appWin.buttonHeightShort

                ToolTip.visible: hovered || down
                ToolTip.delay: 0
                ToolTip.text: qsTr("Filter by additional capabilities offered by model.") + " " +
                              qsTr("Select to show only models with selected capabilities.")
                hoverEnabled: true
            }
        }

    }
}

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

GridLayout {
    id: root

    rowSpacing: appWin.padding * 0.5
    columnSpacing: appWin.padding
    columns: 2
    height: visible ? implicitHeight : 0

    property var models_model: service.models_model

    function open() {
        filterButton.checked = true
    }

    function close() {
        filterButton.checked = false
    }

    function reset() {
        modelsSearchTextField.text = ""
        models_model.resetRoleFilterFlags()
        models_model.resetFeatureFilterFlags()
    }

    function update() {
        var stt_visible = (models_model.roleFilterFlags & ModelsListModel.RoleStt) > 0
        var tts_visible = (models_model.roleFilterFlags & ModelsListModel.RoleTts) > 0
        var mnt_visible = (models_model.roleFilterFlags & ModelsListModel.RoleMnt) > 0
        var other_visible = (models_model.roleFilterFlags & ModelsListModel.RoleOther) > 0

        sttFeature.checked = stt_visible
        ttsFeature.checked = tts_visible
        mntFeature.checked = mnt_visible
        otherFeature.checked = other_visible
        fastProcessingFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureFastProcessing
        mediumProcessingFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureMediumProcessing
        slowProcessingFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureSlowProcessing
        qualityHiFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureQualityHigh
        qualityMeFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureQualityMedium
        qualityLoFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureQualityLow
        sttIrFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureSttIntermediateResults
        sttPuFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureSttPunctuation
        ttsVcFeature.checked = models_model.featureFilterFlags & ModelsListModel.FeatureTtsVoiceCloning
        speedFilters.enabled = stt_visible || tts_visible
        qualityFilters.enabled = stt_visible || tts_visible
        additionalFeaturesFilters.enabled = stt_visible || tts_visible
        sttIrFeature.enabled = stt_visible && (models_model.disabledFeatureFilterFlags & ModelsListModel.FeatureSttIntermediateResults) === 0
        sttPuFeature.enabled = stt_visible && (models_model.disabledFeatureFilterFlags & ModelsListModel.FeatureSttPunctuation) === 0
        ttsVcFeature.enabled = tts_visible && (models_model.disabledFeatureFilterFlags & ModelsListModel.FeatureTtsVoiceCloning) === 0
    }

    Connections {
        target: root.models_model
        onRoleFilterFlagsChanged: root.update()
        onFeatureFilterFlagsChanged: root.update()
        onDisabledFeatureFilterFlagsChanged: root.update()
    }

    Component.onCompleted: {
        reset();
        update();
    }

    Label {
        text: qsTr("Model type")
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }

    RowLayout {
        id: roleFilters

        spacing: appWin.padding * 0.5

        ModelFeatureButton {
            id: sttFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Speech to Text")
            onToggled: {
                if (checked) root.models_model.addRoleFilterFlag(ModelsListModel.RoleStt)
                else root.models_model.removeRoleFilterFlag(ModelsListModel.RoleStt)
            }
        }
        ModelFeatureButton {
            id: ttsFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Text to Speech")
            onToggled: {
                if (checked) root.models_model.addRoleFilterFlag(ModelsListModel.RoleTts)
                else root.models_model.removeRoleFilterFlag(ModelsListModel.RoleTts)
            }
        }
        ModelFeatureButton {
            id: mntFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Translator")
            onToggled: {
                if (checked) root.models_model.addRoleFilterFlag(ModelsListModel.RoleMnt)
                else root.models_model.removeRoleFilterFlag(ModelsListModel.RoleMnt)
            }
        }
        ModelFeatureButton {
            id: otherFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Other")
            onToggled: {
                if (checked) root.models_model.addRoleFilterFlag(ModelsListModel.RoleOther)
                else root.models_model.removeRoleFilterFlag(ModelsListModel.RoleOther)
            }
        }

        Item {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            width: filterButton.width + 2
            height: filterButton.height + 2

            Rectangle {
                anchors.fill: parent
                visible: root.models_model.featureFilterFlags !== ModelsListModel.FeatureDefault
                height: 1
                radius: 5
                width: filterButton.width
                color: filterButton.palette.highlight
                opacity: 0.5
            }

            Button {
                id: filterButton

                flat: true
                icon.name: "filter-symbolic"
                checkable: true
                anchors.centerIn: parent
                implicitHeight: appWin.buttonHeightShort

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: checked ? qsTr("Hide filter options") : qsTr("Show filter options")
            }
        }
    }

    Label {
        visible: speedFilters.visible
        text: qsTr("Processing speed")
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }

    RowLayout {
        id: speedFilters

        visible: filterButton.checked
        spacing: appWin.padding * 0.5

        ModelFeatureButton {
            id: fastProcessingFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Fast")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureFastProcessing)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureFastProcessing)
            }
        }
        ModelFeatureButton {
            id: mediumProcessingFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Medium")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureMediumProcessing)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureMediumProcessing)
            }
        }
        ModelFeatureButton {
            id: slowProcessingFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Slow")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureSlowProcessing)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureSlowProcessing)
            }
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
        }
    }

    Label {
        visible: qualityFilters.visible
        text: qsTr("Quality")
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }

    RowLayout {
        id: qualityFilters

        visible: filterButton.checked
        spacing: appWin.padding * 0.5

        ModelFeatureButton {
            id: qualityHiFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("High")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureQualityHigh)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureQualityHigh)
            }
        }
        ModelFeatureButton {
            id: qualityMeFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Medium")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureQualityMedium)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureQualityMedium)
            }
        }
        ModelFeatureButton {
            id: qualityLoFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Low")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureQualityLow)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureQualityLow)
            }
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
                          qsTr("In case of TTS, it is the naturalness of speech synthesis.")
        }
    }

    Label {
        visible: additionalFeaturesFilters.visible
        text: qsTr("Additional capabilities")
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }

    RowLayout {
        id: additionalFeaturesFilters

        visible: filterButton.checked
        spacing: appWin.padding * 0.5

        ModelFeatureButton {
            id: sttIrFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Intermediate Results")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureSttIntermediateResults)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureSttIntermediateResults)
            }

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Speech recognition results are generated in near real time.")
        }
        ModelFeatureButton {
            id: sttPuFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Punctuation")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureSttPunctuation)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureSttPunctuation)
            }

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Speech decoding recognizes punctuation marks.")
        }
        ModelFeatureButton {
            id: ttsVcFeature

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            text: qsTr("Voice Cloning")
            onToggled: {
                if (checked) root.models_model.addFeatureFilterFlag(ModelsListModel.FeatureTtsVoiceCloning)
                else root.models_model.removeFeatureFilterFlag(ModelsListModel.FeatureTtsVoiceCloning)
            }

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("The model can imitate certain characteristics of another voice based on a voice sample.")
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
        }
    }

    Label {
        visible: nameFilter.visible
        text: qsTr("Name")
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }

    RowLayout {
        id: nameFilter

        spacing: appWin.padding * 0.5
        visible: filterButton.checked

        TextField {
            id: modelsSearchTextField

            text: listViewItem.model.filter
            placeholderText: qsTr("Type to search")
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            Layout.preferredWidth: Math.max(roleFilters.implicitWidth, additionalFeaturesFilters.implicitWidth)
                                   - modelsSearchClearButton.width - modelsSearchClearAllButton.width - appWin.padding
            onTextChanged: listViewItem.model.filter = text.toLowerCase().trim()
            color: palette.text
        }

        Button {
            id: modelsSearchClearButton

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            icon.name: "edit-clear-symbolic"
            onClicked: modelsSearchTextField.text = ""

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Clear text")
        }

        Button {
            id: modelsSearchClearAllButton

            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            icon.name: "edit-undo-symbolic"
            enabled: !root.models_model.defaultFilters || modelsSearchTextField.text.length !== 0
            onClicked: root.reset()

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Reset all filters to default values.")
        }
    }
}

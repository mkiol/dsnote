/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
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

    property bool use_gpu: false
    property var devices
    property int device_index: -1

    Layout.fillWidth: true

    onUse_gpuChanged: checkBox.checked = use_gpu

    CheckBox {
        id: checkBox

        checked: root.use_gpu
        text: qsTr("Use hardware acceleration")
        onCheckedChanged: {
            root.use_gpu = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTr("If a suitable hardware accelerator (CPU or graphics card) is found in the system, it will be used to speed up processing.") + " " +
                      qsTr("Hardware acceleration significantly reduces the time of decoding.") + " " +
                      qsTr("Disable this option if you observe problems.")
        hoverEnabled: true
    }

    TipMessage {
        indends: 1
        visible: root.enabled && root.use_gpu && root.devices.length <= 1
        text: qsTr("A suitable hardware accelerator could not be found.")
    }

    ComboBoxForm {
        id: gpuCombo

        indends: 1
        visible: root.enabled && root.use_gpu && root.devices.length > 1
        label.text: qsTr("Hardware accelerator")
        toolTip: qsTr("Select preferred hardware accelerator.")
        comboBox {
            currentIndex: root.device_index
            model: root.devices
            onActivated: {
                root.device_index = index
            }
        }
    }

    TipMessage {
        indends: 2
        color: palette.text
        visible: root.enabled && root.use_gpu && root.devices.length > 1 && gpuCombo.displayText.search("ROCm") !== -1
        text: qsTr("Tip: If you observe problems with hardware acceleration, try to enable %1 option.")
              .arg("<i>" + qsTr("Other") + "</i> &rarr; <i>" +
              qsTr("Override GPU version") + "</i>")
        label.textFormat: Text.RichText
    }

    TipMessage {
        indends: 2
        color: palette.text
        visible: root.enabled && root.use_gpu && root.devices.length > 1 && gpuCombo.displayText.search("OpenVINO") !== -1
        text: qsTr("Tip: OpenVINO acceleration is most effective when processing long sentences with large models.") + " " +
              qsTr("For short sentences, better results can be obtained without hardware acceleration enabled.")
    }

    TipMessage {
        indends: 1
        visible: root.enabled && root.use_gpu && ((_settings.error_flags & Settings.ErrorCudaUnknown) > 0)
        text: qsTr("Most likely, NVIDIA kernel module has not been fully initialized.") + " " +
              qsTr("Try executing %1 before running Speech Note.").arg("<i>\"nvidia-modprobe -c 0 -u\"</i>")
    }
}

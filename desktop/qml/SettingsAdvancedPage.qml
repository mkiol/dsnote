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

    property bool verticalMode: parent ? parent.verticalMode : false
    readonly property var features: app.features_availability()

    Connections {
        target: app
        onFeatures_availability_updated: {
            root.features = app.features_availability()
        }
    }

    SectionLabel {
        text: qsTranslate("SettingsPage", "Availability of optional features")
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

    SectionLabel {
        text: qsTranslate("SettingsPage", "CPU options")
    }

    SpinBoxForm {
        label.text: qsTranslate("SettingsPage", "Number of simultaneous threads")
        toolTip: qsTranslate("SettingsPage", "Set the maximum number of simultaneous CPU threads.")
        spinBox {
            from: 0
            to: 32
            stepSize: 1
            value: _settings.num_threads < 0 ? 0 : _settings.num_threads > 32 ? 32 : _settings.num_threads
            textFromValue: function(value) {
                return value < 1 ? qsTranslate("SettingsPage", "Auto") : value.toString()
            }
            valueFromText: function(text) {
                if (text === qsTranslate("SettingsPage", "Auto")) return 0
                return parseInt(text);
            }
            onValueChanged: {
                _settings.num_threads = spinBox.value;
            }
        }
    }

    SectionLabel {
        visible: _settings.hw_accel_supported()
        text: qsTranslate("SettingsPage", "Hardware acceleration options")
    }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.hw_scan_cuda
        text: qsTranslate("SettingsPage", "Use %1").arg("NVIDIA CUDA")
        onCheckedChanged: {
            _settings.hw_scan_cuda = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible graphic cards in the system.").arg("NVIDIA CUDA") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true
    }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.hw_scan_hip
        text: qsTranslate("SettingsPage", "Use %1").arg("AMD ROCm")
        onCheckedChanged: {
            _settings.hw_scan_hip = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible graphic cards in the system.").arg("AMD ROCm") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true
    }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.hw_scan_vulkan
        text: qsTranslate("SettingsPage", "Use %1").arg("Vulkan")
        onCheckedChanged: {
            _settings.hw_scan_vulkan = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible graphic cards in the system.").arg("Vulkan") + " " +
                      qsTranslate("SettingsPage", "Only dedicated graphics cards are included. To search for integrated graphics cards, also enable %1.")
                           .arg("<i>" + qsTranslate("SettingsPage", "Use %1 (Integrated GPU)").arg("Vulkan %1") + "</i>") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true
    }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.hw_scan_vulkan_igpu
        text: qsTranslate("SettingsPage", "Use %1").arg("Vulkan iGPU")
        onCheckedChanged: {
            _settings.hw_scan_vulkan_igpu = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible integrated graphic cards in the system.").arg("Vulkan") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true
    }

    // CheckBox {
    //     visible: _settings.hw_accel_supported()
    //     checked: _settings.hw_scan_vulkan_cpu
    //     text: qsTranslate("SettingsPage", "Use %1").arg("Vulkan CPU")
    //     onCheckedChanged: {
    //         _settings.hw_scan_vulkan_cpu = checked
    //     }

    //     ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    //     ToolTip.visible: hovered
    //     ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible CPU in the system.").arg("Vulkan") + " " +
    //                   qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
    //     hoverEnabled: true
    // }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.hw_scan_openvino
        text: qsTranslate("SettingsPage", "Use %1").arg("OpenVINO")
        onCheckedChanged: {
            _settings.hw_scan_openvino = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible hardware in the system.").arg("OpenVINO") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true
    }

    // CheckBox {
    //     visible: _settings.hw_accel_supported()
    //     checked: _settings.hw_scan_openvino_gpu
    //     text: qsTranslate("SettingsPage", "Use %1").arg("OpenVINO GPU")
    //     onCheckedChanged: {
    //         _settings.hw_scan_openvino_gpu = checked
    //     }

    //     ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    //     ToolTip.visible: hovered
    //     ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible graphic cards in the system.").arg("OpenVINO") + " " +
    //                   qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
    //     hoverEnabled: true
    // }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.hw_scan_opencl
        text: qsTranslate("SettingsPage", "Use %1").arg("OpenCL")
        onCheckedChanged: {
            _settings.hw_scan_opencl = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible graphic cards in the system.").arg("OpenCL") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true
    }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.hw_scan_opencl_legacy
        text: qsTranslate("SettingsPage", "Use %1").arg("OpenCL Clover")
        onCheckedChanged: {
            _settings.hw_scan_opencl_legacy = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Try to find %1 compatible graphic cards in the system.").arg("OpenCL Clover") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true
    }

    CheckBox {
        visible: _settings.hw_accel_supported()
        checked: _settings.gpu_override_version
        text: qsTranslate("SettingsPage", "Override GPU version")
        onCheckedChanged: {
            _settings.gpu_override_version = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Override AMD GPU version.") + " " +
                      qsTranslate("SettingsPage", "Enable this option if you observe problems when using GPU acceleration with AMD graphics card.")
        hoverEnabled: true
    }

    TextFieldForm {
        indends: 1
        visible: _settings.hw_accel_supported() && _settings.gpu_override_version
        label.text: qsTranslate("SettingsPage", "Version")
        toolTip: qsTranslate("SettingsPage", "Value has the same effect as %1 environment variable.").arg("<i>HSA_OVERRIDE_GFX_VERSION</i>")
        textField {
            text: _settings.gpu_overrided_version
            onTextChanged: _settings.gpu_overrided_version = text
        }
        button {
            text: qsTranslate("SettingsPage", "Reset")
            onClicked: _settings.gpu_overrided_version = ""
        }
    }

    SectionLabel {
        text: qsTranslate("SettingsPage", "Libraries")
    }

    CheckBox {
        checked: _settings.py_feature_scan
        text: qsTranslate("SettingsPage", "Use Python libriaries")
        onCheckedChanged: {
            _settings.py_feature_scan = checked
        }

        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.visible: hovered
        ToolTip.text: qsTranslate("SettingsPage", "Check the presence of the required Python libraries.") + " " +
                      qsTranslate("SettingsPage", "Disable this option if you observe problems when launching the application.")
        hoverEnabled: true

    }

    TextFieldForm {
        id: pyTextField

        indends: 1
        visible: _settings.py_feature_scan && !_settings.is_flatpak()
        label.text: qsTranslate("SettingsPage", "Location of Python libraries")
        toolTip: qsTranslate("SettingsPage", "Python libraries directory (%1).").arg("<i>PYTHONPATH</i>") + " " + qsTranslate("SettingsPage", "Leave blank to use the default value.") + " " +
                 qsTranslate("SettingsPage", "This option may be useful if you use %1 module to manage Python libraries.").arg("<i>venv</i>")
        textField {
            text: _settings.py_path
            placeholderText: qsTranslate("SettingsPage", "Leave blank to use the default value.")
        }
        button {
            text: qsTranslate("SettingsPage", "Save")
            onClicked: _settings.py_path = pyTextField.textField.text
        }
    }

    SectionLabel {
        visible: _settings.is_xcb()
        text: "X11"
    }

    ComboBoxForm {
        visible: _settings.is_xcb()
        label.text: qsTranslate("SettingsPage", "Fake keyboard type")
        toolTip: qsTranslate("SettingsPage", "Keystroke sending method in %1.").arg("<i>" + qsTranslate("SettingsPage", "Insert into active window") + "</i>")
        comboBox {
            currentIndex: {
                if (_settings.fake_keyboard_type === Settings.FakeKeyboardTypeNative) return 0
                if (_settings.fake_keyboard_type === Settings.FakeKeyboardTypeXdo) return 1
                return 0
            }
            model: [
                qsTranslate("SettingsPage", "Native"),
                "XDO"
            ]
            onActivated: {
                if (index === 1) {
                    _settings.fake_keyboard_type = Settings.FakeKeyboardTypeXdo
                } else {
                    _settings.fake_keyboard_type = Settings.FakeKeyboardTypeNative
                }
            }
        }
    }

    TextFieldForm {
        visible: _settings.is_xcb() && _settings.fake_keyboard_type === Settings.FakeKeyboardTypeNative
        label.text: qsTranslate("SettingsPage", "Compose file")
        toolTip: qsTranslate("SettingsPage", "X11 compose file used in %1.").arg("<i>" + qsTranslate("SettingsPage", "Insert into active window") + "</i>") + " " +
                 qsTranslate("SettingsPage", "Leave blank to use the default value.")
        textField {
            text: _settings.x11_compose_file
            onTextChanged: _settings.x11_compose_file = text
        }
    }

    Item {
        Layout.fillHeight: true
    }
}

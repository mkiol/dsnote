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

    Component.onCompleted: {
        app.update_feature_statuses()
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

            model: appWin.features

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

    ComboBoxForm {
        id: pyScanModeCombo

        label.text: qsTranslate("SettingsPage", "Detection of Python libraries")
        toolTip: qsTranslate("SettingsPage", "Determine how and whether Python libraries are detected when the application is launched.")
        comboBox {
            currentIndex: {
                if (_settings.py_scan_mode === Settings.PyScanOn) return 0
                if (_settings.py_scan_mode === Settings.PyScanOffAllEnabled) return 1
                if (_settings.py_scan_mode === Settings.PyScanOffAllDisabled) return 2
                return _settings.is_xcb() ? 1 : 2
            }
            model: [
                qsTranslate("SettingsPage", "On"),
                qsTranslate("SettingsPage", "Off (Assume all are available)"),
                qsTranslate("SettingsPage", "Off (Assume none are available)"),
            ]
            onActivated: {
                if (index === 0) {
                    _settings.py_scan_mode = Settings.PyScanOn
                } else if (index === 1) {
                    _settings.py_scan_mode = Settings.PyScanOffAllEnabled
                } else if (index === 2) {
                    _settings.py_scan_mode = Settings.PyScanOffAllDisabled
                } else {
                    _settings.py_scan_mode = Settings.PyScanOn;
                }
            }
        }
    }

    TextFieldForm {
        id: pyTextField

        indends: 1
        visible: _settings.py_scan_mode === Settings.PyScanOn || _settings.py_scan_mode === Settings.PyScanOffAllEnabled
        label.text: qsTranslate("SettingsPage", "Location of Python libraries (version: %1)").arg(app.py_version.length === 0 ? "0.0.0" : app.py_version)
        toolTip: qsTranslate("SettingsPage", "Python libraries directory (%1).").arg("<i>PYTHONPATH</i>") + " " +
                 qsTranslate("SettingsPage", "Leave blank to use the default value.") + " " +
                 (_settings.is_flatpak() ?
                      qsTranslate("SettingsPage", "Make sure that the Flatpak application has permissions to access the directory.") :
                      qsTranslate("SettingsPage", "This option may be useful if you use %1 module to manage Python libraries.").arg("<i>venv</i>"))
        textField {
            text: _settings.py_path
            onTextChanged: _settings.py_path = pyTextField.textField.text
            placeholderText: qsTranslate("SettingsPage", "Leave blank to use the default value.")
        }
    }

    SectionLabel {
        visible: app.feature_fake_keyboard || ydoMessage.visible
        text: qsTranslate("SettingsPage", "Insert into active window")
    }

    ComboBoxForm {
        id: fakeKeyboardCombo

        visible: app.feature_fake_keyboard && _settings.is_xcb()
        label.text: qsTranslate("SettingsPage", "Keystroke sending method")
        toolTip: qsTranslate("SettingsPage", "Simulated keystroke sending method used in %1.")
                    .arg("<i>" + qsTranslate("SettingsPage", "Insert into active window") + "</i>")
        comboBox {
            currentIndex: {
                if (_settings.fake_keyboard_type === Settings.FakeKeyboardTypeLegacy) return 0
                if (_settings.fake_keyboard_type === Settings.FakeKeyboardTypeXdo) return 1
                if (_settings.fake_keyboard_type === Settings.FakeKeyboardTypeYdo) return 2
                return _settings.is_xcb() ? 1 : 2
            }
            model: [
                qsTranslate("SettingsPage", "Legacy"),
                "XDO",
                "YDO"
            ]
            onActivated: {
                if (index === 0) {
                    _settings.fake_keyboard_type = Settings.FakeKeyboardTypeLegacy
                } else if (index === 1) {
                    _settings.fake_keyboard_type = Settings.FakeKeyboardTypeXdo
                } else if (index === 2) {
                    _settings.fake_keyboard_type = Settings.FakeKeyboardTypeYdo
                } else {
                    _settings.fake_keyboard_type = _settings.is_xcb() ? Settings.FakeKeyboardTypeXdo : Settings.FakeKeyboardTypeYdo;
                }
            }
        }
    }

    SpinBoxForm {
        visible: app.feature_fake_keyboard && _settings.fake_keyboard_type !== Settings.FakeKeyboardTypeXdo
        label.text: qsTranslate("SettingsPage", "Keystroke delay")
        toolTip: qsTranslate("SettingsPage", "The delay between simulated keystrokes used in %1.").arg("<i>" + qsTranslate("SettingsPage", "Insert into active window") + "</i>")
        spinBox {
            from: 1
            to: 1000
            stepSize: 1
            value: _settings.fake_keyboard_delay
            onValueChanged: {
                _settings.fake_keyboard_delay = spinBox.value;
            }
        }
    }

    TextFieldForm {
        visible: app.feature_fake_keyboard && _settings.fake_keyboard_type !== Settings.FakeKeyboardTypeXdo
        label.text: qsTranslate("SettingsPage", "Compose file")
        toolTip: qsTranslate("SettingsPage", "X11 compose file used in %1.").arg("<i>" + qsTranslate("SettingsPage", "Insert into active window") + "</i>") + " " +
                 qsTranslate("SettingsPage", "Leave blank to use the default value.") +
                 (_settings.is_flatpak() ?
                      (" " + qsTranslate("SettingsPage", "Make sure that the Flatpak application has permissions to access the directory.")) :
                      "")
        textField {
            text: _settings.x11_compose_file
            onTextChanged: _settings.x11_compose_file = text
            placeholderText: qsTranslate("SettingsPage", "Leave blank to use the default value.")
        }
    }

    TextFieldForm {
        visible: app.feature_fake_keyboard && _settings.fake_keyboard_type === Settings.FakeKeyboardTypeYdo
        label.text: qsTranslate("SettingsPage", "Keyboard layout")
        toolTip: qsTranslate("SettingsPage", "Keyboard layout used in %1.").arg("<i>" + qsTranslate("SettingsPage", "Insert into active window") + "</i>") + " " +
                 qsTranslate("SettingsPage", "Leave blank to use the default value.")
        textField {
            text: _settings.fake_keyboard_layout
            onTextChanged: _settings.fake_keyboard_layout = text
            placeholderText: qsTranslate("SettingsPage", "Leave blank to use the default value.")
        }
    }

    TipMessage {
        id: ydoMessage

        color: "red"
        indends: 1
        visible: _settings.fake_keyboard_type === Settings.FakeKeyboardTypeYdo &&
                 !app.feature_fake_keyboard_ydo
        text: qsTranslate("SettingsPage", "Unable to connect to %1 daemon.")
                .arg("<i>ydotool</i>") + " " +
              qsTranslate("SettingsPage", "For %1 action to work, %2 daemon must be installed and running.")
                .arg("<i>start-listening-active-window</i>")
                .arg("<i>ydotool</i>") + (_settings.is_flatpak() ? (" " +
              qsTranslate("SettingsPage", "Also make sure that the Flatpak application has permission to access %1 daemon socket file.")
                        .arg("<i>ydotool</i>")) : "")
    }

    SectionLabel {
        visible: hotkeysCombo.visible
        text: qsTranslate("SettingsPage", "Other options")
    }

    ComboBoxForm {
        id: hotkeysCombo

        visible: app.feature_hotkeys && app.feature_hotkeys_portal && _settings.is_xcb()
        label.text: qsTranslate("SettingsPage", "Global keyboard shortcuts method")
        toolTip: qsTranslate("SettingsPage", "Method used to set global keyboard shortcuts.")
        comboBox {
            currentIndex: {
                if (_settings.hotkeys_type === Settings.HotkeysTypeX11) return 0
                if (_settings.hotkeys_type === Settings.HotkeysTypePortal) return 1
                return _settings.is_xcb() ? 0 : 1
            }
            model: [
                "X11",
                "XDG Desktop Portal"
            ]
            onActivated: {
                if (index === 0) {
                    _settings.hotkeys_type = Settings.HotkeysTypeX11
                } else if (index === 1) {
                    _settings.hotkeys_type = Settings.HotkeysTypePortal
                }
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }
}

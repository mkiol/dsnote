/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SpinBoxForm {
    id: root

    property string optionName: ""
    property alias toolTipText: root.toolTip
    property alias text: root.label.text
    property string unitName: ""
    property bool floatType: false
    property bool showReset: typeof _settings["reset_" + root.optionName] === "function"

    spinBox {
        from: typeof _settings["min_" + root.optionName] === "function" ? _settings["min_" + root.optionName]() : 0
        to: typeof _settings["max_" + root.optionName] === "function" ? _settings["max_" + root.optionName]() : 100
        stepSize: typeof _settings["step_" + root.optionName] === "function" ? _settings["step_" + root.optionName]() : 1
        value: _settings[root.optionName] ? _settings[root.optionName] : 0
        editable: true
        textFromValue: function(value) { 
            return root.unitName.length > 0 ? (value + " " + root.unitName) : value
        }
        valueFromText: function(text) { 
            return root.floatType ? parseFloat(text) : parseInt(text) 
        }
        onValueModified: {
            _settings[root.optionName] = spinBox.value;
        }
    }
    button {
        visible: root.showReset
        icon.name: "edit-reset-symbolic"
        display: root.verticalMode ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
        text: qsTranslate("SettingsPage", "Reset")
        onClicked: {
            if (typeof _settings["reset_" + root.optionName] === "function") {
                _settings["reset_" + root.optionName]()
            }
        }
    }
}
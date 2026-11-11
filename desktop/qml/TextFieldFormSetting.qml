/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

TextFieldForm {
    id: root

    property string optionName: ""
    property alias labelText: root.label.text
    property alias toolTipText: root.toolTip
    property alias placeholderText: root.textField.placeholderText
    property bool showReset: typeof _settings["reset_" + root.optionName] === "function"

    textField {
        text: _settings[root.optionName] ? _settings[root.optionName] : ""
        onTextChanged: _settings[root.optionName] = text
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

/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls

CheckBox {
    id: root

    property string optionName: ""
    property string toolTipText: ""

    checked: _settings[optionName]

    onCheckedChanged: {
        _settings[optionName] = checked
    }

    // Component.onCompleted: {
    //     checked = Qt.binding(function() { return _settings[optionName] })
    // }

    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
    ToolTip.visible: hovered
    ToolTip.text: root.toolTipText
    hoverEnabled: root.toolTipText.length > 0
}

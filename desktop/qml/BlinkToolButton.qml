/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15

ToolButton {
    id: root

    property bool blink: false
    property bool origChecked
    property alias toolTip: toolTip

    function checkedChangedHandler() {
        checked = origChecked;
    }

    function start() {
        origCheckedChanged.connect(checkedChangedHandler)
        checked = origChecked
    }

    function end() {
        origCheckedChanged.disconnect(checkedChangedHandler)
        checked = origChecked
    }

    Component.onCompleted: start()

    Timer {
        id: timer

        interval: 700
        repeat: true
        running: !root.hovered && root.blink
        onTriggered: {
            root.checked = !root.checked
        }

        onRunningChanged: {
            if (running) root.start()
            else root.end()
        }
    }

    ToolTip {
        id: toolTip
        visible: text.length !== 0 && (root.blink || hovered)
        delay: root.blink ? 0 : Qt.styleHints.mousePressAndHoldInterval
    }
}

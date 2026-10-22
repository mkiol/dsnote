/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Grid {
    id: root

    property bool verticalMode: false
    property bool expanded: false
    property alias first: _first
    property alias second: _second
    property double itemHeight: {
        if (!visible) return 0;
        if (verticalMode) {
            if (expanded) return first.comboItemHeight + second.comboItemHeight
            return first.comboItemHeight
        }
        return Math.max(first.comboItemHeight, second.comboItemHeight)
    }

    columns: verticalMode ? 1 : 2

    Item {
        width: verticalMode ? root.width : root.width / 2
        height: _first.height

        ComboButton {
            id: _first

            width: parent.width
            rightMargin: verticalMode ? Theme.horizontalPageMargin : 0
        }

        IconButton {
            id: _iconButton

            icon.source: (root.expanded ? "image://theme/icon-m-up?" : "image://theme/icon-m-down?") +
                         (pressed ? Theme.highlightColor : Theme.primaryColor)
            onClicked: root.expanded = !root.expanded
            anchors.right: parent.right
        }
    }

    ComboButton {
        id: _second

        visible: root.expanded || !root.verticalMode
        height: visible ? implicitHeight : 0.0
        width: verticalMode ? root.width : root.width / 2
        clip: true
        Behavior on height { NumberAnimation { duration: 100 } }
    }
}

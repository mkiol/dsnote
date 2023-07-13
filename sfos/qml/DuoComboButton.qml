/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
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
    property alias first: _first
    property alias second: _second
    property double itemHeight: !visible ? 0 : verticalMode ? first.comboItemHeight + second.comboItemHeight :
                                               Math.max(first.comboItemHeight, second.comboItemHeight)

    columns: verticalMode ? 1 : 2

    ComboButton {
        id: _first

        width: verticalMode ? root.width : root.width / 2
        rightMargin: verticalMode ? Theme.horizontalPageMargin : 0
    }

    ComboButton {
        id: _second

        width: verticalMode ? root.width : root.width / 2
    }
}

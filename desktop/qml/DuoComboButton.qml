/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

GridLayout {
    id: root

    property bool verticalMode: false
    property alias first: _first
    property alias second: _second

    columns: verticalMode ? 1 : 2
    columnSpacing: 0
    rowSpacing: 0

    ComboButton {
        id: _first
        verticalMode: root.verticalMode
    }

    ComboButton {
        id: _second
        verticalMode: root.verticalMode
    }
}


/* Copyright (C) 2017-2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

BusyIndicator {
    id: root
    size: BusyIndicatorSize.Medium

    property double progress: -1.0

    Label {
        visible: progress >= 0.0
        color: parent.color
        anchors.centerIn: parent
        font.pixelSize: {
            switch(size) {
            case BusyIndicatorSize.ExtraSmall:
            case BusyIndicatorSize.Small:
                return Theme.fontSizeExtraSmall
            case BusyIndicatorSize.Medium:
                return Theme.fontSizeTiny
            case BusyIndicatorSize.Large:
                return Theme.fontSizeMedium
            }
        }
        text:  Math.round(progress * 100) + "%"
    }
}


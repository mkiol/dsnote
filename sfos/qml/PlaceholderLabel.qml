/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Label {
    id: root

    opacity: enabled ? 1.0 : 0.0
    visible: opacity > 0.0
    Behavior on opacity { FadeAnimator { duration: 150 } }
    font.pixelSize: Theme.fontSizeLarge
    color: Theme.secondaryHighlightColor
    wrapMode: Text.Wrap
    horizontalAlignment: Text.AlignHCenter
    textFormat: Text.StyledText
    anchors {
        left: parent.left
        right: parent.right
        leftMargin: Theme.paddingLarge
        rightMargin: Theme.paddingLarge
        verticalCenter: parent.verticalCenter
    }
}

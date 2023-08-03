/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    id: root

    property double maxWidth: parent.width - 2 * Theme.horizontalPageMargin

    width: label.width + 2 * Theme.paddingLarge
    color: Theme.highlightDimmerColor
    height: label.height + 2 * Theme.paddingLarge
    radius: 10
    visible: opacity > 0.0
    opacity: timer.running ? 1.0 : 0.0
    Behavior on opacity { FadeAnimator {} }

    Timer {
        id: timer

        interval: 5000
    }

    Label {
        id: label

        x: Theme.paddingLarge
        y: Theme.paddingLarge
        width: Math.min(implicitWidth, root.maxWidth - 2 * Theme.paddingLarge)
        font.pixelSize: Theme.fontSizeSmall
        color: mouse.pressed ? Theme.highlightColor : Theme.primaryColor
        wrapMode: Text.Wrap
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        onClicked: timer.stop()
    }

    function show(summary) {
        label.text = summary
        timer.start()
    }
}

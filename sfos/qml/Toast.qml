/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    id: root
    color: Theme.overlayBackgroundColor
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: label.height + 2 * Theme.paddingSmall

    visible: opacity > 0.0
    opacity: timer.running ? 1.0 : 0.0
    Behavior on opacity { FadeAnimator { duration: 150 } }

    Timer {
        id: timer
        interval: 5000
    }

    Label {
        id: label
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.paddingLarge
        anchors.rightMargin: Theme.paddingLarge
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.primaryColor
        wrapMode: Text.WordWrap
    }

    MouseArea {
        anchors.fill: parent
        onClicked: timer.stop()
    }

    function show(summary) {
        label.text = summary
        timer.start()
    }
}

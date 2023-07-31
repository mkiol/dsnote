/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property string text: ""
    property string textPlaceholder: ""
    property alias status: indicator.status
    property alias busy: busyIndicator.running
    property alias progress: busyIndicator.progress
    property alias canCancel: cancelButton.visible

    signal cancelClicked()

    readonly property bool _empty: text.length === 0

    height: intermediateLabel.height + 2 * Theme.paddingLarge

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 1.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.05) }
            GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.10) }
        }
    }

    SpeechIndicator {
        id: indicator

        anchors {
            topMargin: Theme.paddingLarge
            top: parent.top
        }

        x: !root._empty || textPlaceholder.length !== 0  ?
               Theme.paddingSmall : (parent.width - width) / 2
        Behavior on x { NumberAnimation { duration: 100; easing: Easing.InOutQuad } }

        width: Theme.itemSizeSmall
        color: Theme.highlightColor

        // status values:
        // 0 - idle
        // 1 - speech detected,
        // 2 - processing
        // 3 - initializing,
        // 4 - speech playing
        status: 0

        off: false
        visible: opacity > 0.0
        opacity: busyIndicator.running ? 0.0 : 1.0
        Behavior on opacity { FadeAnimator { duration: 100 } }
    }

    BusyIndicatorWithProgress {
        id: busyIndicator

        color: Theme.highlightColor
        size: BusyIndicatorSize.Medium
        anchors.centerIn: indicator
        running: false
        _forceAnimation: true
    }

    Label {
        id: intermediateLabel

        anchors.topMargin: Theme.paddingLarge
        anchors.top: parent.top
        anchors.left: indicator.right
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin + (root.canCancel ? cancelButton.width : 0)
        anchors.leftMargin: Theme.paddingMedium * 0.7
        text: root._empty ? root.textPlaceholder : root.text
        wrapMode: root._empty ? Text.NoWrap : Text.Wrap
        truncationMode: _empty ? TruncationMode.Fade : TruncationMode.None
        color: root._empty ? Theme.secondaryHighlightColor : Theme.highlightColor
        font.italic: root._empty
    }

    IconButton {
        id: cancelButton

        anchors.right: parent.right
        anchors.rightMargin: Theme.paddingSmall
        anchors.top: parent.top
        icon.source: "image://theme/icon-m-cancel?" + (pressed ? Theme.highlightColor : Theme.primaryColor)
        onClicked: root.cancelClicked()
    }
}

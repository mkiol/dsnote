/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Dsnote 1.0

Item {
    id: root

    property string text: ""
    property string textPlaceholder: ""
    property alias status: indicator.status
    property alias busy: busyIndicator.running
    property alias progress: busyIndicator.progress
    property alias canCancel: cancelButton.visible
    property alias canStop: stopButton.visible
    property alias canPause: pauseButton.visible

    signal cancelClicked()
    signal pauseClicked()
    signal stopClicked()
    signal resumeClicked()

    readonly property bool _empty: text.length === 0

    height: Math.max(intermediateLabel.height + 2 * Theme.paddingLarge,
                     (canPause ? pauseButton.height : 0) +
                     (canStop ? stopButton.height : 0) +
                     (canCancel ? cancelButton.height : 0))

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
        Behavior on x { NumberAnimation { duration: 100; easing.type: Easing.InOutQuad } }

        width: Theme.itemSizeSmall
        color: Theme.highlightColor

        // status values:
        // 0 - idle
        // 1 - speech detected,
        // 2 - processing
        // 3 - initializing,
        // 4 - speech playing
        // 5 - speech paused
        status: 0

        off: false
        visible: !busyIndicator.running
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

    onStatusChanged: {
        if (root.status === 5) blinkAnimation.start()
        else {
            blinkAnimation.stop()
            pauseButton.opacity = 1.0
        }
    }

    SequentialAnimation {
        id: blinkAnimation

        loops: Animation.Infinite

        FadeAnimator {
            target: pauseButton
            from: 1.0
            to: 0.0
            duration: 500
        }
        FadeAnimator {
            target: pauseButton
            from: 0.0
            to: 1.0
            duration: 500
        }
    }

    IconButton {
        id: pauseButton

        anchors.right: parent.right
        anchors.rightMargin: Theme.paddingSmall
        anchors.top: parent.top
        icon.source: (root.status == 5 ? "image://theme/icon-m-play?" : "image://theme/icon-m-pause?") +
                     (pressed ? Theme.highlightColor : Theme.primaryColor)
        onClicked: {
            if (root.status == 5) root.resumeClicked()
            else root.pauseClicked()
        }
    }

    IconButton {
        id: stopButton

        anchors.right: parent.right
        anchors.rightMargin: Theme.paddingSmall
        anchors.top: parent.top
        icon.source: "image://theme/icon-m-stop?" +
                     (pressed ? Theme.highlightColor : Theme.primaryColor)
        onClicked: root.stopClicked()
    }

    IconButton {
        id: cancelButton

        anchors.right: parent.right
        anchors.rightMargin: Theme.paddingSmall
        anchors.top: pauseButton.visible ? pauseButton.bottom :
                     stopButton.visible ? stopButton.bottom : parent.top
        icon.source: "image://theme/icon-m-cancel?"
                          + (pressed ? Theme.highlightColor : Theme.primaryColor)
        onClicked: root.cancelClicked()
    }
}

/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

SilicaItem {
    id: root

    property string text: ""
    property string textPlaceholder: ""
    property string textPlaceholderActive: ""
    property string textPlaceholderNotConfigured: ""
    property string textPlaceholderBusy: ""
    property alias active: indicator.active
    property bool configured: false
    property alias busy: busyIndicator.running
    readonly property alias down: mouse.pressed
    property alias progress: busyIndicator.progress
    signal pressed()
    signal released()

    readonly property bool _active: highlighted
    readonly property color _pColor: _active ? Theme.highlightColor : Theme.primaryColor
    readonly property color _sColor: _active ? Theme.secondaryHighlightColor : Theme.secondaryColor
    readonly property bool _empty: text.length === 0

    height: intermediateLabel.height + 2 * Theme.paddingLarge
    highlighted: mouse.pressed || !mouse.enabled

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 1.0; color: Theme.rgba(root.palette.highlightBackgroundColor, 0.05) }
            GradientStop { position: 0.0; color: Theme.rgba(root.palette.highlightBackgroundColor, 0.10) }
        }
    }

    SpeechIndicator {
        id: indicator
        anchors.topMargin: Theme.paddingLarge
        anchors.top: parent.top
        anchors.leftMargin: Theme.paddingSmall
        anchors.left: parent.left
        width: Theme.itemSizeSmall
        color: root._pColor
        off: !root.configured
        active: false
        visible: opacity > 0.0
        opacity: busyIndicator.running ? 0.0 : 1.0
        Behavior on opacity { NumberAnimation { duration: 150 } }

    }

    BusyIndicatorWithProgress {
        id: busyIndicator
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
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.leftMargin: Theme.paddingMedium * 0.7
        text: {
            if (root._empty) {
                if (root.configured) {
                    if (root.busy) return root.textPlaceholderBusy
                    return root._active ? root.textPlaceholderActive : root.textPlaceholder
                } else {
                    return root.textPlaceholderNotConfigured
                }
            }
            return root.text
        }
        wrapMode: root._empty ? Text.NoWrap : Text.WordWrap
        truncationMode: _empty ? TruncationMode.Fade : TruncationMode.None
        color: root._empty ? root._sColor : root._pColor
        font.italic: root._empty
    }

    MouseArea {
        id: mouse
        enabled: root.enabled && root.configured && !root.busy
        anchors.fill: parent
        onPressedChanged: {
            if (pressed) root.pressed()
            else root.released()
        }
    }
}

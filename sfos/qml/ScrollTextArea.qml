/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

SilicaItem {
    id: root

    property alias textArea: _textArea
    property alias flick: _flick
    property alias showTopBorder: topBorder.visible
    property alias showBottomBorder: bottomBorder.visible
    property bool canUndo: false
    property bool canRedo: false
    property bool canClear: true

    signal clearClicked()
    signal copyClicked()
    signal undoClicked()

    function scrollToBottom() {
        if (!flick.atYEnd && flick.contentHeight > 0) {
            flick.contentY = flick.contentHeight - flick.height
            flick.returnToBounds()
        }
    }

    Rectangle {
        visible: root.highlighted
        anchors.fill: parent
        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
        opacity: Theme.opacityLow
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.textArea.forceActiveFocus()
    }

    Flickable {
        id: _flick

        y: topBorder.height + Theme.paddingMedium
        width: parent.width
        height: Math.min(parent.height - bottomBorder.height - topBorder.height - Theme.paddingMedium, root.textArea.height)
        contentHeight: root.textArea.height
        contentWidth: width
        clip: true

        TextArea {
            id: _textArea

            enabled: root.enabled
            width: parent.width
            background: null
            labelComponent: null
            opacity: root.enabled ? 1.0 : 0.8
            Behavior on opacity { FadeAnimator { duration: 100 } }
        }

        VerticalScrollDecorator {
            flickable: root.flick
            opacity: 1.0
        }
    }

    Row {
        opacity: root.textArea.focus ? 0.0 :
                 root.enabled && (flick.moving || copyButton.pressed || clearButton.pressed || undoButton.pressed ||
                 flick.contentHeight <= (root.height - Theme.itemSizeSmall)) ? 1.0 : 0.4
        Behavior on opacity { FadeAnimator { duration: 100 } }
        visible: opacity > 0.0
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        height: Theme.itemSizeSmall + Theme.paddingMedium

        IconButton {
            id: copyButton

            visible: root.textArea.text.length !== 0
            icon.source: "image://theme/icon-m-clipboard?" + (pressed ? Theme.highlightColor : Theme.primaryColor)
            onClicked: root.copyClicked()
        }
        IconButton {
            id: clearButton

            visible: root.canClear && !textArea.readOnly && textArea.text.length !== 0
            icon.source: "image://theme/icon-m-clear?" + (pressed ? Theme.highlightColor : Theme.primaryColor)
            onClicked: root.clearClicked()
        }
        IconButton {
            id: undoButton

            visible: !root.textArea.readOnly && (root.canUndo || root.canRedo)
            icon.source: (root.canUndo ? "image://theme/icon-m-rotate-left?" : "image://theme/icon-m-rotate-right?") +
                         (pressed ? Theme.highlightColor : Theme.primaryColor)
            onClicked: root.undoClicked()
        }
    }

    Rectangle {
        id: bottomBorder

        visible: false
        width: parent.width
        height: visible ? 2 : 0
        opacity: Theme.opacityLow
        color: Theme.highlightColor
        anchors.bottom: parent.bottom
    }

    Rectangle {
        id: topBorder

        visible: false
        width: parent.width
        height: visible ? 2 : 0
        opacity: Theme.opacityLow
        color: Theme.highlightColor
        anchors.top: parent.top
    }

    Component.onCompleted: scrollToBottom()
}

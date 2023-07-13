/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property alias button: _button
    property alias combo: _combo
    property alias comboModel: _comboRepeater.model
    property double rightMargin: Theme.horizontalPageMargin
    property alias comboPlaceholderText: _comboPlaceholder.text
    property double expandedHeight: 0
    readonly property bool off: comboModel.length === 0
    readonly property double comboItemHeight: !off && combo.contentItem ? combo.contentItem.height : Theme.itemSizeSmall

    implicitHeight: off ? Theme.itemSizeSmall : combo.height
    height: implicitHeight
    enabled: !root.off

    Item {
        width: parent.width
        height: parent.height - expandedHeight

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 1.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.05) }
                GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.10) }
            }
        }

        ComboBox {
            id: _combo

            visible: !root.off
            enabled: !root.off && root.enabled
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - (_button.visible ? (_button.width + root.rightMargin) : 0)
            opacity: enabled ? 1.0 : Theme.opacityOverlay
            valueColor: enabled ? Theme.highlightColor : Theme.secondaryHighlightColor
            menu: ContextMenu {
                Repeater {
                    id: _comboRepeater
                    MenuItem { text: modelData }
                }
            }
        }

        Label {
            id: _comboPlaceholder

            visible: root.off
            anchors.left: parent.left
            anchors.leftMargin: Theme.horizontalPageMargin
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - (_button.visible ? _button.width : 0)
                   - root.rightMargin - Theme.horizontalPageMargin
            opacity: Theme.opacityOverlay
            color: Theme.secondaryColor
            truncationMode: TruncationMode.Fade
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }

        Button {
            id: _button

            visible: text.length !== 0
            enabled: !root.off && root.enabled
            anchors.right: parent.right
            anchors.rightMargin: root.rightMargin
            height: root.combo.menu ? root.height - root.combo.menu.height : root.comboItemHeight
            preferredWidth: Theme.buttonWidthExtraSmall
        }
    }
}

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

Item {
    id: root

    property alias textArea: _textArea
    property color textColor: palette.text
    property bool canUndo: true
    property bool canUndoFallback: false
    property bool canRedo: true
    property bool canClear: true
    property bool canPaste: true
    readonly property bool _fitContent: scrollView.availableHeight - 2 * appWin.padding >= scrollView.contentHeight
    readonly property bool _contextActive: copyButton.hovered || clearButton.hovered ||
                                           undoButton.hovered || redoButton.hovered || pasteButton.hovered ||
                                           _fitContent

    signal clearClicked()
    signal copyClicked()
    signal undoFallbackClicked()

    function scrollToBottom() {
        var position = (scrollView.contentHeight - scrollView.availableHeight) / scrollView.contentHeight
        if (position > 0)
            scrollView._bar.position = position
    }

    ScrollView {
        id: scrollView

        property ScrollBar _bar: ScrollBar.vertical

        anchors.fill: parent
        enabled: root.enabled
        clip: true

        opacity: enabled ? 1.0 : 0.8
        Behavior on opacity { OpacityAnimator { duration: 100 } }

        TextArea {
            id: _textArea

            enabled: root.enabled
            color: {
                if (root._contextActive && !root._fitContent) {
                    var c = root.textColor
                    return Qt.rgba(c.r, c.g, c.b, 0.4)
                }
                return root.textColor
            }
            wrapMode: TextEdit.WordWrap
            verticalAlignment: TextEdit.AlignTop
            font.pixelSize: _settings.font_size

            Keys.onUpPressed: _bar.decrease()
            Keys.onDownPressed: _bar.increase()
        }
    }

    RowLayout {
        opacity: root.enabled && root._contextActive ? 1.0 : 0.4
        Behavior on opacity { OpacityAnimator { duration: 100 } }
        visible: opacity > 0.0

        anchors {
            bottom: parent.bottom
            bottomMargin: appWin.padding
            right: parent.right
            rightMargin: appWin.padding + (scrollView._bar.visible ? scrollView._bar.width : 0)
        }

        ToolButton {
            id: copyButton

            icon.name: "edit-copy-symbolic"
            onClicked: root.copyClicked()
            visible: root.textArea.text.length !== 0

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Copy")
        }
        ToolButton {
            id: pasteButton

            icon.name: "edit-paste-symbolic"
            onClicked: root.textArea.paste()
            visible: root.canPaste && root.textArea.canPaste

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Paste")
        }
        ToolButton {
            id: clearButton

            icon.name: "edit-clear-all-symbolic"
            onClicked: root.clearClicked()
            visible: root.canClear && !root.textArea.readOnly && root.textArea.text.length !== 0

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Clear")
        }
        ToolButton {
            id: undoButton

            icon.name: "edit-undo-symbolic"
            onClicked: {
                if (root.textArea.canUndo)
                    root.textArea.undo()
                else
                    root.undoFallbackClicked()
            }
            visible: !root.textArea.readOnly && root.canUndo && (root.textArea.canUndo || root.canUndoFallback)

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Undo")
        }
        ToolButton {
            id: redoButton

            icon.name: "edit-redo-symbolic"
            onClicked: root.textArea.redo()
            visible: !root.textArea.readOnly && root.canRedo && root.textArea.canRedo

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Redo")
        }
    }
}

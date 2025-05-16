/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
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

    property string name: ""
    property alias textArea: _textArea
    property alias textFormatCombo: _textFormatCombo
    property alias textFormatInvalid: _textFormatComboRedBorder.visible
    property color textColor: palette.text
    property bool showTranslate: false
    property bool showInsertIndicator: false
    property bool showControlTags: false
    property bool canUndo: true
    property bool canUndoFallback: false
    property bool canRedo: true
    property bool canClear: true
    property bool canPaste: true
    property bool canReadSelected: false
    property bool canReadAll: false
    property bool canTranslateSelected: false
    property bool canTranslateAll: false
    // readonly property bool _fitContent: scrollView.availableHeight - 4 * appWin.padding >= scrollView.contentHeight
    // readonly property bool _contextActive: _textFormatCombo.hovered || copyButton.hovered || clearButton.hovered ||
    //                                        undoButton.hovered || redoButton.hovered || pasteButton.hovered ||
    //                                        _fitContent
    readonly property bool _fitContent: true
    readonly property bool _contextActive: true
    signal clearClicked()
    signal copyClicked()
    signal undoFallbackClicked()
    signal readSelectedClicked(int start, int end);
    signal translateSelectedClicked(int start, int end);

    function scrollToBottom() {
        var position = (scrollView.contentHeight - scrollView.availableHeight) / scrollView.contentHeight
        if (position && position > 0)
            scrollView.ScrollBar.vertical.position = position
    }

    ScrollView {
        id: scrollView

        anchors.fill: parent
        enabled: root.enabled
        clip: true

        opacity: enabled ? 1.0 : 0.8
        Behavior on opacity { OpacityAnimator { duration: 100 } }

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

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
            persistentSelection: true
            selectByMouse: true
            wrapMode: TextEdit.WordWrap
            verticalAlignment: TextEdit.AlignTop
            font: _settings.notepad_font
            onCursorPositionChanged: cursorIndicator.update(cursorPosition)
            Component.onCompleted: cursorIndicator.update(-1)
            onTextChanged: cursorIndicator.update(cursorPosition)
            Accessible.name: root.name
            bottomPadding: _textFormatCombo.height + 2 * appWin.padding

            Rectangle {
                id: cursorIndicator

                function update(position) {
                    if (root.textArea.text.length === 0) position = 0
                    else if (position < 0) position = root.textArea.text.length

                    var rect = root.textArea.positionToRectangle(position)

                    width = rect.width
                    height = rect.height
                    x = rect.x
                    y = rect.y
                }

                visible: root.showInsertIndicator && !parent.activeFocus
                color: parent.palette.text
            }

            TextContextMenu {
                disableNative: true

                MenuSeparator {}

                Menu {
                    title: qsTr("Insert control tag")
                    enabled: root.showControlTags
                    height: enabled ? implicitHeight : 0

                    MenuItem {
                        action: Action {
                            text: "{speed: 1.0}"
                        }
                        enabled: root.showControlTags
                        visible: enabled
                        onTriggered: {
                            root.textArea.insert(root.textArea.cursorPosition, "{speed: 1.0}")
                        }
                    }

                    MenuItem {
                        action: Action {
                            text: "{silence: 1s}"
                        }
                        enabled: root.showControlTags
                        visible: enabled
                        onTriggered: {
                            root.textArea.insert(root.textArea.cursorPosition, "{silence: 1s}")
                        }
                    }
                }

                MenuSeparator {}

                MenuItem {
                    action: Action {
                        icon.name: "audio-speakers-symbolic"
                        text: (root.textArea.selectedText.length !== 0 && root.canReadSelected) ? qsTr("Read selection") : qsTr("Read All")
                    }
                    enabled: (root.textArea.selectedText.length !== 0 && root.canReadSelected) || root.canReadAll
                    onTriggered: {
                        if (root.textArea.selectedText.length !== 0 && root.canReadSelected)
                            root.readSelectedClicked(root.textArea.selectionStart, root.textArea.selectionEnd)
                        else
                            root.readSelectedClicked(0, root.textArea.selectedText.length - 1)
                    }
                }

                MenuItem {
                    action: Action {
                        icon.name: "audio-speakers-symbolic"
                        text: qsTr("Read from cursor position")
                    }
                    enabled: root.textArea.cursorPosition >= 0 &&
                             root.textArea.cursorPosition < root.textArea.text.length &&
                             root.canReadSelected
                    onTriggered: {
                        root.readSelectedClicked(root.textArea.cursorPosition, -1)
                    }
                }

                MenuItem {
                    action: Action {
                        icon.name: "audio-speakers-symbolic"
                        text: qsTr("Read to cursor position")
                    }
                    enabled: root.textArea.cursorPosition > 0 && root.canReadSelected
                    onTriggered: {
                        root.readSelectedClicked(0, root.textArea.cursorPosition)
                    }
                }

                MenuSeparator {
                    visible: root.showTranslate
                    height: visible ? implicitHeight : 0
                }

                MenuItem {
                    visible: root.showTranslate
                    height: visible ? implicitHeight : 0
                    action: Action {
                        icon.name: "insert-text-symbolic"
                        text: (root.textArea.selectedText.length !== 0 && root.canTranslateSelected) ? qsTr("Translate selection") : qsTr("Translate All")
                    }
                    enabled: (root.textArea.selectedText.length !== 0 && root.canTranslateSelected) || root.canTranslateSelected
                    onTriggered: {
                        if (root.textArea.selectedText.length !== 0 && root.canTranslateSelected)
                            root.translateSelectedClicked(root.textArea.selectionStart, root.textArea.selectionEnd)
                        else
                            root.translateSelectedClicked(0, root.textArea.selectedText.length - 1)
                    }
                }

                MenuItem {
                    visible: root.showTranslate
                    height: visible ? implicitHeight : 0
                    action: Action {
                        icon.name: "insert-text-symbolic"
                        text: qsTr("Translate from cursor position")
                    }
                    enabled: root.textArea.cursorPosition >= 0 &&
                             root.textArea.cursorPosition < root.textArea.text.length &&
                             root.canTranslateSelected
                    onTriggered: {
                        root.translateSelectedClicked(root.textArea.cursorPosition, -1)
                    }
                }

                MenuItem {
                    visible: root.showTranslate
                    height: visible ? implicitHeight : 0
                    action: Action {
                        icon.name: "insert-text-symbolic"
                        text: qsTr("Translate to cursor position")
                    }
                    enabled: root.textArea.cursorPosition > 0 && root.canTranslateSelected
                    onTriggered: {
                        root.translateSelectedClicked(0, root.textArea.cursorPosition)
                    }
                }
            }
        }
    }

    RowLayout {
        opacity: root.enabled && root._contextActive ? 1.0 : 0.4
        Behavior on opacity { OpacityAnimator { duration: 100 } }
        visible: opacity > 0.0

        anchors {
            bottom: parent.bottom
            bottomMargin: appWin.padding
            left: parent.left
            leftMargin: appWin.padding
            right: parent.right
            rightMargin: appWin.padding + (scrollView.ScrollBar.vertical.visible ?
                                               scrollView.ScrollBar.vertical.width : 0)
        }

        ComboBox {
            id: _textFormatCombo

            Layout.alignment: Qt.AlignLeft
            visible: model !== undefined && model.length !== 0

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: root.textFormatInvalid ? qsTr("The text format may be incorrect!") : qsTr("Text format")
            hoverEnabled: true

            Rectangle {
                id: _textFormatComboRedBorder

                visible: false
                border.width: 1
                border.color: "red"
                radius: 2
                anchors.fill: parent
                color: "transparent"
            }

            Accessible.name: qsTr("Text formats")
        }

        Item {
            Layout.fillWidth: true
        }

        ToolButton {
            id: copyButton

            Layout.alignment: Qt.AlignRight
            icon.name: "edit-copy-symbolic"
            text: qsTr("Copy")
            display: AbstractButton.IconOnly
            onClicked: root.copyClicked()
            visible: root.textArea.text.length !== 0

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Copy")
            hoverEnabled: true
        }
        ToolButton {
            id: pasteButton

            Layout.alignment: Qt.AlignRight
            icon.name: "edit-paste-symbolic"
            text: qsTr("Paste")
            display: AbstractButton.IconOnly
            onClicked: root.textArea.paste()
            visible: root.canPaste && root.textArea.canPaste

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Paste")
            hoverEnabled: true
        }
        ToolButton {
            id: clearButton

            Layout.alignment: Qt.AlignRight
            icon.name: "edit-delete-symbolic"
            text: qsTr("Clear")
            display: AbstractButton.IconOnly
            onClicked: root.clearClicked()
            visible: root.canClear && !root.textArea.readOnly && root.textArea.text.length !== 0

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Clear")
            hoverEnabled: true
        }
        ToolButton {
            id: undoButton

            Layout.alignment: Qt.AlignRight
            icon.name: "edit-undo-symbolic"
            text: qsTr("Undo")
            display: AbstractButton.IconOnly
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
            hoverEnabled: true
        }
        ToolButton {
            id: redoButton

            Layout.alignment: Qt.AlignRight
            icon.name: "edit-redo-symbolic"
            text: qsTr("Redo")
            display: AbstractButton.IconOnly
            onClicked: root.textArea.redo()
            visible: !root.textArea.readOnly && root.canRedo && root.textArea.canRedo

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("Redo")
            hoverEnabled: true
        }
    }
}

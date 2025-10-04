/* Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    default property alias content: column.data
    property alias listViewStackItem: listViewStack
    property alias flickItem: flick
    property alias placeholderLabel: _placeholderLabel
    property alias footerLabel: _footerLabel
    property alias frame: _frame
    readonly property bool listViewExists: !listViewStackItem.empty
    readonly property real _rightMargin: (!root.mirrored && scrollBar.visible) ? appWin.padding + scrollBar.width : appWin.padding
    readonly property real _leftMargin: (root.mirrored && scrollBar.visible) ? appWin.padding + scrollBar.width : appWin.padding

    implicitHeight: Math.min(
                        header.height + flick.contentHeight + (listViewExists ? root.parent.height : 0) + footer.height + 8 * verticalPadding,
                        parent.height - 4 * appWin.padding)
    implicitWidth: parent.width - 4 * appWin.padding
    anchors.centerIn: parent
    verticalPadding: appWin.padding
    horizontalPadding: 1
    modal: true

    header: Item {
        visible: root.title.length !== 0
        height: visible ? titleLabel.height + appWin.padding : 0

        Label {
            id: titleLabel

            anchors {
                left: parent.left
                leftMargin: root.leftPadding + appWin.padding
                top: parent.top
                topMargin: root.topPadding
            }

            text: root.title
            wrapMode: Text.Wrap
            font.pixelSize: Qt.application.font.pixelSize * 1.2
            elide: Label.ElideRight
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }
    }

    footer: Item {
        height: closeButton.height + appWin.padding

        Label {
            id: _footerLabel

            wrapMode: Text.Wrap
            anchors {
                left: parent.left
                leftMargin: root.leftPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding + appWin.padding
            }
        }

        Button {
            id: closeButton

            anchors {
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }
            text: qsTr("Close")
            icon.name: "window-close-symbolic"
            onClicked: root.reject()
            Keys.onEscapePressed: root.reject()
        }
    }

    Frame {
        id: _frame

        anchors.fill: parent
        topPadding: 1
        bottomPadding: 1
        leftPadding: 0
        rightPadding: 0
        leftInset: -1
        rightInset: -1
        clip: true

        PlaceholderLabel {
            id: _placeholderLabel

            enabled: false
        }

        StackView {
            id: listViewStack

            anchors.fill: parent
        }

        Component {
            id: listViewComp

            ListView {
                id: listView

                anchors.fill: parent
                topMargin: appWin.padding
                bottomMargin: appWin.padding

                focus: true
                clip: true
                spacing: appWin.padding

                Keys.onUpPressed: listViewScrollBar.decrease()
                Keys.onDownPressed: listViewScrollBar.increase()

                ScrollBar.vertical: ScrollBar {
                    id: listViewScrollBar
                }
            }
        }

        Flickable {
            id: flick

            anchors.fill: root.listViewExists ? null : parent
            visible: !root.listViewExists
            contentWidth: width
            contentHeight: column.height + 2 * appWin.padding
            topMargin: appWin.padding
            bottomMargin: appWin.padding
            clip: true

            Keys.onUpPressed: scrollBar.decrease()
            Keys.onDownPressed: scrollBar.increase()

            ScrollBar.vertical: ScrollBar { id: scrollBar }

            ColumnLayout {
                id: column
                x: root._leftMargin
                width: flick.width - x - root._rightMargin
                spacing: appWin.padding
            }
        }
    }
}

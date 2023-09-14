/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

Dialog {
    id: root

    default property alias content: column.data
    property alias listViewItem: listView
    property alias flickItem: flick
    property alias placeholderLabel: _placeholderLabel
    readonly property real _rightMargin: scrollBar.visible ? appWin.padding + scrollBar.width : appWin.padding
    readonly property real _leftMargin: appWin.padding

    implicitHeight: Math.min(
                        header.height + flick.contentHeight + (listView.model ? root.parent.height : 0) + footer.height + 8 * verticalPadding,
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
            Keys.onReturnPressed: root.reject()
        }
    }

    Frame {
        id: frame

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

        Flickable {
            id: flick

            anchors.fill: listView.model ? null : parent
            visible: !listView.model
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

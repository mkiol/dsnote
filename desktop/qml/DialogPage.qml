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
    readonly property real _rightMargin: scrollBar.visible ? appWin.padding + scrollBar.width : appWin.padding
    readonly property real _leftMargin: appWin.padding

    standardButtons: Dialog.Close
    implicitHeight: Math.min(
                        header.height + flick.contentHeight + footer.height + 2 * verticalPadding,
                        parent.height - 4 * appWin.padding)
    implicitWidth: parent.width - 4 * appWin.padding
    anchors.centerIn: parent
    verticalPadding: appWin.padding
    horizontalPadding: 1
    modal: true
    header: Item {
        visible: root.title.length !== 0
        height: titleLabel.height + appWin.padding

        Label {
            id: titleLabel
            x: appWin.padding
            y: appWin.padding

            text: root.title
            font.pixelSize: Qt.application.font.pixelSize * 1.2
            elide: Label.ElideRight
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }
    }

    Flickable {
        id: flick

        anchors.fill: parent
        contentWidth: width
        contentHeight: column.height + 2 * appWin.padding
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

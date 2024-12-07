/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
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

    standardButtons: Dialog.Close
    anchors.centerIn: parent
    modal: true

    width: flick.width + root.leftPadding + root.rightMargin
    height: flick.height + root.header.height + root.footer.height + root.topPadding + root.bottomPadding

    Flickable {
        id: flick

        width: contentWidth
        height: Math.min(contentHeight, root.parent.height -
                         root.header.height - root.footer.height - root.topPadding - root.bottomPadding)
        contentWidth: Math.min(column.implicitWidth, root.parent.width - root.leftPadding - root.rightPadding)
        contentHeight: column.height
        clip: true

        Keys.onUpPressed: scrollBar.decrease()
        Keys.onDownPressed: scrollBar.increase()
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: column

            width: Math.min(column.implicitWidth, root.parent.width - root.leftPadding - root.rightPadding) -
                    (flick.ScrollBar.vertical.visible ? flick.ScrollBar.vertical.width : root.rightPadding)
        }
    }
}

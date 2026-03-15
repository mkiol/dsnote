/* Copyright (C) 2024-2025 Michal Kosciesza <michal@mkiol.net>
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
    readonly property real parentWidth: parent && parent.width ? parent.width : 0
    readonly property real parentHeight: parent && parent.height ? parent.height : 0

    standardButtons: Dialog.Close
    anchors.centerIn: parent
    bottomInset: appWin.padding
    modal: true

    width: flick.width - (2 * appWin.padding)
    height: flick.height + header.height + footer.height + topPadding + bottomPadding + bottomInset

    Component.onCompleted: {
        footer.bottomPadding += bottomInset
    }

    Flickable {
        id: flick

        width: contentWidth
        height: Math.min(contentHeight, root.parentHeight -
                         root.header.height - root.footer.height - root.topPadding - root.bottomPadding - root.bottomInset)
        contentWidth: Math.min(column.implicitWidth, root.parentWidth - root.leftPadding - root.rightPadding)
        contentHeight: column.height
        clip: true

        Keys.onUpPressed: scrollBar.decrease()
        Keys.onDownPressed: scrollBar.increase()
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: column

            width: Math.min(column.implicitWidth, root.parentWidth - root.leftPadding - root.rightPadding - 4 * appWin.padding) -
                    (((!root.mirrored && flick.ScrollBar.vertical.visible) ? flick.ScrollBar.vertical.width : root.rightPadding) + x)
            x: (root.mirrored && flick.ScrollBar.vertical.visible) ? flick.ScrollBar.vertical.width : 0
        }
    }
}

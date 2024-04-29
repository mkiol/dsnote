/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

InlineMessage {
    id: root

    property int indends: 0
    property bool verticalMode: parent.verticalMode !== undefined ? parent.verticalMode :
                                parent.parent.verticalMode !== undefined ? parent.parent.verticalMode :
                                parent.parent.parent.verticalMode !== undefined ? parent.parent.parent.verticalMode :
                                false
    property alias text: _label.text
    property alias label: _label
    readonly property double _textWidth: fontMetrics.boundingRect(text).width
    readonly property bool _fill: _textWidth > (0.8 * appWin.width) || verticalMode

    color: "red"
    Layout.fillWidth: _fill
    Layout.preferredWidth: _fill ? 0 : _textWidth + 5 * appWin.padding
    Layout.leftMargin: indends * appWin.padding
    Layout.rightMargin: indends * appWin.padding

    onCloseClicked: visible = false

    FontMetrics {
        id: fontMetrics
    }

    Label {
        id: _label

        Layout.fillWidth: true
        color: root.color
        wrapMode: Text.Wrap
    }
}

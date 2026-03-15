/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Label {
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    Layout.margins: 3 * appWin.padding
    horizontalAlignment: Qt.AlignHCenter
    verticalAlignment: Qt.AlignVCenter
    wrapMode: Text.Wrap
    textFormat: Text.StyledText
    font.pixelSize: Qt.application.font.pixelSize * 1.2
    visible: opacity > 0.0
    opacity: enabled ? 0.8 : 0.0
    Behavior on opacity { OpacityAnimator { duration: 100 } }
}

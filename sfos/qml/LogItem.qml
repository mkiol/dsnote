/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Label {
    textFormat: Text.RichText
    wrapMode: Text.Wrap
    onLinkActivated: Qt.openUrlExternally(link)
    anchors.left: parent.left; anchors.right: parent.right
    anchors.leftMargin: Theme.paddingLarge; anchors.rightMargin: Theme.paddingLarge
    highlighted: true
    linkColor: Theme.highlightColor
}

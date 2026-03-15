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
    textFormat: Text.RichText
    wrapMode: Text.Wrap
    onLinkActivated: Qt.openUrlExternally(link)
}

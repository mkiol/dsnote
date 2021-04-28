/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: root

    allowedOrientations: Orientation.All

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content

            width: root.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Changes")
            }

            SectionHeader {
                text: qsTr("Version %1").arg("1.0.1")
            }

            LogItem {
                title: "DeepSpeech lib update"
                description: "DeepSpeech library was updated to 0.10.0-alpha.3 version. " +
                             "Speech recognition accuracy is much better now."
            }

            LogItem {
                title: "Support for Jolla 1, Jolla C and PinePhone"
                description: "DeepSpeech library update made possible to run app on more ARM devices. " +
                             "Unfortunately Jolla Tablet is still not supported."
            }

            Spacer {}
        }
    }

    VerticalScrollDecorator {
        flickable: flick
    }
}

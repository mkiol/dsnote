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
        contentHeight: column.height

        Column {
            id: column

            width: root.width

            PageHeader {
                title: qsTr("Languages")
            }

            // service.all_models:
            // [0] - model id
            // [1] - lang id
            // [2] - friendly name
            // [3] - model availability
            // [4] - download in progress
            // [5] - download progress

            LangList {
                width: root.width
                model: service.all_models
                visible: !service.busy
            }

            ExpandingSectionGroup {
                ExpandingSection {
                    expanded: false
                    title: qsTr("Experimental")

                    content.sourceComponent: Column {
                        LangList {
                            width: root.width
                            model: service.all_experimental_models
                            visible: !service.busy
                        }
                    }
                }
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: service.busy
        size: BusyIndicatorSize.Large
    }

    VerticalScrollDecorator {
        flickable: flick
    }
}

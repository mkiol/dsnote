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

    SilicaListView {
        id: listView

        anchors.fill: parent
        currentIndex: -1
        model: service.lang_model

        Connections {
            target: service.lang_model
            onItemChanged: {
                listView.positionViewAtIndex(idx, ListView.Center)
            }
        }

        Binding {
            target: service.lang_model
            property: "showExperimental"
            value: _settings.show_experimental
        }

        PullDownMenu {
            busy: app.busy || service.busy || service.lang_model.downloading

            MenuItem {
                text: _settings.show_experimental ? qsTr("Hide experimental") : qsTr("Show experimental")
                onClicked: {
                    _settings.show_experimental = !_settings.show_experimental
                }
            }
        }

        header: SearchPageHeader {
            implicitWidth: root.width
            title: qsTr("Languages")
            model: listView.model
            view: listView
        }

        delegate: LangItem {
            name: model.name
            modelId: model.id
            available: model.available
            experimental: model.experimental
            downloading: model.downloading
            progress: model.progress
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: service.busy
        size: BusyIndicatorSize.Large
    }

    VerticalScrollDecorator {
        flickable: listView
    }
}

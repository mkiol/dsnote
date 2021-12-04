/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

SimpleListItem {
    id: listItem

    property string name
    property string modelId
    property bool available: true
    property bool experimental: false
    property bool downloading: false
    property double progress: 0.0

    readonly property color itemColor: highlighted ? Theme.highlightColor : Theme.primaryColor
    readonly property color secondaryItemColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

    title.text: experimental ? "🔬 " + name : name

    Component {
        id: menuComp
        ContextMenu {
            MenuItem {
                enabled: !listItem.downloading
                text: listItem.available ? qsTr("Delete") : qsTr("Download")
                onClicked: {
                    if (listItem.available) {
                        service.delete_model(listItem.modelId)
                    } else {
                        service.download_model(listItem.modelId)
                    }
                }
            }
        }
    }

    menu: listItem.downloading ? null : menuComp

    Image {
        visible: listItem.available
        source: "image://theme/icon-m-certificates?" + listItem.itemColor
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter
        height: Theme.iconSizeMedium
        width: Theme.iconSizeMedium
    }

    BusyIndicator {
        visible: listItem.downloading
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter
        height: Theme.iconSizeMedium
        width: Theme.iconSizeMedium
        running: visible

        Label {
            id: progressLabel
            color: Theme.highlightColor
            anchors.centerIn: parent
            font.pixelSize: Theme.fontSizeTiny
            text:  listItem.progress > 0.0 ?
                       Math.round(listItem.progress * 100) + "%" : ""
        }

        Connections {
            target: service
            onModel_download_progress: {
                if (listItem.modelId === id) {
                    progressLabel.text = Math.round(progress * 100) + "%"
                }
            }
        }
    }

    onClicked: openMenu()
}

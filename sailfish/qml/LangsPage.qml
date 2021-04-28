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

        model: app.langs

        header: PageHeader {
            title: qsTr("Languages")
        }

        delegate: SimpleListItem {
            id: listItem

            visible: !app.busy

            property color itemColor: highlighted ? Theme.highlightColor : Theme.primaryColor
            property color secondaryItemColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

            property string langId: modelData[0]
            property bool available: modelData[2]
            property bool downloading: modelData[3]
            property double progress: modelData[4]

            title.text: modelData[1]

            Component {
                id: menuComp
                ContextMenu {
                    MenuItem {
                        enabled: !listItem.downloading
                        text: listItem.available ? qsTr("Delete") : qsTr("Download")
                        onClicked: {
                            if (listItem.available) {
                                app.delete_lang(listItem.langId)
                            } else {
                                app.download_lang(listItem.langId)
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
                    target: app
                    onLang_download_progress: {
                        if (listItem.langId === id) {
                            progressLabel.text = Math.round(progress * 100) + "%"
                        }
                    }
                }
            }

            onClicked: openMenu()
        }

        ViewPlaceholder {
            enabled: listView.count === 0 && !app.busy
            text: qsTr("No languages")
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: app.busy
        size: BusyIndicatorSize.Large
    }

    VerticalScrollDecorator {
        flickable: listView
    }
}

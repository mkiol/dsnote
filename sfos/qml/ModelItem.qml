/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

SimpleListItem {
    id: root

    property string name
    property string modelId
    property int score: 2
    property bool defaultModelForLangAllowed: false
    property bool defaultModelForLang: false
    property bool available: true
    property bool downloading: false
    property double progress: 0.0

    readonly property color itemColor: highlighted ? Theme.highlightColor : Theme.primaryColor
    readonly property color secondaryItemColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

    function formatProgress(progress) {
        return progress > 0.0 ? Math.round(progress * 100) + "%" : ""
    }

    title.horizontalAlignment: Text.AlignLeft

    title.text: {
        if (available && defaultModelForLang) return "⭐ " + name
        return name
    }

    Component {
        id: menuComp
        ContextMenu {
            MenuItem {
                visible: root.available && root.defaultModelForLangAllowed && !root.defaultModelForLang
                text: qsTr("Set as default for this language")
                onClicked: {
                    if (root.default_model_for_lang) return
                    service.set_default_model_for_lang(modelId)
                }
            }
            MenuItem {
                text: root.downloading ? qsTr("Cancel") : root.available ? qsTr("Delete") : qsTr("Download")
                onClicked: {
                    if (root.downloading) service.cancel_model_download(root.modelId)
                    else if (root.available) service.delete_model(root.modelId)
                    else service.download_model(root.modelId)
                }
            }
        }
    }

    onProgressChanged: progressLabel.text = formatProgress(progress)
    Connections {
        target: service
        onModel_download_progress_changed: {
            if (root.modelId === id) {
                progressLabel.text = formatProgress(progress)
            }
        }
    }
    Component.onCompleted: {
        if (downloading) {
            progressLabel.text = formatProgress(
                        service.model_download_progress(root.modelId))
        }
    }

    menu: menuComp

    Image {
        visible: root.available
        source: "image://theme/icon-m-certificates?" + root.itemColor
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter
        height: Theme.iconSizeMedium
        width: Theme.iconSizeMedium
    }

    BusyIndicator {
        visible: root.downloading
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
        }
    }

    onClicked: openMenu()
}

/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Dsnote 1.0

SimpleListItem {
    id: root

    property var mobj: null
    property string packId: ""
    property string langId
    property string langName
    property double progress: 0.0
    readonly property bool defaultModelForLangAllowed: mobj.role === ModelsListModel.Stt ||
                                                       mobj.role === ModelsListModel.Tts
    readonly property color itemColor: highlighted ? Theme.highlightColor : Theme.primaryColor
    readonly property color secondaryItemColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
    readonly property bool packView: packId.length !== 0
    readonly property bool isPack: mobj && mobj.pack_id.length !== 0

    function formatProgress(progress) {
        return progress > 0.0 ? Math.round(progress * 100) + "%" : ""
    }

    function showPack() {
        var lang_id = langId
        var lang_name = langName
        var pack_id = mobj.pack_id
        var pack_name = mobj.name

        pageStack.push(Qt.resolvedUrl("LangsPage.qml"),
                       {langId: lang_id, langName: lang_name, packId: pack_id, packName: pack_name})
    }

    title.horizontalAlignment: Text.AlignLeft

    title.text: {
        if (mobj.available && mobj.default_for_lang) return "⭐ " + mobj.name
        return name
    }

    Component {
        id: modelMenuComp
        ContextMenu {
            MenuItem {
                visible: !root.isPack && root.mobj.available &&
                         root.defaultModelForLangAllowed && !root.mobj.default_for_lang
                text: qsTr("Set as default for this language")
                onClicked: {
                    if (root.mobj.default_for_lang) return
                    service.set_default_model_for_lang(mobj.id)
                }
            }

            MenuItem {
                enabled: !root.mobj.dl_off
                visible: !root.mobj.downloading && !root.mobj.available
                text: root.mobj.dl_multi ? qsTr("Enable") : qsTr("Download")
                onClicked: service.download_model(root.mobj.id)
            }

            MenuItem {
                visible: !root.mobj.downloading && root.mobj.available
                text: root.mobj.dl_multi ? qsTr("Disable") : qsTr("Delete")
                onClicked: service.delete_model(root.mobj.id)
            }

            MenuItem {
                visible: root.mobj.downloading
                text: qsTr("Cancel")
                onClicked: service.cancel_model_download(root.mobj.id)
            }
        }
    }

    Component {
        id: packMenuComp
        ContextMenu {
            MenuItem {
                text: qsTr("Show models")
                onClicked: showPack()
            }
        }
    }

    onProgressChanged: progressLabel.text = formatProgress(root.progress)
    Connections {
        target: service
        onModel_download_progress_changed: {
            if (root.mobj.id === id) {
                progressLabel.text = formatProgress(progress)
            }
        }
    }
    Component.onCompleted: {
        if (downloading) {
            progressLabel.text = formatProgress(
                        service.model_download_progress(root.mobj.id))
        }
    }

    menu: isPack ? packMenuComp : modelMenuComp

    textRightMargin: Theme.horizontalPageMargin +
                     (root.mobj.available || root.mobj.downloading ? Theme.iconSizeMedium : 0) +
                     (root.isPack ? Theme.iconSizeMedium : 0)

    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter
        height: Theme.iconSizeMedium

        Image {
            visible: root.mobj.available
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-certificates?" + root.itemColor
            height: Theme.iconSizeMedium
            width: Theme.iconSizeMedium
        }

        BusyIndicator {
            visible: root.mobj.downloading
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

        Image {
            visible: root.isPack
            source: "image://theme/icon-m-forward?" + root.itemColor
            anchors.verticalCenter: parent.verticalCenter
            height: Theme.iconSizeMedium
            width: Theme.iconSizeMedium
        }
    }

    onClicked: isPack ? showPack() : openMenu()
}

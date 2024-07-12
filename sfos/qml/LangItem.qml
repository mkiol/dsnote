/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Dsnote 1.0

SimpleListItem {
    id: listItem

    property string name
    property string langId
    property bool available: true
    property bool downloading: false

    readonly property color itemColor: highlighted ? Theme.highlightColor : Theme.primaryColor
    readonly property color secondaryItemColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

    title.text: name

    title.horizontalAlignment: Text.AlignLeft

    function reset(lang_id) {
        service.models_model.lang = lang_id
        service.pack_model.lang = ""
        service.pack_model.pack = ""
        service.models_model.filter = ""
        service.models_model.roleFilter = ModelsListModel.AllModels
    }

    function showModels() {
        reset(langId)
        pageStack.push(Qt.resolvedUrl("LangsPage.qml"), {langId: langId, langName: name})
    }

    Component {
        id: menuComp
        ContextMenu {
            MenuItem {
                text: qsTr("Show models")
                onClicked: showModels()
            }
        }
    }

    onClicked: showModels()

    menu: menuComp

    Image {
        visible: listItem.available && !listItem.downloading
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
    }
}

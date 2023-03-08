/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.DirModel 1.0

Dialog {
    id: root

    allowedOrientations: Orientation.All

    property real preferredItemHeight: root && root.isLandscape ?
                                           Theme.itemSizeSmall :
                                           Theme.itemSizeLarge

    property string selectedPath

    canAccept: itemModel.isCurrentWritable()

    onDone: {
        if (result === DialogResult.Accepted)
            selectedPath = itemModel.currentPath
    }

    // Hack to update model after all transitions
    property bool _completed: false
    Component.onCompleted: _completed = true
    onStatusChanged: {
        if (status === PageStatus.Active && _completed) {
            _completed = false
            itemModel.updateModel()
        }
    }

    DirModel {
        id: itemModel
        currentPath: _settings.lang_models_dir
    }

    SilicaListView {
        id: listView

        anchors.fill: parent

        currentIndex: -1

        model: itemModel

        header: DialogHeader {
            title: itemModel.currentPath
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("SD Card")
                onClicked: itemModel.changeToRemovable()
            }

            MenuItem {
                text: qsTr("Home")
                onClicked: itemModel.changeToHome()
            }
        }

        delegate: SimpleListItem {
            title.text: model.name
            icon.source: (title.text === ".." ?
                             "image://theme/icon-m-back?" :
                             "image://theme/icon-m-file-folder?") +
                         (highlighted ? Theme.highlightColor : Theme.primaryColor)
            onClicked: itemModel.currentPath = model.path
        }

        ViewPlaceholder {
            enabled: listView.count === 0 && !itemModel.busy
            text: qsTr("No directories")
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: itemModel.busy
        size: BusyIndicatorSize.Large
    }

    VerticalScrollDecorator {
        flickable: listView
    }
}

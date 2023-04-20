/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Dsnote 1.0

Page {
    id: root

    property string langId
    property string langName
    readonly property bool langsView: langId.length == 0
    readonly property var model: langsView ? service.langs_model : service.models_model

    Component.onCompleted: {
        service.models_model.lang = root.langId
    }

    allowedOrientations: Orientation.All

    SilicaListView {
        id: listView

        anchors.fill: parent
        currentIndex: -1
        model: root.model

        Connections {
            target: root.model
            onItemChanged: {
                listView.positionViewAtIndex(idx, ListView.Center)
            }
        }

        header: SearchPageHeader {
            implicitWidth: root.width
            title: langsView ? qsTr("Languages") : root.langName
            view: listView
        }

        Component {
            id: modelSectionDelegate
            SectionHeader {
                height: section == ModelsListModel.Stt ? 0 : implicitHeight
                text: section == ModelsListModel.Stt ? "" : qsTr("Complementary models")
            }
        }

        Component {
            id: modelItemDelegate
            ModelItem {
                name: model.name
                modelId: model.id
                available: model.available
                downloading: model.downloading
                progress: model.progress
                score: model.score
                defaultModelForLangAllowed: model.role === ModelsListModel.Stt
                defaultModelForLang: model.default_for_lang
            }
        }

        Component {
            id: langItemDelegate
            LangItem {
                name: model.name
                langId: model.id
                available: model.available
                downloading: model.downloading
            }
        }

        delegate: langsView ? langItemDelegate : modelItemDelegate

        section.property: "role"
        section.delegate: langsView ? null : modelSectionDelegate
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

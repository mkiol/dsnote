/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
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
    property string packId: ""
    property string langName
    property string packName: ""
    readonly property bool langsView: langId.length == 0
    readonly property bool packView: packId.length !== 0
    readonly property bool modelView: !langsView && !packView
    readonly property var model: langsView ? service.langs_model :
                                 packView ? service.pack_model : service.models_model

    function reset(lang_id, pack_id) {
        service.models_model.lang = lang_id
        service.pack_model.pack = pack_id

        if (langsView) service.langs_model.filter = ""
        if (packView) {
            service.pack_model.roleFilter = ModelsListModel.AllModels
            service.pack_model.filter = ""
        }
        if (modelView) {
            service.models_model.roleFilter = ModelsListModel.AllModels
            service.models_model.filter = ""
        }
    }

    Component.onCompleted: {
        reset(root.langId, root.packId)
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
            id: searchPageHeader

            width: root.width
            title: root.langsView ? qsTr("Languages") : root.packView ? root.packName : root.langName
            view: listView
            comboModel: [
                qsTr("All"),
                qsTr("Speech to Text"),
                qsTr("Text to Speech"),
                qsTr("Translator"),
                qsTr("Other")
            ]
            combo {
                visible: !root.langsView && !root.packView
                label: qsTr("Model type")
                currentIndex: {
                    if (root.langsView || root.packView) return
                    switch (service.models_model.roleFilterFlags) {
                    case ModelsListModel.RoleAll:
                        return 0
                    case ModelsListModel.RoleStt:
                        return 1
                    case ModelsListModel.RoleTts:
                        return 2
                    case ModelsListModel.RoleMnt:
                        return 3
                    case ModelsListModel.RoleOther:
                        return 4
                    }
                    return 0
                }
                onCurrentIndexChanged: {
                    if (root.langsView) return
                    var index = searchPageHeader.combo.currentIndex
                    if (index === 1) service.models_model.roleFilterFlags = ModelsListModel.RoleStt
                    else if (index === 2) service.models_model.roleFilterFlags = ModelsListModel.RoleTts
                    else if (index === 3) service.models_model.roleFilterFlags = ModelsListModel.RoleMnt
                    else if (index === 4) service.models_model.roleFilterFlags = ModelsListModel.RoleOther
                    else service.models_model.roleFilterFlags = ModelsListModel.RoleAll
                }
            }
        }

        Component {
            id: modelSectionDelegate
            SectionHeader {
                text: {
                    if (section == ModelsListModel.Stt)
                        return qsTr("Speech to Text")
                    if (section == ModelsListModel.Tts)
                        return qsTr("Text to Speech")
                    if (section == ModelsListModel.Mnt)
                        return qsTr("Translator")
                    if (section == ModelsListModel.Ttt)
                        return qsTr("Other")
                }
            }
        }

        Component {
            id: modelItemDelegate
            ModelItem {
                mobj: model
                langName: root.langName
                langId: root.langId
                progress: model.progress
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

        ViewPlaceholder {
            text: langsView ? qsTr("There are no languages that match your search criteria.") :
                              qsTr("There are no models that match your search criteria.")
            enabled: listView.model.count === 0
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

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

    function reset(lang_id) {
        service.models_model.lang = lang_id
        service.models_model.filter = ""
        service.models_model.roleFilter = ModelsListModel.AllModels
    }

    Component.onCompleted: reset(root.langId)

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
            title: langsView ? qsTr("Languages") : root.langName
            view: listView
            comboModel: [
                qsTr("All"),
                qsTr("Speech to Text"),
                qsTr("Text to Speech"),
                qsTr("Translator"),
                qsTr("Other")
            ]
            combo {
                visible: !root.langsView
                label: qsTr("Model type")
                currentIndex: {
                    if (root.langsView) return
                    switch (service.models_model.roleFilter) {
                    case ModelsListModel.AllModels:
                        return 0
                    case ModelsListModel.SttModels:
                        return 1
                    case ModelsListModel.TtsModels:
                        return 2
                    case ModelsListModel.MntModels:
                        return 3
                    case ModelsListModel.OtherModels:
                        return 4
                    }
                    return 0
                }
                onCurrentIndexChanged: {
                    if (root.langsView) return
                    var index = searchPageHeader.combo.currentIndex
                    if (index === 1) service.models_model.roleFilter = ModelsListModel.SttModels
                    else if (index === 2) service.models_model.roleFilter = ModelsListModel.TtsModels
                    else if (index === 3) service.models_model.roleFilter = ModelsListModel.MntModels
                    else if (index === 4) service.models_model.roleFilter = ModelsListModel.OtherModels
                    else service.models_model.roleFilter = ModelsListModel.AllModels
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
                name: model.name
                modelId: model.id
                available: model.available
                downloading: model.downloading
                progress: model.progress
                score: model.score
                defaultModelForLangAllowed: model.role === ModelsListModel.Stt || model.role === ModelsListModel.Tts
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

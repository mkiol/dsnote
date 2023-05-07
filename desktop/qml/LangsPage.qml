/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0

Page {
    id: root

    property string langId
    property string langName
    readonly property bool langsView: langId.length == 0

    title: langsView ? qsTr("Languages") : langName

    Component.onCompleted: {
        service.models_model.lang = root.langId
    }

    Component {
        id: langItemDelegate
        ItemDelegate {
            width: ListView.view.width
            text: model.name
            onClicked: {
                stackView.push("LangsPage.qml", {langId: model.id, langName: model.name})
            }
            BusyIndicator {
                visible: !model.available
                height: parent.height
                anchors.right: parent.right
                running: model.downloading
            }
            Label {
                visible: model.available
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 10
                text: "\u2714"
            }
        }
    }

    Component {
        id: modelSectionDelegate
        RowLayout {
            required property var section

            width: listView.width

            Label {
                Layout.margins: 10
                width: listView.width
                font.bold: true
                text: {
                    if (parent.section == ModelsListModel.Stt)
                        return qsTr("Speech to Text")
                    if (parent.section == ModelsListModel.Tts)
                        return qsTr("Text to Speech")
                    if (parent.section == ModelsListModel.Ttt)
                        return qsTr("Text to Text")
                }
            }
        }
    }

    Component {
        id: modelItemDelegate
        RowLayout {
            width: listView.width

            Label {
                Layout.margins: 10
                text: (model.score === 0 ? "(e) " : "") + model.name
            }

            ProgressBar {
                id: bar
                Layout.fillWidth: true
                value: model.available ? 1 : model.progress

                Connections {
                    target: service
                    function onModel_download_progress_changed(id, progress) {
                        if (model.id === id) bar.value = progress
                    }
                }
                Component.onCompleted: {
                    if (model.downloading) {
                        bar.value = service.model_download_progress(model.id)
                    }
                }
            }

            Button {
                Layout.rightMargin: 10
                text: model.downloading ? qsTr("Cancel") : model.available ? qsTr("Delete") : qsTr("Download")
                onClicked: {
                    if (model.downloading) service.cancel_model_download(model.id)
                    else if (model.available) service.delete_model(model.id)
                    else service.download_model(model.id)
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        spacing: 5
        model: langsView ? service.langs_model : service.models_model
        delegate: langsView ? langItemDelegate : modelItemDelegate
        section.property: "role"
        section.delegate: langsView ? null : modelSectionDelegate
    }
}

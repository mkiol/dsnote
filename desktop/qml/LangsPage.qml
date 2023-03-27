/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

Page {
    id: root

    property string langId
    property string langName
    readonly property bool langsView: langId.length == 0

    title: langsView ? qsTr("Languages") : langName

    Component.onCompleted: {
        service.models_model.lang = root.langId
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.height + 20
        clip: true

        ScrollIndicator.vertical: ScrollIndicator {}

        GridLayout {
            id: content
            width: parent.width
            flow: GridLayout.TopToBottom
            columnSpacing: 10
            rowSpacing: langsView ? 0 : 10
            columns: langsView ? 1 : 3
            rows: langsView ? service.langs_model.count : service.models_model.count

            Repeater {
                model: langsView ? service.langs_model : null
                ItemDelegate {
                    Layout.fillWidth: true
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

            Repeater {
                model: langsView ? null : service.models_model
                Label {
                    Layout.margins: 10
                    text: (model.score === 0 ? "(e) " :
                           model.score === 3 ? "(r) " : "") + model.name
                }
            }

            Repeater {
                model: langsView ? null : service.models_model
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
            }

            Repeater {
                model: langsView ? null : service.models_model
                Button {
                    text: model.downloading ? qsTr("Cancel") : model.available ? qsTr("Delete") : qsTr("Download")
                    onClicked: {
                        if (model.downloading) service.cancel_model_download(model.id)
                        else if (model.available) service.delete_model(model.id)
                        else service.download_model(model.id)
                    }
                }
            }
        }
    }
}

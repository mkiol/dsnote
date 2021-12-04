/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
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

    title: qsTr("Languages")

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.height + 20
        clip: true

        ScrollIndicator.vertical: ScrollIndicator {}

        GridLayout {
            id: content
            x: 10
            y: 10
            width: parent.width - 20
            flow: GridLayout.TopToBottom
            columnSpacing: 10
            rowSpacing: 10
            columns: 3
            rows: service.lang_model.count

            Repeater {
                model: service.lang_model
                Label {
                    text: model.name
                }
            }

            Repeater {
                model: service.lang_model
                ProgressBar {
                    id: bar
                    Layout.fillWidth: true
                    value: model.available ? 1 : model.progress

                    Connections {
                        target: service
                        function onModel_download_progress(id, progress) {
                            if (model.id === id) bar.value = progress
                        }
                    }
                }
            }

            Repeater {
                model: service.lang_model
                Button {
                    enabled: !model.downloading
                    text: model.downloading ? qsTr("Downloading...") : model.available ? qsTr("Delete") : qsTr("Download")
                    onClicked: {
                        if (model.available) service.delete_model(model.id)
                        else service.download_model(model.id)
                    }
                }
            }
        }
    }
}

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

    // app.langs:
    // [0] - id
    // [1] - friendly name
    // [2] - model availability
    // [3] - download in progress

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
            rows: app.langs.length

            Repeater {
                model: app.langs
                Label {
                    text: modelData[1]
                }
            }

            Repeater {
                model: app.langs
                ProgressBar {
                    id: bar
                    Layout.fillWidth: true
                    value: modelData[2] ? 1 : 0

                    Connections {
                        target: app
                        function onLang_download_progress(id, progress) {
                            if (modelData[0] === id) {
                                bar.value = progress
                            }
                        }
                    }
                }
            }

            Repeater {
                model: app.langs
                Button {
                    enabled: !modelData[3]
                    text: modelData[3] ? qsTr("Downloading...") : modelData[2] ? qsTr("Delete") : qsTr("Download")
                    onClicked: {
                        if (modelData[2])
                            app.delete_lang(modelData[0])
                        else
                            app.download_lang(modelData[0])
                    }
                }
            }
        }
    }
}

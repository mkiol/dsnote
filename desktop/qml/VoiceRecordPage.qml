/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

Dialog {
    id: root

    modal: true
    width: Math.min(implicitWidth, parent.width)
    height: column.implicitHeight + 2 * verticalPadding

    ColumnLayout {
        id: column

        width: parent.width
        spacing: appWin.padding

        Label {
            opacity: app.recorder_recording ? 1.0 : 0.6
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: app.recorder_recording ? qsTr("Say something...") :
                                           qsTr("Press %1 to start recording.").arg("<i>" + qsTr("Start") + "</i>")
            font.pixelSize: appWin.textFontSizeBig
        }

        Label {
            opacity: app.recorder_recording ? 1.0 : 0.0
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: app.recorder_duration + "s"
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter

            Button {
                icon.name: app.recorder_recording ? "media-playback-stop-symbolic" : "media-record-symbolic"
                text: app.recorder_recording ? qsTr("Stop") : qsTr("Start")
                onClicked: {
                    if (app.recorder_recording) {
                        app.recorder_stop()
                        root.accept()
                    } else {
                        app.recorder_start()
                    }
                }
            }

            Button {
                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.reject()
            }
        }
    }
}

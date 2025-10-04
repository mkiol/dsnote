/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
            color: app.recorder_duration >= 10 ? "green" : "red"
            text: app.recorder_duration + "s"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            Button {
                width: appWin.buttonWithIconWidth
                visible: app.recorder_recording
                enabled: app.recorder_duration >= 10
                icon.name: "media-playback-stop-symbolic"
                text: qsTr("Stop")
                onClicked: {
                    app.recorder_stop()
                    root.accept()
                }
            }

            Button {
                width: appWin.buttonWithIconWidth
                visible: !app.recorder_recording
                icon.name: "media-record-symbolic"
                text: qsTr("Start")
                onClicked: {
                    app.recorder_start()
                }
            }

            Button {
                width: appWin.buttonWithIconWidth
                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.reject()
            }
        }
    }
}

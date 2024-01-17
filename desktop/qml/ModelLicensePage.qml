/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

Dialog {
    id: root

    property string licenseId
    property string licenseName
    property url licenseUrl
    property var acceptHandler: null
    property bool busy: false

    function update_license() {
        busy = true
        textArea.text = ""
        textArea.text = app.download_content(licenseUrl);
        busy = false
    }

    title: licenseName.length !== 0 ?
               licenseName + " (" + licenseId + ")" : licenseId

    width: parent.width - 8 * appWin.padding
    height: scrollView.height + footer.height + header.height + root.topPadding + root.bottomPadding
    modal: true
    verticalPadding: appWin.padding
    horizontalPadding: appWin.padding

    onVisibleChanged: {
        if (visible) update_license()
    }

    header: Item {
        visible: root.title.length !== 0
        height: visible ? titleLabel.height + appWin.padding : 0

        Label {
            id: titleLabel

            anchors {
                left: parent.left
                leftMargin: appWin.padding
                top: parent.top
                topMargin: root.topPadding
            }

            text: root.title
            wrapMode: Text.Wrap
            font.pixelSize: Qt.application.font.pixelSize * 1.2
            elide: Label.ElideRight
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
        }
    }

    footer: Item {
        height: footerRow.height + appWin.padding

        RowLayout { 
            id: footerRow

            anchors {
                right: parent.right
                rightMargin: root.rightPadding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                visible: root.acceptHandler
                text: qsTr("Reject")
                onClicked: root.reject()
                Keys.onEscapePressed: root.reject()
            }
            Button {
                enabled: textArea.text.length !== 0
                visible: root.acceptHandler
                text: qsTr("Accept")
                onClicked: {
                    root.acceptHandler()
                    root.accept()
                }
            }
            Button {
                visible: !root.acceptHandler
                text: qsTr("Close")
                onClicked: root.reject()
            }
        }
    }

    ScrollView {
        id: scrollView

        anchors { left: parent.left; right: parent.right }
        height: Math.min(textArea.implicitHeight, root.parent.height * 0.75)
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn

        TextArea {
            id: textArea

            wrapMode: Text.Wrap
            readOnly: true
            textFormat: TextEdit.MarkdownText

            BusyIndicator {
                anchors.centerIn:  parent
                running: root.busy
            }
        }
    }
}

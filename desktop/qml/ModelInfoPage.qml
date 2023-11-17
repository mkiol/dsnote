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

    property string name: ""
    property var downloadUrls: []
    property string downloadSize: ""
    property string licenseId: ""
    property string licenseName: ""
    property url licenseUrl: ""

    readonly property bool showLicense: licenseId.length !==0 && licenseName.length !== 0

    title: name

    width: parent.width - 8 * appWin.padding
    height: column.height + footer.height + header.height + root.topPadding + root.bottomPadding
    modal: true
    verticalPadding: appWin.padding
    horizontalPadding: appWin.padding

    onDownloadUrlsChanged: {
        urlsArea.text = downloadUrls.join("\n")
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
                visible: !root.licenseAcceptRequired
                text: qsTr("Close")
                onClicked: root.reject()
            }
        }
    }

    ColumnLayout {
        id: column

        spacing: appWin.padding
        anchors {
            left: parent.left
            right: parent.right
        }

        SectionLabel {
            visible: root.showLicense
            text: qsTr("License")
        }

        Label {
            visible: root.showLicense
            wrapMode: Text.Wrap
            text: root.licenseName.length !== 0 ?
                      root.licenseName + " (" + root.licenseId + ")" : root.licenseId
        }

        Button {
            visible: root.showLicense && root.licenseUrl.toString().length !== 0
            text: qsTr("Show license")
            onClicked: appWin.showModelLicenseDialog(root.licenseId, root.licenseName, root.licenseUrl, null)
        }

        SectionLabel {
            visible: root.downloadUrls.length !== 0
            text: qsTr("Files to download")
        }

        Label {
            id: urlsArea

            visible: root.downloadUrls.length !== 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Label {
            visible: root.downloadUrls.length !== 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Total size: %1").arg("<b>" + root.downloadSize + "</b>");
        }
    }
}

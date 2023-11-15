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

DialogPage {
    id: root

    readonly property bool verticalMode: width < appWin.verticalWidthThreshold

    title: qsTr("Voice samples")

    footer: Item {
        height: closeButton.height + appWin.padding

        Button {
            id: createNewButton

            anchors {
                left: parent.left
                leftMargin: root.leftPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            text: qsTr("Create a new voice sample")
            icon.name: "special-effects-symbolic"
            onClicked: appWin.openDialog("VoiceImportPage.qml")
            Keys.onReturnPressed: appWin.openDialog("VoiceImportPage.qml")
        }

        Button {
            id: closeButton

            anchors {
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            text: qsTr("Close")
            //icon.name: "window-close-symbolic"
            onClicked: root.reject()
            Keys.onReturnPressed: root.reject()
        }
    }

    Component {
        id: voiceDelegate

        Control {
            id: control

            property bool editActive: false

            background: Rectangle {
                id: bg

                anchors.fill: parent
                color: palette.text
                opacity: control.hovered ? 0.1 : 0.0
            }

            width: root.listViewItem.width
            height: deleteButton.height
            onEditActiveChanged: {
                if (!editActive)
                    app.rename_tts_ref_voice(index, editField.text)
            }

            Label {
                id: nameField

                text: modelData
                visible: !control.editActive
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: appWin.padding
                anchors.right: renameButton.left
                anchors.rightMargin: appWin.padding
            }

            TextField {
                id: editField

                text: modelData
                visible: control.editActive
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: appWin.padding
                anchors.right: renameButton.left
                anchors.rightMargin: appWin.padding
            }

            Button {
                id: renameButton
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: deleteButton.left
                anchors.rightMargin: appWin.padding
                width: appWin.buttonSize
                text: control.editActive ? qsTr("Save") : qsTr("Rename")
                onClicked: control.editActive = !control.editActive
            }

            Button {
                id: deleteButton
                icon.name: "edit-delete-symbolic"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: root._rightMargin
                width: appWin.buttonSize
                text: qsTr("Delete")
                onClicked: app.delete_tts_ref_voice(index)
            }
        }
    }

    placeholderLabel {
        text: qsTr("You haven't created voice samples yet.") + " " +
              qsTr("Use %1 to make a new one.").arg("<i>" + createNewButton.text + "</i>")
        enabled: root.listViewItem.model.length === 0
    }

    listViewItem {
        model: app.available_tts_ref_voices
        delegate: voiceDelegate
    }
}

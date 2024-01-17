/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
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

    property var streams
    property int selectedId: 0

    modal: true
    width: Math.min(implicitWidth, parent.width - 2 * appWin.padding)
    height: column.implicitHeight + 2 * verticalPadding + footer.height
    closePolicy: Popup.CloseOnEscape
    clip: true

    onOpened: {
        combo.currentIndex = 0
    }

    header: Item {}

    footer: Item {
        height: closeButton.height + appWin.padding

        RowLayout {
            anchors {
                right: parent.right
                rightMargin: root.rightPadding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.reject()
                Keys.onEscapePressed: root.reject()
            }

            Button {
                text: qsTr("Transcribe selected stream")
                icon.name: "document-open-symbolic"
                Keys.onReturnPressed: root.accept()
                onClicked: root.accept()
            }
        }
    }

    ColumnLayout {
        id: column

        width: parent.width
        spacing: appWin.padding

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("The file contains multiple audio streams. Select which one you want to process.")
            font.pixelSize: appWin.textFontSizeBig
        }

        ComboBox {
            id: combo

            model: root.streams
            onActivated: {
                var name = displayText
                root.selectedId = parseInt(name.substring(name.lastIndexOf("(") + 1, name.lastIndexOf(")")))
            }
        }
    }
}

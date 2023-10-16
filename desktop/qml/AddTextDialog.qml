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

    signal addClicked
    signal replaceClicked

    modal: true
    width: Math.min(implicitWidth, parent.width)
    height: column.implicitHeight + 2 * verticalPadding
    closePolicy: Popup.CloseOnEscape

    ColumnLayout {
        id: column

        width: parent.width
        spacing: appWin.padding

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Should the text be added to the current note or replace it?")
            font.pixelSize: 1.2 * appWin.textFontSize
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter

            Button {
                text: qsTr("Add")
                onClicked: {
                    root.addClicked()
                    root.close()
                }
            }

            Button {
                text: qsTr("Replace")
                onClicked: {
                    root.replaceClicked()
                    root.close()
                }
            }
        }
    }
}

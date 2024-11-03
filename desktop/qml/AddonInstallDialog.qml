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

    modal: true
    width: Math.min(implicitWidth, parent.width - 2 * appWin.padding)
    standardButtons: Dialog.Close

    property bool nvidiaAddon: true

    function openNvidia() {
        nvidiaAddon = true
        open()
    }

    function openAmd() {
        nvidiaAddon = false
        open()
    }

    ColumnLayout {
        id: column

        width: parent.width
        spacing: appWin.padding

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            font.pixelSize: appWin.textFontSizeBig

            text: qsTr("To install the Flatpak add-on, which provides GPU acceleration support for %1 graphics card, follow one of the steps below.")
                  .arg(root.nvidiaAddon ? "NVIDIA" : "AMD")
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.topMargin: appWin.padding
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            font.pixelSize: appWin.textFontSize
            textFormat: Text.RichText

            text: "&rarr; " + qsTr("Use the software manager application on your system and install %2, or")
            .arg(root.nvidiaAddon ? "<i><b>Speech Note NVIDIA</b></i>" : "<i><b>Speech Note AMD</b></i>")
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            font.pixelSize: appWin.textFontSize
            textFormat: Text.RichText

            text: "&rarr; " + qsTr("Run the following command in the terminal:")
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.leftMargin: 4 * appWin.padding
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            textFormat: Text.RichText
            font.pixelSize: appWin.textFontSize

            text: "<code>flatpak install net.mkiol.SpeechNote.Addon." + (root.nvidiaAddon ? "nvidia" : "amd") + "</code>"
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 2 * appWin.padding
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            font.pixelSize: appWin.textFontSize

            text: qsTr("Note that installing the add-on requires a significant amount of disk space.")
        }
    }
}

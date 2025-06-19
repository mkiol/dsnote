/* Copyright (C) 2024-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

HelpDialog {
    id: root

    property bool nvidiaAddon: true

    title: qsTr("Install Flatpak add-on")

    function openNvidia() {
        nvidiaAddon = true
        open()
    }

    function openAmd() {
        nvidiaAddon = false
        open()
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap

        text: qsTr("To install Flatpak add-on, which provides GPU acceleration support for %1 graphics card, follow one of the steps below.")
              .arg(root.nvidiaAddon ? "NVIDIA" : "AMD")
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: appWin.padding
        wrapMode: Text.Wrap
        textFormat: Text.RichText

        text: "&rarr; " + qsTr("Use the software manager application on your system and install %1, or")
        .arg(root.nvidiaAddon ? "<i><b>Speech Note NVIDIA</b></i>" : "<i><b>Speech Note AMD</b></i>")
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        textFormat: Text.RichText

        text: "&rarr; " + qsTr("Run the following command in the terminal:")
    }

    Label {
        Layout.fillWidth: true
        Layout.leftMargin: 4 * appWin.padding
        wrapMode: Text.Wrap
        textFormat: Text.RichText

        text: "<code>flatpak install net.mkiol.SpeechNote.Addon." + (root.nvidiaAddon ? "nvidia" : "amd") + "</code>"
    }

    Label {
        visible: root.nvidiaAddon
        Layout.topMargin: appWin.padding
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        textFormat: Text.RichText
        text: qsTr("The add-on enables faster processing when using the following Speech to Text and Text to Speech engines:") + " " +
              "<ul>" +
              "<li>WhisperCpp STT</li>" +
              "<li>FasterWhisper STT</li>" +
              "<li>Coqui TTS</li>" +
              "<li>F5-TTS</li>" +
              "<li>Kokoro TTS</li>" +
              "<li>Parler-TTS</li>" +
              "<li>WhisperSpeech TTS</li>" +
              "</ul>"
    }

    Label {
        visible: !root.nvidiaAddon
        Layout.topMargin: appWin.padding
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        textFormat: Text.RichText
        text: qsTr("The add-on enables faster processing when using the following Speech to Text and Text to Speech engines:") + " " +
              "<ul>" +
              "<li>WhisperCpp STT</li>" +
              "<li>Coqui TTS</li>" +
              "<li>F5-TTS</li>" +
              "<li>Kokoro TTS</li>" +
              "<li>Parler-TTS</li>" +
              "<li>WhisperSpeech TTS</li>" +
              "</ul>"
    }

    Label {
        Layout.topMargin: appWin.padding
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: qsTr("If you're interested in fast and accurate Speech to Text processing, consider using %1 with Vulkan hardware acceleration, " +
                   "which works without installing an add-on.").arg("<i>WhisperCpp</i>")
    }

    Label {
        Layout.topMargin: appWin.padding
        Layout.fillWidth: true
        wrapMode: Text.Wrap

        text: qsTr("Note that installing the add-on requires a significant amount of disk space.")
    }
}

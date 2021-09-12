/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Settings 1.0

Page {
    id: root

    readonly property bool configured: app.available_langs.length > 0

    title: qsTr("Note")

    ColumnLayout {
        anchors.fill: parent

        Flickable {
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            TextArea {
                anchors.fill: parent
                verticalAlignment: TextEdit.AlignBottom
                text: _settings.note
            }
        }
        RowLayout {
            visible: configured

            Layout.fillWidth: true

            SpeechIndicator {
                width: 20
                Layout.leftMargin: 10
                height: speechText.height / 2
                active: app.speech
                Layout.alignment: Qt.AlignVCenter
                color: palette.text
            }

            TextArea {
                id: speechText
                Layout.fillWidth: true
                readOnly: true
                placeholderText: _settings.speech_mode === Settings.SpeechAutomatic || app.speech ?
                                     qsTr("Say something...") : qsTr("Press and say something...")
                font.italic: true
                text: app.intermediate_text
                leftPadding: 0
            }
        }
    }

    footer: ToolBar {
        contentHeight: toolButton.implicitHeight
        RowLayout {
            anchors.fill: parent

            ToolButton {
                id: toolButton

                visible: _settings.speech_mode === Settings.SpeechManual && configured
                text: qsTr("Press and hold to speek")
                onPressed: app.speech = true
                onReleased: app.speech = false
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                Layout.rightMargin: 10
                visible: !configured
                text: qsTr("No language is configured")
            }

            ComboBox {
                visible: configured
                currentIndex: app.active_lang_idx
                model: app.available_langs
                onActivated: app.set_active_lang_idx(index)
            }
        }
    }
}

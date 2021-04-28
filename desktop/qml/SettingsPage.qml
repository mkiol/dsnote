/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3

import org.mkiol.dsnote.Settings 1.0

Page {
    id: root

    title: qsTr("Settings")

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.height + 20
        clip: true

        ScrollIndicator.vertical: ScrollIndicator {}

        ColumnLayout {
            id: column
            x: 10
            y: 10
            width: parent.width - 20
            spacing: 10

            RowLayout {
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Speech detection mode")
                }
                ComboBox {
                    currentIndex: _settings.speech_mode == Settings.SpeechAutomatic ? 0 : 1
                    model: ListModel {
                        ListElement { text: qsTr("Automatic") }
                        ListElement { text: qsTr("Manual") }
                    }
                    onActivated: _settings.speech_mode = index === 0 ?
                                     Settings.SpeechAutomatic : Settings.SpeechManual
                }
            }

            RowLayout {
                Label {
                    text: qsTr("Location of language files")
                }
                TextField {
                    Layout.fillWidth: true
                    text: _settings.lang_models_dir
                    enabled: false

                }
                Button {
                    text: qsTr("Change")
                    onClicked: fileDialog.open()
                }
            }

            FileDialog {
                id: fileDialog
                title: qsTr("Please choose a directory")
                selectFolder: true
                selectExisting: true
                folder:  _settings.lang_models_dir_url
                onAccepted: {
                    _settings.lang_models_dir_url = fileUrl
                }
            }
        }
    }
}

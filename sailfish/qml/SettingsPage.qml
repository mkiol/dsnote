/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Settings 1.0

Page {
    id: root

    readonly property bool configured: app.available_langs.length > 0

    allowedOrientations: Orientation.All

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: root.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Settings")
            }

            ComboBox {
                id: langCombo
                label: qsTr("Active language")
                visible: configured
                currentIndex: app.active_lang_idx
                menu: ContextMenu {
                    Repeater {
                        model: app.available_langs
                        MenuItem { text: modelData }
                    }
                }

                onCurrentIndexChanged: {
                    app.set_active_lang_idx(currentIndex)
                }

                Connections {
                    target: app
                    onLangs_changed: langCombo.currentIndex = app.active_lang_idx
                }
            }

            Button {
                text: qsTr("Language download")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: pageStack.push(Qt.resolvedUrl("LangsPage.qml"))
            }

            Spacer {}

            ComboBox {
                label: qsTr("Speech detection mode")
                currentIndex: _settings.speech_mode == Settings.SpeechAutomatic ? 0 : 1
                menu: ContextMenu {
                    MenuItem { text: qsTr("Automatic") }
                    MenuItem { text: qsTr("Manual") }
                }
                onCurrentIndexChanged: _settings.speech_mode = currentIndex === 0 ?
                                           Settings.SpeechAutomatic : Settings.SpeechManual
                description: qsTr("Speech is automatically recognized and converted to text (Automatic) " +
                                  "or tap on bottom panel triggers speech recognition (Manual).");
            }

            ItemBox {
                title: qsTr("Location of language files")
                value: _settings.lang_models_dir_name
                description: qsTr("Directory where language files are downloaded to and stored.")

                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Change")
                        onClicked: {
                            var obj = pageStack.push(Qt.resolvedUrl("DirPage.qml"));
                            obj.accepted.connect(function() {
                                _settings.lang_models_dir = obj.selectedPath
                            })
                        }
                    }
                    MenuItem {
                        text: qsTr("Set default")
                        onClicked: {
                            _settings.lang_models_dir = ""
                        }
                    }
                }
            }
        }
    }

    RemorsePopup {
        id: remorse
    }

    VerticalScrollDecorator {
        flickable: flick
    }
}

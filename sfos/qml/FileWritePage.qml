/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: root

    canAccept: nameField.text.trim().length > 0

    Column {
        width: parent.width

        DialogHeader {
            acceptText: qsTr("Save File")
        }

        ItemBox {
            title: qsTr("Folder")
            value: _settings.file_save_dir_name

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Change")
                    onClicked: {
                        var obj = pageStack.push(Qt.resolvedUrl("DirPage.qml"),
                                                 {path: _settings.file_save_dir});
                        obj.accepted.connect(function() {
                            _settings.file_save_dir = obj.selectedPath
                        })
                    }
                }
                MenuItem {
                    text: qsTr("Set default")
                    onClicked: {
                        _settings.file_save_dir = ""
                    }
                }
            }
        }

        TextField {
            id: nameField
            anchors.left: parent.left
            anchors.right: parent.right
            width: parent.width
            placeholderText: qsTr("File name")
            label: qsTr("File name")
            Component.onCompleted: {
                text = _settings.file_save_filename
            }
        }
    }

    onDone: {
        if (result !== DialogResult.Accepted) return

        var file_name = nameField.text.trim()

        if (file_name.indexOf('.') == -1)
            file_name += ".wav"

        var file_path = _settings.file_save_dir + "/" + file_name

        app.speech_to_file(file_path)
    }
}

/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: root

    property alias selectedUrl: urlField.text

    allowedOrientations: Orientation.All

    canAccept: app.is_valid_url(urlField.text.trim())

    onAccepted: {
        appWin.importUrls([selectedUrl])
    }

    onOpened: {
        urlField.text = app.clipboard_if_url()
    }

    SilicaFlickable {
        width: parent.width
        height: parent.height
        contentHeight: column.height
        clip: true

        Column {
            id: column

            width: parent.width

            DialogHeader {
                acceptText: qsTr("Import from a URL")
            }

            TextField {
                id: urlField

                width: parent.width
                placeholderText: qsTr("Enter URL")
                label: qsTr("URL")
                wrapMode: TextInput.WrapAnywhere
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
                description: qsTr("The URL must be of HTTP or HTTPS type and should point to text content.")

                EnterKey.iconSource: root.canAccept ?
                                         "image://theme/icon-m-enter-accept" :
                                         "image://theme/icon-m-enter-next"
                EnterKey.onClicked: {
                    if (root.canAccept) {
                        root.accept()
                    }
                }

                rightItem: IconButton {
                    onClicked: urlField.text = ""
                    width: icon.width
                    height: icon.height
                    icon.source: "image://theme/icon-m-input-clear"
                    opacity: urlField.text.length > 0 ? 1.0 : 0.0
                    Behavior on opacity { FadeAnimation {} }
                }
            }

            TextSwitch {
                checked: _settings.import_extract_readable
                automaticCheck: false
                text: qsTr("When importing an HTML, extract only the readable text")
                onClicked: {
                    _settings.keep_last_note = !_settings.keep_last_note
                }
            }
        }

        VerticalScrollDecorator {}
    }
}

/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

DialogPage {
    id: root

    property alias selectedUrl: urlField.text
    readonly property bool canAccept: app.is_valid_url(urlField.text.trim())

    title: qsTr("Import from a URL")

    onOpened: {
        urlField.text = app.clipboard_if_url()
    }

    onAccepted: {
        appWin.importUrls([selectedUrl])
    }

    header: Item {}

    footer: Item {
        height: closeButton.height + appWin.padding

        RowLayout {
            anchors {
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                Layout.alignment: Qt.AlignRight
                enabled: root.canAccept
                text: qsTr("Import from a URL")
                icon.name: "globe-symbolic"
                Keys.onReturnPressed: {
                    root.accept()
                }
                onClicked: {
                    root.accept()
                }
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.reject()
                Keys.onEscapePressed: root.reject()
            }
        }
    }

    ColumnLayout {
        id: column

        spacing: appWin.padding * 2

        TextField {
            id: urlField

            Layout.fillWidth: true
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
            placeholderText: qsTr("Enter URL")
            Keys.onReturnPressed: {
                if (root.canAccept)
                    root.accept();
            }

            ToolTip.visible: hovered
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.text: qsTr("The URL must be of HTTP or HTTPS type and should point to text content.")
            hoverEnabled: true
        }

        CheckBox {
            checked: _settings.import_extract_readable
            text: qsTr("When importing an HTML, extract only the readable text")
            onCheckedChanged: {
                _settings.import_extract_readable = checked
            }
        }
    }
}

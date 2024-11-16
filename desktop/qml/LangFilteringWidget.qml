/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Dsnote 1.0

RowLayout {
    id: root

    property var lang_model: service.langs_model

    spacing: appWin.padding * 0.5

    Item {
        Layout.fillWidth: true
    }

    Connections {
        target: root.lang_model
        onFilterChanged: langsSearchTextField.text = root.lang_model.filter
    }

    TextField {
        id: langsSearchTextField

        text: root.lang_model.filter
        placeholderText: qsTr("Type to search")
        verticalAlignment: Qt.AlignVCenter
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        Layout.preferredWidth: 2 * appWin.buttonWithIconWidth
        onTextChanged: root.lang_model.filter = text.toLowerCase().trim()
        color: palette.text
        Accessible.name: qsTr("Language search")

        TextContextMenu {}
    }

    Button {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        icon.name: "edit-clear-symbolic"
        text: qsTr("Clear text")
        display: AbstractButton.IconOnly
        onClicked: langsSearchTextField.text = ""

        ToolTip.visible: hovered
        ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
        ToolTip.text: qsTr("Clear text")
        hoverEnabled: true
    }
}

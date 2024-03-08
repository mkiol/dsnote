/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.12
import QtQuick.Controls 2.15

Menu {
    id: root

    property var textArea: parent

    Component.onCompleted: {
        if (!_settings.is_native_style()) {
            textArea.pressed.connect(function(event){
                if (event.button !== Qt.RightButton) return

                root.y = event.y + 1
                root.x = event.x + 1
                root.open()
            })
        }
    }

    MenuItem {
        text: qsTr("Undo")
        enabled: root.textArea.canUndo
        icon.name: "edit-undo-symbolic"
        onClicked: root.textArea.undo()
    }

    MenuItem {
        text: qsTr("Redo")
        enabled: root.textArea.canRedo
        icon.name: "edit-redo-symbolic"
        onClicked: root.textArea.redo()
    }

    MenuSeparator {}

    MenuItem {
        text: qsTr("Cut")
        icon.name: "edit-cut-symbolic"
        enabled: root.textArea.selectedText.length !== 0
        onClicked: root.textArea.cut()
    }

    MenuItem {
        text: qsTr("Copy")
        icon.name: "edit-copy-symbolic"
        enabled: root.textArea.selectedText.length !== 0
        onClicked: root.textArea.copy()
    }

    MenuItem {
        text: qsTr("Paste")
        enabled: root.textArea.canPaste
        icon.name: "edit-paste-symbolic"
        onClicked: root.textArea.paste()
    }

    MenuItem {
        text: qsTr("Delete")
        enabled: root.textArea.selectedText.length !== 0
        icon.name: "edit-delete-symbolic"
        onClicked: root.textArea.remove(root.textArea.selectionStart, root.textArea.selectionEnd)
    }

    MenuSeparator {}

    MenuItem {
        text: qsTr("Select All")
        icon.name: "edit-select-all-symbolic"
        onClicked: root.textArea.selectAll()
    }
}

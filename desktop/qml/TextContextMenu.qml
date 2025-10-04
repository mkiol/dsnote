/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls

Menu {
    id: root

    property var textArea: parent
    property bool disableNative: false

    modal: true

    Component.onCompleted: {
        var is_native = _settings.is_native_style()

        if (is_native && disableNative) {
            for (var i = 0; i < textArea.resources.length; ++i) {
                if (textArea.resources[i].toString().indexOf("TapHandler") >= 0) {
                    textArea.resources[i].acceptedButtons = Qt.LeftButton
                }
            }
        }

        if (!is_native || disableNative) {
            textArea.pressed.connect(function(event){
                if (event.button !== Qt.RightButton) return

                root.y = event.y + 1
                root.x = event.x + 1
                root.open()
            })
        }
    }

    MenuItem {
        action: Action {
            icon.name: "edit-undo-symbolic"
            text: qsTr("Undo")
            shortcut: StandardKey.Undo
        }
        enabled: root.textArea.canUndo
        onTriggered: {
            root.textArea.undo()
        }
    }

    MenuItem {
        action: Action {
            icon.name: "edit-redo-symbolic"
            text: qsTr("Redo")
            shortcut: StandardKey.Redo
        }
        enabled: root.textArea.canRedo
        onTriggered: {
            root.textArea.redo()
        }
    }

    MenuSeparator {}

    MenuItem {
        action: Action {
            icon.name: "edit-cut-symbolic"
            text: qsTr("Cut")
            shortcut: StandardKey.Cut
        }
        enabled: root.textArea.selectedText.length !== 0
        onTriggered: {
            root.textArea.cut()
        }
    }

    MenuItem {
        action: Action {
            icon.name: "edit-copy-symbolic"
            text: qsTr("Copy")
            shortcut: StandardKey.Copy
        }
        enabled: root.textArea.selectedText.length !== 0
        onTriggered: {
            root.textArea.copy()
        }
    }

    MenuItem {
        action: Action {
            icon.name: "edit-paste-symbolic"
            text: qsTr("Paste")
            shortcut: StandardKey.Paste
        }
        enabled: root.textArea.canPaste
        onTriggered: {
            root.textArea.paste()
        }
    }

    MenuItem {
        action: Action {
            icon.name: "edit-delete-symbolic"
            text: qsTr("Delete")
            shortcut: StandardKey.Delete
        }
        enabled: root.textArea.selectedText.length !== 0
        onTriggered: {
            root.textArea.remove(root.textArea.selectionStart, root.textArea.selectionEnd)
        }
    }

    MenuSeparator {}

    MenuItem {
        action: Action {
            icon.name: "edit-select-all-symbolic"
            text: qsTr("Select All")
            shortcut: StandardKey.SelectAll
        }
        onTriggered: {
            root.textArea.selectAll()
        }
    }
}

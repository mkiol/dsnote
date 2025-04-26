/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

import harbour.dsnote.Settings 1.0
import harbour.dsnote.Dsnote 1.0

PullDownMenu {
    id: root

    busy: app.busy || service.busy || app.state !== DsnoteApp.StateIdle

    MenuItem {
        text: qsTr("About %1").arg(APP_NAME)
        onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
    }

    MenuItem {
        text: qsTr("Settings")
        onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
    }

    MenuItem {
        text: qsTr("Languages and Models")
        onClicked: pageStack.push(Qt.resolvedUrl("LangsPage.qml"))
    }

    MenuItem {
        visible: !_settings.translator_mode
        enabled: !app.busy
        text: qsTr("Import from a file")
        onClicked: {
            pageStack.push(fileReadDialog)
        }
    }

    MenuItem {
        enabled: app.note.length !== 0 && !app.busy
        text: qsTr("Export to a file")
        onClicked: {
            pageStack.push(Qt.resolvedUrl("ExportFilePage.qml"), {translated: false})
        }
    }

    MenuItem {
        visible: _settings.translator_mode
        enabled: app.translated_text.length !== 0 && !app.busy
        text: qsTr("Export the translation to a file")
        onClicked: {
            pageStack.push(Qt.resolvedUrl("ExportFilePage.qml"), {translated: true})
        }
    }

    MenuItem {
        text: qsTr("Mode: %1").arg(_settings.translator_mode ? qsTr("Translator") :
                                                               qsTr("Notepad"))
        onClicked: {
            if (!_settings.translator_mode)
                _settings.hint_translator = false
            _settings.translator_mode = !_settings.translator_mode
        }
    }
}

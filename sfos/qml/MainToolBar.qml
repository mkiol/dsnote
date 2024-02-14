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

    busy: app.busy || service.busy ||
          app.state === DsnoteApp.StateTranscribingFile ||
          app.state === DsnoteApp.StateExtractingSubtitles

    MenuItem {
        text: qsTr("About %1").arg(APP_NAME)
        onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
    }

    MenuItem {
        text: qsTr("Settings")
        onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
    }

    MenuItem {
        text: qsTr("Languages")
        onClicked: pageStack.push(Qt.resolvedUrl("LangsPage.qml"))
    }

    MenuItem {
        visible: !_settings.translator_mode && app.stt_configured
        enabled: !_settings.translator_mode &&
                 (app.state === DsnoteApp.StateListeningManual ||
                  app.state === DsnoteApp.StateListeningAuto ||
                  app.state === DsnoteApp.StateListeningSingleSentence ||
                  app.state === DsnoteApp.StateIdle ||
                  app.state === DsnoteApp.StatePlayingSpeech)
        text: qsTr("Import from a file")
        onClicked: {
            pageStack.push(fileReadDialog)
        }
    }

    MenuItem {
        visible: app.tts_configured && (!_settings.translator_mode || app.mnt_configured)
        enabled: app.note.length !== 0 &&
                 (app.state === DsnoteApp.StateListeningManual ||
                  app.state === DsnoteApp.StateListeningAuto ||
                  app.state === DsnoteApp.StateListeningSingleSentence ||
                  app.state === DsnoteApp.StateIdle ||
                  app.state === DsnoteApp.StatePlayingSpeech)
        text: qsTr("Export to audio file")
        onClicked: {
            pageStack.push(Qt.resolvedUrl("FileWritePage.qml"), {translated: false})
        }
    }

    MenuItem {
        visible: _settings.translator_mode && app.mnt_configured && app.tts_configured
        enabled: _settings.translator_mode && app.translated_text.length !== 0 &&
                 app.tts_configured && app.active_tts_model_for_out_mnt.length !== 0 &&
                 (app.state === DsnoteApp.StateListeningManual ||
                  app.state === DsnoteApp.StateListeningAuto ||
                  app.state === DsnoteApp.StateListeningSingleSentence ||
                  app.state === DsnoteApp.StateIdle ||
                  app.state === DsnoteApp.StatePlayingSpeech)
        text: qsTr("Export the translation to audio file")
        onClicked: {
            pageStack.push(Qt.resolvedUrl("FileWritePage.qml"), {translated: true})
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

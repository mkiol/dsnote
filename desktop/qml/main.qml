/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ApplicationWindow {
    id: appWin

    property bool compactMode: appWin.width < 500
    property var _dialogPage

    function openDialog(file) {
        closeDialog()

        var cmp = Qt.createComponent(file)
        if (cmp.status === Component.Ready) {
            var dialog = cmp.createObject(appWin);
            dialog.open()
            _dialogPage = dialog
        }
    }

    function openDialogIfNotOpen(file) {
        if (_dialogPage) return
        openDialog(file)
    }

    function closeDialog() {
        if (_dialogPage) {
            _dialogPage.close()
            _dialogPage = undefined
        }
    }

    function closeHelloDialog() {
        if (_dialogPage && _dialogPage.objectName === "hello")
            closeDialog()
    }

    property int padding: 8

    width: Screen.width / 2
    height: Screen.height / 2

    visible: true

    header: MainToolBar {}

    ColumnLayout {
        anchors.fill: parent
        spacing: appWin.padding

        ScrollView {
            enabled: app.stt_configured || app.tts_configured
            opacity: enabled ? 1.0 : 0.0
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            TextArea {
                id: textArea

                wrapMode: TextEdit.WordWrap
                verticalAlignment: TextEdit.AlignBottom
                text: _settings.note
                onTextChanged: _settings.note = text

                Keys.onUpPressed: scrollBar.decrease()
                Keys.onDownPressed: scrollBar.increase()

                ScrollBar.vertical: ScrollBar { id: scrollBar }
            }
        }

        SpeechWidget {
            enabled: !appWin.compactMode
            visible: enabled
        }

        SpeechWidgetCompact {
            enabled: appWin.compactMode
            visible: enabled
        }
    }

    PlaceholderLabel {
        visible: !app.stt_configured && !app.tts_configured
        text: qsTr("No language has been set.") + " " +
              qsTr("Go to 'Languages' to download language models.")
    }

    ToastNotification {
        id: toast
    }

    SpeechConfig {
        id: service

        onModel_download_finished: toast.show(qsTr("The model download is complete!"))
        onModel_download_error: toast.show(qsTr("Error: Couldn't download the model file."))
    }

    function showWelcome() {
        if (!app.busy && !app.stt_configured && !app.tts_configured) {
            appWin.openDialog("HelloPage.qml")
        } else if (APP_VERSION !== _settings.prev_app_ver) {
            _settings.prev_app_ver = APP_VERSION
            appWin.openDialogIfNotOpen("ChangelogPage.qml")
        } else {
            appWin.closeHelloDialog()
        }
    }

    DsnoteApp {
        id: app

        onBusyChanged: showWelcome()
        onStt_configuredChanged: showWelcome()
        onTts_configuredChanged: showWelcome()
        Component.onCompleted: showWelcome()

        onNote_copied: toast.show(qsTr("Copied!"))
        onTranscribe_done: toast.show(qsTr("File transcription is complete!"))
        onSpeech_to_file_done: toast.show(qsTr("Speech saved to audio file!"))
        onError: {
            switch (type) {
            case DsnoteApp.ErrorFileSource:
                toast.show(qsTr("Error: Audio file processing has failed."))
                break;
            case DsnoteApp.ErrorMicSource:
                toast.show(qsTr("Error: Couldn't access Microphone."))
                break;
            case DsnoteApp.ErrorSttEngine:
                toast.show(qsTr("Error: Speech to Text engine initialization has failed."))
                break;
            case DsnoteApp.ErrorTtsEngine:
                toast.show(qsTr("Error: Text to Speech engine initialization has failed."))
                break;
            default:
                toast.show(qsTr("Error: An unknown problem has occurred."))
            }
        }
    }
}

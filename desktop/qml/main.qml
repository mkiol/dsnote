/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

import org.mkiol.dsnote.Dsnote 1.0
import org.mkiol.dsnote.Settings 1.0

ApplicationWindow {
    id: appWin

    readonly property int padding: 8
    readonly property double verticalWidthThreshold: 600
    readonly property bool canCancelStt: app.state === DsnoteApp.StateTranscribingFile ||
                                app.state === DsnoteApp.StateListeningSingleSentence ||
                                app.state === DsnoteApp.StateListeningAuto ||
                                app.state === DsnoteApp.StateListeningManual
    readonly property bool canCancelTts: app.state === DsnoteApp.StatePlayingSpeech ||
                                app.state === DsnoteApp.StateWritingSpeechToFile
    readonly property alias textFontSize: _dummyTextField.font.pixelSize
    readonly property alias buttonSize: _dummyButton.width

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

    function showWelcome() {
        if (!app.busy && !app.stt_configured && !app.tts_configured) {
            appWin.openDialogIfNotOpen("HelloPage.qml")
        } else if (APP_VERSION !== _settings.prev_app_ver) {
            _settings.prev_app_ver = APP_VERSION
            appWin.openDialogIfNotOpen("ChangelogPage.qml")
        } else {
            appWin.closeHelloDialog()
        }
    }

    function update() {
        notepad.update()
        translator.update()
    }

    width: Screen.width / 2
    height: Screen.height / 2
    visible: true
    header: MainToolBar {}

    TextField {
        id: _dummyTextField
        visible: false
        height: 0
        Component.onCompleted: console.log("default font pixel size:", font.pixelSize)
    }

    FontMetrics {
        id: fontMetrics
    }

    Button {
        id: _dummyButton
        visible: false
        width: implicitWidth
        height: 0
        text: {
            var list = [
                qsTr("Cancel"),
                qsTr("Delete"),
                qsTr("Download"),
                qsTr("Read"),
                qsTr("Listen")
            ]

            list.sort(function (a, b) {
                var aw = fontMetrics.boundingRect(a).width
                var bw = fontMetrics.boundingRect(b).width
                if (aw > bw) return -1
                if (aw < bw) return 1
                return 0
            })

            console.log("_dummyButton:",list[0])

            return list[0]
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: appWin.padding

        Translator {
            id: translator

            Layout.fillWidth: true
            enabled: _settings.translator_mode
            readOnly: appWin.canCancelStt || appWin.canCancelTts
        }

        Notepad {
            id: notepad

            Layout.fillWidth: true
            enabled: !_settings.translator_mode
            readOnly: appWin.canCancelStt || appWin.canCancelTts
        }

        SpeechWidget {
            id: panel

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
        }
    }

    PlaceholderLabel {
        enabled: !_settings.translator_mode && !app.stt_configured && !app.tts_configured && app.note.length === 0
        offset: -panel.height
        text: notepad.placeholderText
        color: notepad.noteTextArea.textArea.color
    }

    ToastNotification {
        id: toast
    }

    Connections {
        target: _settings
        onTranslator_modeChanged: appWin.update()
    }

    SpeechConfig {
        id: service

        onModel_download_finished: toast.show(qsTr("The model download is complete!"))
        onModel_download_error: toast.show(qsTr("Error: Couldn't download the model file."))
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
            case DsnoteApp.ErrorMntEngine:
                toast.show(qsTr("Error: Translation engine initialization has failed."))
                break;
            default:
                toast.show(qsTr("Error: An unknown problem has occurred."))
            }
        }
    }
}

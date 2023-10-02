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

    function openDialog(file, props) {
        closeDialog()

        var cmp = Qt.createComponent(file)
        if (cmp.status === Component.Ready) {
            var dialog = props ? cmp.createObject(appWin, props) : cmp.createObject(appWin);
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

    function update_dektop_notification() {
        var policy = false
        switch(_settings.desktop_notification_policy) {
        case Settings.DesktopNotificationNever: policy = false; break
        case Settings.DesktopNotificationWhenInacvtive: policy = !active; break
        case Settings.DesktopNotificationAlways: policy = true; break
        }

        if (policy && (
            app.state === DsnoteApp.StateListeningSingleSentence ||
            app.state === DsnoteApp.StateListeningAuto ||
            app.state === DsnoteApp.StateListeningManual)) {
            var text_summary
            if (app.task_state === DsnoteApp.TaskStateInitializing)
                text_summary = qsTr("Getting ready, please wait...")
            else if (app.task_state === DsnoteApp.TaskStateProcessing)
                text_summary = qsTr("Processing, please wait...")
            else
                text_summary = qsTr("Say something...")

            var text_body
            if (app.intermediate_text.length !== 0)
                text_body = app.intermediate_text
            else
                text_body = ""

            app.show_desktop_notification(text_summary, text_body, true)
        } else {
            app.close_desktop_notification()
        }
    }

    width: Screen.width / 2
    height: Screen.height / 2
    visible: true
    header: MainToolBar {}
    onActiveChanged: update_dektop_notification()

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
            readOnly: appWin.canCancelStt
        }

        Notepad {
            id: notepad

            Layout.fillWidth: true
            enabled: !_settings.translator_mode
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
        onRestart_required_changed: {
            if (_settings.restart_required)
                toast.show(qsTr("Restart the application to apply changes."))
        }
    }

    Connections {
        target: _app_server
        onActivate_requested: appWin.raise()
        onAction_requested: app.execute_action_name(action_name)
        onFiles_to_open_requested: {
            app.open_files(files)
            appWin.raise()
        }
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
        Component.onCompleted: {
            app.open_files(_files_to_open)
            app.execute_action_name(_requested_action)
            showWelcome()
        }

        onNote_copied: toast.show(qsTr("Copied!"))
        onTranscribe_done: toast.show(qsTr("File transcription is complete!"))
        onSpeech_to_file_done: toast.show(qsTr("Speech saved to audio file!"))
        onSave_note_to_file_done: toast.show(qsTr("Note saved to text file!"))
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
            case DsnoteApp.ErrorSaveNoteToFile:
                toast.show(qsTr("Error: Couldn't save to the file."))
                break;
            case DsnoteApp.ErrorLoadNoteFromFile:
                toast.show(qsTr("Error: Couldn't open the file."))
                break;
            default:
                toast.show(qsTr("Error: An unknown problem has occurred."))
            }
        }
        onStateChanged: update_dektop_notification()
        onTask_state_changed: update_dektop_notification()
        onIntermediate_text_changed: update_dektop_notification()
    }
}

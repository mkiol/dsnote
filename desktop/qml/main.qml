/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
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
    readonly property double textFontSizeBig: textFontSize * 1.2
    readonly property double textFontSizeSmall: textFontSize * 0.8
    readonly property alias buttonWidth: _dummyButton.width
    readonly property alias buttonWithIconWidth: _dummyButtonWithIcon.width
    readonly property alias buttonHeight: _dummyButton.height
    readonly property double buttonHeightShort: buttonHeight * 0.8

    property var _dialogPage

    function openFile(path) {
        if (app.note.length > 0 && _settings.file_import_action === Settings.FileImportActionAsk) {
            addTextDialog.addHandler = function(){app.import_file(path, -1, false)}
            addTextDialog.replaceHandler = function(){app.import_file(path, -1, true)}
            addTextDialog.open()
        } else {
            app.import_file(path, -1, _settings.file_import_action === Settings.FileImportActionReplace)
        }

        _settings.file_open_dir = _settings.dir_of_file(path)
    }

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
        if (app.busy) return

        if (!app.stt_configured && !app.tts_configured && !app.mnt_configured) {
            appWin.openDialogIfNotOpen("HelloPage.qml")
        } else if (APP_VERSION !== _settings.prev_app_ver) {
            _settings.prev_app_ver = APP_VERSION
            appWin.openDialogIfNotOpen("ChangelogPage.qml")
        } else {
            appWin.closeHelloDialog()
        }
    }

    function showModelLicenseDialog(licenseId, licenseName, licenseUrl, acceptHandler) {
        modelLicenseLoader.setSource("ModelLicensePage.qml", {
                                      licenseId: licenseId,
                                      licenseName: licenseName,
                                      licenseUrl: licenseUrl,
                                      acceptHandler: acceptHandler
                                  })
    }

    function showModelInfoDialog(model) {
        modelInfoLoader.setSource("ModelInfoPage.qml", {model: model})
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

        if (!policy) {
            app.close_desktop_notification()
            return
        }

        if (app.state === DsnoteApp.StateListeningSingleSentence ||
            app.state === DsnoteApp.StateListeningAuto ||
            app.state === DsnoteApp.StateListeningManual ||
            app.state === DsnoteApp.StatePlayingSpeech) {
            var text_summary = ""
            if (app.task_state === DsnoteApp.TaskStateInitializing)
                text_summary = qsTr("Getting ready, please wait...")
            else if (app.task_state === DsnoteApp.TaskStateProcessing)
                text_summary = qsTr("Processing, please wait...")
            else if (app.task_state === DsnoteApp.TaskStateSpeechPlaying)
                text_summary = qsTr("Reading a note...")
            else if (app.task_state === DsnoteApp.TaskStateSpeechPaused)
                text_summary = qsTr("Reading is paused.")
            else if (app.state === DsnoteApp.StateListeningSingleSentence || app.state === DsnoteApp.StateListeningManual)
                    if (app.task_state === DsnoteApp.TaskStateSpeechDetected)
                        text_summary = qsTr("Say something...")
            else if (app.state === DsnoteApp.StateListeningAuto)
                text_summary = qsTr("Say something...")

            if (text_summary.length === 0) {
                app.close_desktop_notification()
                return
            }

            var text_body = ""
            if (_settings.desktop_notification_details && app.intermediate_text.length !== 0)
                text_body = app.intermediate_text

            app.show_desktop_notification(text_summary, text_body, true)
        }
    }

    Component.onCompleted: {
        var hidded =  _start_in_tray || (_settings.start_in_tray && _settings.use_tray)
        visible = !hidded;
    }

    width: Screen.width / 2
    height: Screen.height / 2
    visible: false
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
        text: {
            var list = [
                qsTr("Cancel"),
                qsTr("Delete"),
                qsTr("Download"),
                qsTr("Read"),
                qsTr("Listen"),
                qsTr("Enable"),
                qsTr("Disable"),
                qsTr("Stop"),
                qsTr("Start")
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

    Button {
        id: _dummyButtonWithIcon

        visible: false
        icon.name: "media-playback-stop-symbolic"
        text: _dummyButton.text
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

    Loader {
        id: modelInfoLoader

        anchors.fill: parent
        onLoaded: {
            item.anchors.centerIn = modelInfoLoader
            item.open()
            item.onRejected.connect(function(){modelInfoLoader.source = ""});
            item.onAccepted.connect(function(){modelInfoLoader.source = ""});
        }
    }

    Loader {
        id: modelLicenseLoader

        anchors.fill: parent
        onLoaded: {
            item.anchors.centerIn = modelLicenseLoader
            item.open()
            item.onRejected.connect(function(){modelLicenseLoader.source = ""});
            item.onAccepted.connect(function(){modelLicenseLoader.source = ""});
        }
    }

    AddTextDialog {
        id: addTextDialog

        property var addHandler
        property var replaceHandler

        anchors.centerIn: parent

        onAddClicked: addHandler()
        onReplaceClicked: replaceHandler()
    }

    StreamSelectionDialog {
        id: streamSelectionDialog

        property string filePath
        property string replace

        anchors.centerIn: parent

        onAccepted: {
            app.import_file(filePath, selectedIndex, replace)
        }
    }

    DropArea {
        anchors.fill: parent

        onDropped: {
            if (!drop.hasUrls) return

            if (app.note.length > 0 && _settings.file_import_action === Settings.FileImportActionAsk) {
                var urls = drop.urls
                addTextDialog.addHandler = function(){app.import_files_url(urls, false)}
                addTextDialog.replaceHandler = function(){app.import_files_url(urls, true)}
                addTextDialog.open()
            } else {
                app.import_files_url(drop.urls, _settings.file_import_action === Settings.FileImportActionReplace)
            }
        }
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
        onActivate_requested: {
            appWin.show()
            appWin.raise()
        }
        onAction_requested: app.execute_action_name(action_name)
        onFiles_to_open_requested: {
            if (app.note.length > 0 && _settings.file_import_action === Settings.FileImportActionAsk) {
                var list_of_files = files
                addTextDialog.addHandler = function(){app.import_files(list_of_files, false)}
                addTextDialog.replaceHandler = function(){app.import_files(list_of_files, true)}
                addTextDialog.open()
            } else {
                app.import_files(files, _settings.file_import_action === Settings.FileImportActionReplace)
            }

            appWin.show()
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

        onTray_activated: appWin.visible ? appWin.hide() : appWin.show()
        onBusyChanged: showWelcome()
        onStt_configuredChanged: showWelcome()
        onTts_configuredChanged: showWelcome()
        Component.onCompleted: {
            if (_start_in_tray) show_tray()
            if (_files_to_open.length > 0) {
                appWin.show()

                if (app.note.length > 0 && _settings.file_import_action === Settings.FileImportActionAsk) {
                    var list_of_files = _files_to_open
                    addTextDialog.addHandler = function(){app.import_files(list_of_files, false)}
                    addTextDialog.replaceHandler = function(){app.import_files(list_of_files, true)}
                    addTextDialog.open()
                } else {
                    app.import_files(_files_to_open, _settings.file_import_action === Settings.FileImportActionReplace)
                }
            }
            app.execute_action_name(_requested_action)
            app.set_app_window(appWin);
            showWelcome()
        }

        onNote_copied: toast.show(qsTr("Copied!"))
        onTranscribe_done: toast.show(qsTr("Import from the file is complete!"))
        onSave_note_to_file_done: toast.show(qsTr("Export to file is complete!"))
        onText_decoded_to_clipboard: {
            var policy = false
            switch(_settings.desktop_notification_policy) {
            case Settings.DesktopNotificationNever: policy = false; break
            case Settings.DesktopNotificationWhenInacvtive: policy = !active; break
            case Settings.DesktopNotificationAlways: policy = true; break
            }

            if (policy)
                app.show_desktop_notification(qsTr("Text copied to clipboard!"), "", false)
        }
        onImport_file_multiple_streams: {
            streamSelectionDialog.filePath = file_path
            streamSelectionDialog.streams = streams
            streamSelectionDialog.replace = replace
            streamSelectionDialog.open()
        }

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
            case DsnoteApp.ErrorMntRuntime:
                toast.show(qsTr("Error: Not all text has been translated."))
                break;
            case DsnoteApp.ErrorExportFileGeneral:
                toast.show(qsTr("Error: Couldn't export to the file."))
                break;
            case DsnoteApp.ErrorImportFileGeneral:
                toast.show(qsTr("Error: Couldn't import the file."))
                break;
            case DsnoteApp.ErrorImportFileNoStreams:
                toast.show(qsTr("Error: Couldn't import. The file does not contain audio or subtitles."))
                break;
            case DsnoteApp.ErrorSttNotConfigured:
                toast.show(qsTr("Error: Speech to Text model has been set up yet."))
                break;
            case DsnoteApp.ErrorTtsNotConfigured:
                toast.show(qsTr("Error: Text to Speech model has been set up yet."))
                break;
            case DsnoteApp.ErrorMntNotConfigured:
                toast.show(qsTr("Error: Translator model has been set up yet."))
                break;
            case DsnoteApp.ErrorContentDownload:
                toast.show(qsTr("Error: Couldn't download a licence."))
                break;
            default:
                toast.show(qsTr("Error: An unknown problem has occurred."))
            }
        }
        onStateChanged: update_dektop_notification()
        onTask_state_changed: update_dektop_notification()
        onIntermediate_text_changed: update_dektop_notification()
        onActivate_requested: {
            appWin.show()
            appWin.raise()
        }
    }
}

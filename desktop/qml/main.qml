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
    readonly property bool canCancelStt: app.state === DsnoteApp.StateTranscribingFile ||
                                app.state === DsnoteApp.StateListeningSingleSentence ||
                                app.state === DsnoteApp.StateListeningAuto ||
                                app.state === DsnoteApp.StateListeningManual
    readonly property bool canCancelTts: app.state === DsnoteApp.StatePlayingSpeech ||
                                app.state === DsnoteApp.StateWritingSpeechToFile ||
                                app.state === DsnoteApp.StateRestoringText
    readonly property alias textFontSize: _dummyTextField.font.pixelSize
    readonly property double textFontSizeBig: textFontSize * 1.2
    readonly property double textFontSizeSmall: textFontSize * 0.8
    readonly property alias buttonWidth: _dummyButton.width
    readonly property alias buttonWithIconWidth: _dummyButtonWithIcon.width
    readonly property alias buttonHeight: _dummyButton.height
    readonly property double buttonHeightShort: buttonHeight * 0.8
    property var features: app.features_availability()

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

    function openPopup(comp) {
        popupLoader.sourceComponent = comp
    }
    function openPopupFile(file, props) {
        popupLoader.setSource(file, props)
    }
    function closePopup() {
        if (popupLoader.item) popupLoader.item.close()
        popupLoader.sourceComponent = undefined
        popupLoader.source = ""
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

    function showRefHelpIfNeeded() {
        var ref_voice_needed = (app.tts_ref_voice_needed && !_settings.translator_mode) ||
                ((app.tts_for_in_mnt_ref_voice_needed || app.tts_for_out_mnt_ref_voice_needed) && _settings.translator_mode)
        var ref_prompt_needed = (app.tts_ref_prompt_needed && !_settings.translator_mode) ||
                ((app.tts_for_in_mnt_ref_prompt_needed || app.tts_for_out_mnt_ref_prompt_needed) && _settings.translator_mode)

        if (ref_voice_needed && ((_settings.hint_done_flags & Settings.HintDoneRefVoiceHelp) == 0 || app.available_tts_ref_voices.length === 0)) {
            appWin.openPopup(refVoiceHelpMessage)
            _settings.set_hint_done(Settings.HintDoneRefVoiceHelp)
        }
        if (ref_prompt_needed && ((_settings.hint_done_flags & Settings.HintDoneRefPromptHelp) == 0 || _settings.tts_voice_prompts.length === 0)) {
            appWin.openPopup(refPromptHelpMessage)
            _settings.set_hint_done(Settings.HintDoneRefPromptHelp)
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

    Component {
        id: refVoiceHelpMessage

        HelpDialog {
            title: qsTr("Audio sample")

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("The selected model supports voice cloning. Create an %1 to clone someone's voice.").arg("<i>" + qsTr("Audio sample") + "</i>")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("You can make a new %1 in the %2 menu.").arg("<i>" + qsTr("Audio sample") + "</i>").arg("<i>" + qsTr("Voice profiles") + "</i>")
            }
        }
    }

    Component {
        id: refPromptHelpMessage

        HelpDialog {
            title: qsTr("Text voice profile")

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("The selected model supports the voice characteristics defined in the text description.")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("You can make a new %1 in the %2 menu.").arg("<i>" + qsTr("Text voice profile") + "</i>").arg("<i>" + qsTr("Voice profiles") + "</i>")
            }
        }
    }

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
        id: padColumn

        anchors.fill: parent
        spacing: 0

        readonly property bool showingTip: warningTip1.visible || warningTip2.visible ||
                                           warningTip3.visible || warningTip4.visible ||
                                           warningTip5.visible || warningTip6.visible

        MainTipMessage {
            id: warningTip1

            warning: true
            visible: _settings.error_flags & Settings.ErrorMoreThanOneGpuAddons
            text: qsTr("Both %1 and %2 GPU acceleration add-ons are installed, which is not optimal. Uninstall one of them.")
                      .arg("NVIDIA")
                      .arg("AMD")
        }

        MainTipMessage {
            id: warningTip5

            warning: true
            visible: _settings.error_flags & Settings.ErrorIncompatibleNvidiaGpuAddon
            text: qsTr("This version of %1 is not compatible with the installed %2 GPU acceleration add-on.")
                  .arg("<i>Speech Note</i>")
                  .arg("NVIDIA") + " " +
                  qsTr("The required version of the add-on is %1.").arg(APP_ADDON_VERSION)
        }

        MainTipMessage {
            id: warningTip6

            warning: true
            visible: _settings.error_flags & Settings.ErrorIncompatibleAmdGpuAddon
            text: qsTr("This version of %1 is not compatible with the installed %2 GPU acceleration add-on.")
                .arg("<i>Speech Note</i>")
                .arg("AMD") + " " +
                qsTr("The required version of the add-on is %1.").arg(APP_ADDON_VERSION)
        }

        MainTipMessage {
            id: warningTip2

            warning: false
            // visible: _settings.hw_accel_supported() && _settings.is_flatpak() &&
            //          _settings.addon_flags == Settings.AddonNone &&
            //          ((_settings.hint_done_flags & Settings.HintDoneAddon) == 0) &&
            //          (((_settings.system_flags & Settings.SystemAmdGpu) > 0) ||
            //           ((_settings.system_flags & Settings.SystemNvidiaGpu) > 0))

            // right now, ROCm 6.x causes many problems with Coqui TTS therefore
            // "missing add-on" warning for AMD GPU is not shown
            visible: _settings.hw_accel_supported() && _settings.is_flatpak() &&
                     _settings.addon_flags == Settings.AddonNone &&
                     ((_settings.hint_done_flags & Settings.HintDoneAddon) == 0) &&
                     ((_settings.system_flags & Settings.SystemNvidiaGpu) > 0)
            onCloseClicked: _settings.set_hint_done(Settings.HintDoneAddon)
            text: {
                var nvidia_addon = (_settings.system_flags & Settings.SystemNvidiaGpu) > 0
                var amd_addon = (_settings.system_flags & Settings.SystemAmdGpu) > 0
                var txt = ""
                if (nvidia_addon && amd_addon) {
                    txt = qsTr("Both %1 and %2 graphics cards have been detected.").arg("NVIDIA").arg("AMD")
                } else {
                    txt = qsTr("%1 graphics card has been detected.").arg(nvidia_addon ? "NVIDIA" : "AMD")
                }
                txt += " " + qsTr("To add GPU acceleration support, install the additional Flatpak add-on.")
            }

            actionButtonToolTip: qsTr("Click to see instructions for installing the add-on.")
            actionButton {
                text: qsTr("Install")
                onClicked: {
                    var nvidia_addon = (_settings.system_flags & Settings.SystemNvidiaGpu) > 0;
                    appWin.openPopupFile("AddonInstallDialog.qml", {nvidiaAddon: nvidia_addon})
                }
            }
        }

        MainTipMessage {
            id: warningTip4

            warning: false
            visible: _settings.hw_accel_supported() &&
                     ((_settings.system_flags & Settings.SystemHwAccel) > 0) &&
                     ((_settings.hint_done_flags & Settings.HintDoneHwAccel) == 0) &&
                     !_settings.whispercpp_use_gpu && !_settings.fasterwhisper_use_gpu &&
                     !_settings.coqui_use_gpu && !_settings.whisperspeech_use_gpu &&
                     (((_settings.system_flags & Settings.SystemAmdGpu) > 0) ||
                      ((_settings.system_flags & Settings.SystemNvidiaGpu) > 0))
            onCloseClicked: _settings.set_hint_done(Settings.HintDoneHwAccel)
            text: qsTr("To speed up processing, enable hardware acceleration in the settings.")
        }

        MainTipMessage {
            id: warningTip3

            warning: true
            visible: _settings.hw_accel_supported() &&
                     ((app.feature_whispercpp_gpu && _settings.whispercpp_use_gpu) ||
                      (app.feature_fasterwhisper_gpu && _settings.fasterwhisper_use_gpu)) &&
                     ((_settings.error_flags & Settings.ErrorCudaUnknown) > 0)
            text: qsTr("Most likely, %1 kernel module has not been fully initialized.").arg("NVIDIA") + " " +
                  qsTr("Try executing %1 before running Speech Note.")
                      .arg("<i>\"nvidia-modprobe -c 0 -u\"</i>")
        }

        Translator {
            id: translator

            Layout.topMargin: appWin.padding
            Layout.fillWidth: true
            enabled: _settings.translator_mode
            readOnly: appWin.canCancelStt
        }

        Notepad {
            id: notepad

            Layout.topMargin: appWin.padding
            Layout.fillWidth: true
            enabled: !_settings.translator_mode
        }

        Item {
            height: 5
        }

        SpeechWidget {
            id: panel

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
        }
    }

    PlaceholderLabel {
        enabled: !_settings.translator_mode && !app.stt_configured &&
                 !app.tts_configured && app.note.length === 0
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

    Loader {
        id: popupLoader

        anchors.centerIn: parent
        anchors.fill: parent
        onLoaded: {
            item.onClosed.connect(function(){
                popupLoader.sourceComponent = undefined
                popupLoader.source = ""
            });
            item.open()
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
            // register app in dbus server
            _app_server.setDsnoteApp(app)

            if (_start_in_tray) show_tray()

            app.set_app_window(appWin);

            showWelcome()
        }

        onNote_copied: toast.show(qsTr("Copied!"))
        onTranscribe_done: toast.show(qsTr("Import from the file is complete!"))
        onSave_note_to_file_done: toast.show(qsTr("Export to file is complete!"))
        onText_repair_done: toast.show(qsTr("Text repair is complete!"))
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

        onStt_auto_lang_changed: {
            if (stt_auto_lang_name.length > 0)
                toast.show(stt_auto_lang_name)
        }

        onFeatures_availability_updated: {
            appWin.features = app.features_availability()
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
            case DsnoteApp.ErrorTextRepairEngine:
                toast.show(qsTr("Error: Couldn't repair the text."))
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
                toast.show(qsTr("Error: Speech to Text model has not been set up yet."))
                break;
            case DsnoteApp.ErrorTtsNotConfigured:
                toast.show(qsTr("Error: Text to Speech model has not been set up yet."))
                break;
            case DsnoteApp.ErrorMntNotConfigured:
                toast.show(qsTr("Error: Translator model has not been set up yet."))
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
        onTts_ref_voice_neededChanged: appWin.showRefHelpIfNeeded()
        onTts_for_in_mnt_ref_voice_neededChanged: appWin.showRefHelpIfNeeded()
        onTts_for_out_mnt_ref_voice_neededChanged: appWin.showRefHelpIfNeeded()
        onTts_ref_prompt_neededChanged: appWin.showRefHelpIfNeeded()
        onTts_for_in_mnt_ref_prompt_neededChanged: appWin.showRefHelpIfNeeded()
        onTts_for_out_mnt_ref_prompt_neededChanged: appWin.showRefHelpIfNeeded()
    }
}

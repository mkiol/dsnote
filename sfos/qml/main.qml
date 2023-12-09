/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import Sailfish.Share 1.0

import harbour.dsnote.Settings 1.0
import harbour.dsnote.Dsnote 1.0

ApplicationWindow {
    id: appWin

    allowedOrientations: Orientation.All
    initialPage: mainPage
    cover: Qt.resolvedUrl("CoverPage.qml")

    function handleResource(resource) {
        console.log("share request received");
        if (resource.type === ShareResource.FilePathType) {
            app.open_files(resource.filePath, false)
            appWin.activate()
        } else if (resource.type === ShareResource.StringDataType) {
            app.update_note(resource.data, false)
            appWin.activate()
        } else {
            console.warn("unknown resource type:", resource.type)
        }
    }

    ShareProvider {
        method: "files"
        capabilities: [
            "audio/aac",
            "audio/mpeg",
            "audio/x-mp3",
            "audio/ogg",
            "audio/vorbis",
            "audio/x-vorbis",
            "audio/opus",
            "audio/x-speex",
            "audio/speex",
            "audio/wav",
            "audio/x-wav",
            "audio/flac",
            "audio/x-flac",
            "audio/x-matroska",
            "audio/webm",
            "audio/mp4",
            "video/mpeg",
            "video/mp4",
            "video/ogg",
            "video/x-matroska",
            "video/webm",
            "text/plain"
        ]

        onTriggered: {
            for(var i = 0; i < resources.length; i++)
                handleResource(resources[i])
        }
    }

    Connections {
        target: _app_server
        onActivate_requested: appWin.activate()
        onFiles_to_open_requested: app.open_files(_files_to_open, false)
    }

    SpeechConfig {
        id: service
    }

    DsnoteApp {
        id: app
    }

    Component {
        id: mainPage

        Page {
            id: root

            readonly property bool verticalMode: isPortrait
            readonly property bool canCancelStt: !app.another_app_connected &&
                                                 (app.state === DsnoteApp.StateTranscribingFile ||
                                                  app.state === DsnoteApp.StateListeningSingleSentence ||
                                                  app.state === DsnoteApp.StateListeningAuto ||
                                                  app.state === DsnoteApp.StateListeningManual)
            readonly property bool canCancelTts: !app.another_app_connected &&
                                                 (app.state === DsnoteApp.StatePlayingSpeech ||
                                                  app.state === DsnoteApp.StateWritingSpeechToFile)
            readonly property bool panelAlwaysOpen: notepad.enabled && (app.stt_configured || app.tts_configured)

            allowedOrientations: Orientation.All

            function update() {
                notepad.update()
                translator.update()
            }

            onOrientationChanged: update()

            Component.onCompleted: {
                if (APP_VERSION !== _settings.prev_app_ver) {
                    console.log("new version detected")
                    _settings.prev_app_ver = APP_VERSION
                }
            }

            SilicaFlickable {
                id: flick

                width: parent.width
                height: parent.height
                contentHeight: Math.max(column.height +
                                        (translator.visible ? translator.height : 0) +
                                        (notepad.visible ? notepad.height : 0) +
                                        (panel.open ? panel.height : 0), height)
                clip: true

                Column {
                    id: column

                    width: parent.width
                    spacing: Theme.paddingLarge

                    MainToolBar {
                        id: menu
                    }
                }

                Translator {
                    id: translator

                    enabled: _settings.translator_mode
                    visible: opacity > 0.0
                    opacity: enabled ? root.canCancelStt || root.canCancelTts ? 0.8 : 1.0 : 0.0
                    Behavior on opacity { FadeAnimator { duration: 100 } }
                    maxHeight: root.height - (panel.open ? panel.height : 0)
                    verticalMode: root.verticalMode
                    width: parent.width
                    readOnly: !app.another_app_connected &&
                              (app.busy || service.busy || !app.connected)
                }

                Notepad {
                    id: notepad

                    enabled: !_settings.translator_mode
                    visible: opacity > 0.0
                    opacity: enabled ? root.canCancelStt || root.canCancelTts ? 0.8 : 1.0 : 0.0
                    Behavior on opacity { FadeAnimator { duration: 100 } }
                    maxHeight: root.height - (panel.open ? panel.height : 0)
                    verticalMode: root.verticalMode
                    width: parent.width
                    readOnly: !app.another_app_connected &&
                              (app.busy || service.busy || !app.connected)
                }

                SpeechWidget {
                    id: panel

                    property bool open: !app.connected ||
                                        (app.state !== DsnoteApp.StateIdle && app.state !== DsnoteApp.StateTranslating) ||
                                        root.panelAlwaysOpen
                    width: parent.width
                    enabled: open
                    y: open ? parent.height - height : parent.height
                    canCancel: app.connected && !app.busy && (root.canCancelStt || root.canCancelTts)
                    canPause: app.state === DsnoteApp.StatePlayingSpeech &&
                              (app.task_state === DsnoteApp.TaskStateProcessing ||
                               app.task_state === DsnoteApp.TaskStateSpeechPlaying ||
                               app.task_state === DsnoteApp.TaskStateSpeechPaused)
                    canStop: app.connected && !app.busy &&
                                 app.task_state !== DsnoteApp.TaskStateProcessing &&
                                 app.task_state !== DsnoteApp.TaskStateInitializing &&
                                 app.state === DsnoteApp.StateListeningSingleSentence
                    onCancelClicked: app.cancel()
                    onPauseClicked: app.pause_speech()
                    onResumeClicked: app.resume_speech()
                    onStopClicked: app.stop_listen()
                }

                BusyIndicator {
                    size: BusyIndicatorSize.Large
                    anchors.centerIn: parent
                    running: app.busy || service.busy || !app.connected
                }

                VerticalScrollDecorator {
                    flickable: flick
                }
            }

            HintLabel {
                enabled: _settings.hint_translator
                text: qsTr("To switch between %1 and %2 modes use option in pull-down menu.")
                .arg("<i>" + qsTr("Notepad") + "</i>").arg("<i>" + qsTr("Translator") + "</i>")
                onClicked: _settings.hint_translator = false
            }

            Component {
                id: fileReadDialog

                FilePickerPage {
                    nameFilters: [ '*.wav', '*.mp3', '*.ogg', '*.oga', '*.opus', '*.spx', '*.flac', '*.m4a', '*.aac', '*.mp4', '*.mkv', '*.ogv', '*.webm' ]
                    onSelectedContentPropertiesChanged: {
                        app.transcribe_file(selectedContentProperties.filePath, false)
                    }
                }
            }

            Toast {
                id: toast

                anchors.centerIn: parent
            }

            Connections {
                target: _settings
                onTranslator_modeChanged: root.update()
            }

            Connections {
                target: _app_server
                onActivate_requested: {
                    appWin.activate()
                }
                onFiles_to_open_requested: {
                    app.open_files(files, false)
                    appWin.activate()
                }
            }

            Connections {
                target: service
                onModel_download_finished: toast.show(qsTr("The model download is complete!"))
                onModel_download_error: toast.show(qsTr("Error: Couldn't download the model file."))
            }

            Connections {
                target: app
                onNote_copied: toast.show(qsTr("Copied!"))
                onTranscribe_done: toast.show(qsTr("File transcription is complete!"))
                onSpeech_to_file_done: toast.show(qsTr("Speech saved to audio file!"))
                Component.onCompleted: app.open_files(_files_to_open, false)
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
            }
        }
    }
}

/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
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

ApplicationWindow {
    id: appWin

    allowedOrientations: Orientation.All
    initialPage: mainPage
    cover: Qt.resolvedUrl("CoverPage.qml")

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
            readonly property bool canCancelStt: app.state === DsnoteApp.StateTranscribingFile ||
                                        app.state === DsnoteApp.StateListeningSingleSentence ||
                                        app.state === DsnoteApp.StateListeningAuto ||
                                        app.state === DsnoteApp.StateListeningManual
            readonly property bool canCancelTts: app.state === DsnoteApp.StatePlayingSpeech ||
                                        app.state === DsnoteApp.StateWritingSpeechToFile
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
                    readOnly: app.busy || service.busy || !app.connected ||
                              root.canCancelStt || root.canCancelTts
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
                    readOnly: app.busy || service.busy || !app.connected ||
                              root.canCancelStt || root.canCancelTts
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
                    onCancelClicked: app.cancel()
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
                    nameFilters: [ '*.wav', '*.mp3', '*.ogg', '*.flac', '*.m4a', '*.aac', '*.opus' ]
                    onSelectedContentPropertiesChanged: {
                        app.transcribe_file(selectedContentProperties.filePath)
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
                target: service
                onModel_download_finished: toast.show(qsTr("The model download is complete!"))
                onModel_download_error: toast.show(qsTr("Error: Couldn't download the model file."))
            }

            Connections {
                target: app
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
    }
}

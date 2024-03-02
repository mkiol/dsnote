/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Settings 1.0
import harbour.dsnote.Dsnote 1.0

SpeechPanel {
    id: root

    visible: opacity > 0.0
    opacity: enabled ? 1.0 : 0.0
    Behavior on opacity { FadeAnimator { duration: 100 } }

    status: {
        if (app.busy || service.busy || !app.connected)
            return DsnoteApp.TaskStateIdle

        switch (app.task_state) {
        case DsnoteApp.TaskStateIdle: return 0
        case DsnoteApp.TaskStateSpeechDetected: return 1
        case DsnoteApp.TaskStateProcessing: return 2
        case DsnoteApp.TaskStateInitializing: return 3
        case DsnoteApp.TaskStateSpeechPlaying: return 4
        case DsnoteApp.TaskStateSpeechPaused: return 5
        case DsnoteApp.TaskStateCancelling: return 3;
        }
        return 0;
    }

    busy: app.task_state !== DsnoteApp.TaskStateProcessing &&
          app.task_state !== DsnoteApp.TaskStateInitializing &&
          (app.state === DsnoteApp.StateTranscribingFile ||
           app.state === DsnoteApp.StateWritingSpeechToFile ||
           app.state === DsnoteApp.StateImporting ||
           app.state === DsnoteApp.StateExporting)
    text: app.intermediate_text
    textPlaceholder: {
        if (!app.connected) return qsTr("Starting...")
        if (app.busy || service.busy) return qsTr("Busy...")
        if (!app.stt_configured && !app.tts_configured) return qsTr("No language has been set.")
        if (app.task_state === DsnoteApp.TaskStateCancelling)
            return qsTr("Cancelling, please wait...")
        if (app.task_state === DsnoteApp.TaskStateInitializing) return qsTr("Getting ready, please wait...")
        if (app.state === DsnoteApp.StateWritingSpeechToFile) return qsTr("Converting text to speech...")
        if (app.state === DsnoteApp.StateImporting)
            return qsTr("Importing from a file...")
        if (app.state === DsnoteApp.StateExporting)
            return qsTr("Exporting to a file...")
        if (app.state === DsnoteApp.StateTranslating)
            return qsTr("Translating...")
        if (app.task_state === DsnoteApp.TaskStateProcessing) return qsTr("Processing, please wait...")
        if (app.state === DsnoteApp.StateTranscribingFile) return qsTr("Transcribing audio file...")
        if (app.state === DsnoteApp.StateListeningSingleSentence ||
                app.state === DsnoteApp.StateListeningManual ||
                app.state === DsnoteApp.StateListeningAuto) return qsTr("Say something...")
        if (app.task_state === DsnoteApp.TaskStateSpeechPaused) return qsTr("Reading is paused.")
        if (app.state === DsnoteApp.StatePlayingSpeech) return qsTr("Reading a note...")
        return ""
    }

    progress: app.state === DsnoteApp.StateTranscribingFile ? app.transcribe_progress :
              app.state === DsnoteApp.StateWritingSpeechToFile ? app.speech_to_file_progress :
              app.state === DsnoteApp.StateTranslating ? app.translate_progress :
              app.state === DsnoteApp.StateImporting || app.state === DsnoteApp.StateExporting ?
                                                             app.mc_progress : -1.0
}

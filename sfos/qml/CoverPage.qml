/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Settings 1.0
import harbour.dsnote.Dsnote 1.0

CoverBackground {
    id: root

    Label {
        id: noteLabel
        anchors.margins: Theme.paddingLarge
        anchors.bottomMargin: Theme.itemSizeExtraSmall
        anchors.fill: parent
        clip: true
        wrapMode: Text.WordWrap
        text: app.intermediate_text
        font.pixelSize: Theme.fontSizeSmall
        verticalAlignment: Text.AlignBottom
    }

    OpacityRampEffect {
         sourceItem: noteLabel
         direction: OpacityRamp.BottomToTop
         offset: 0.3
    }

    SpeechIndicator {
        id: indicator
        y: app.speech ? 2 * Theme.paddingLarge : (root.height - height) / 2
        anchors.horizontalCenter: parent.horizontalCenter
        width: Theme.coverSizeLarge.width - 2 * Theme.paddingMedium
        height: width / 2
        color: Theme.primaryColor
        status: {
            switch (app.speech) {
            case DsnoteApp.SpeechStateNoSpeech: return 0;
            case DsnoteApp.SpeechStateSpeechDetected: return 1;
            case DsnoteApp.SpeechStateSpeechDecodingEncoding: return 2;
            case DsnoteApp.SpeechStateSpeechInitializing: return 3;
            case DsnoteApp.SpeechStateSpeechPlaying: return 4;
            }
            return 0;
        }
        off: (!app.stt_configured && !app.tts_configured) || !app.connected
        Behavior on y { NumberAnimation { duration: 300; easing {type: Easing.OutBack} } }
    }

    BusyIndicatorWithProgress {
        id: busyIndicator
        size: BusyIndicatorSize.Large
        anchors.centerIn: indicator
        running: app.speech !== DsnoteApp.SpeechStateSpeechDecodingEncoding &&
                 app.speech !== DsnoteApp.SpeechStateSpeechInitializing &&
                 (app.busy || service.busy || !app.connected ||
                 app.state === DsnoteApp.StateTranscribingFile)
        progress: app.transcribe_progress
    }

    CoverActionList {
        id: actions
        iconBackground: true
        enabled: (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                  app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                  app.state === DsnoteApp.StateListeningSingleSentence ||
                  app.state === DsnoteApp.StatePlayingSpeech ||
                  (app.state === DsnoteApp.StateIdle && _settings.mode === Settings.Stt && _settings.speech_mode === Settings.SpeechSingleSentence) ||
                  (app.state === DsnoteApp.StateIdle && _settings.mode === Settings.Tts && _settings.note.length > 0)) &&
                 !app.busy && !service.busy && app.connected

        CoverAction {
            iconSource: {
                if (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                    app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                    app.state === DsnoteApp.StateListeningSingleSentence ||
                    app.state === DsnoteApp.StatePlayingSpeech) {
                    return "image://theme/icon-cover-cancel"
                }
                if (_settings.mode === Settings.Stt)
                    return "image://theme/icon-cover-record"
                return "image://theme/icon-cover-play"
            }

            onTriggered: {
                if (app.speech === DsnoteApp.SpeechStateSpeechDecodingEncoding ||
                    app.speech === DsnoteApp.SpeechStateSpeechInitializing ||
                    app.state === DsnoteApp.StateListeningSingleSentence ||
                    app.state === DsnoteApp.StatePlayingSpeech) {
                    app.cancel()
                    return
                }

                if (_settings.mode === Settings.Stt)
                    return app.listen()
                return app.play_speech()
            }
        }
    }
}

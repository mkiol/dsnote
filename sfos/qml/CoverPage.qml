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
            case DsnoteApp.SttNoSpeech: return 0;
            case DsnoteApp.SttSpeechDetected: return 1;
            case DsnoteApp.SttSpeechDecoding: return 2;
            }
            return 0;
        }
        off: !app.configured
        Behavior on y { NumberAnimation { duration: 300; easing {type: Easing.OutBack} } }
    }

    BusyIndicatorWithProgress {
        id: busyIndicator
        size: BusyIndicatorSize.Large
        anchors.centerIn: indicator
        running: app.state === DsnoteApp.SttTranscribingFile &&
                 app.speech !== DsnoteApp.SttSpeechDecoding
        progress: app.transcribe_progress
    }

    CoverActionList {
        id: actions
        iconBackground: true
        enabled: app.speech === DsnoteApp.SttSpeechDecoding ||
                 app.state === DsnoteApp.SttTranscribingFile ||
                 (_settings.speech_mode === Settings.SpeechSingleSentence &&
                 (app.state === DsnoteApp.SttListeningSingleSentence || app.state === DsnoteApp.SttIdle))

        CoverAction {
            iconSource: app.speech === DsnoteApp.SttSpeechDecoding ||
                        app.state === DsnoteApp.SttListeningSingleSentence ||
                        app.state === DsnoteApp.SttTranscribingFile ?
                            "image://theme/icon-cover-cancel" : "image://theme/icon-cover-unmute"
            onTriggered: {
                if (app.speech === DsnoteApp.SttSpeechDecoding || app.state === DsnoteApp.SttTranscribingFile) {
                    app.cancel()
                    return
                }

                if (app.state === DsnoteApp.SttListeningSingleSentence) app.stop_listen()
                else app.listen()
            }
        }
    }
}

/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
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
        wrapMode: Text.Wrap
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

        y: status === 1 || status === 4 || status === 5 ?
               2 * Theme.paddingLarge : (root.height - height) / 2
        anchors.horizontalCenter: parent.horizontalCenter
        width: Theme.coverSizeLarge.width - 2 * Theme.paddingMedium
        height: width / 2
        color: Theme.primaryColor
        status: {
            switch (app.task_state) {
            case DsnoteApp.TaskStateIdle: return 0;
            case DsnoteApp.TaskStateSpeechDetected: return 1;
            case DsnoteApp.TaskStateProcessing: return 2;
            case DsnoteApp.TaskStateInitializing: return 3;
            case DsnoteApp.TaskStateSpeechPlaying: return 4;
            case DsnoteApp.TaskStateSpeechPaused: return 5;
            }
            return 0;
        }
        off: !app.connected
        Behavior on y { NumberAnimation { duration: 300; easing {type: Easing.OutBack} } }
    }

    CoverActionList {
        enabled: app.state === DsnoteApp.StatePlayingSpeech &&
                 (app.task_state === DsnoteApp.TaskStateProcessing ||
                  app.task_state === DsnoteApp.TaskStateSpeechPlaying ||
                  app.task_state === DsnoteApp.TaskStateSpeechPaused)

        CoverAction {
            iconSource: app.task_state === DsnoteApp.TaskStateSpeechPaused ?
                            "image://theme/icon-cover-play?" :
                            "image://theme/icon-cover-pause?"
            onTriggered: {
                if (app.task_state === DsnoteApp.TaskStateSpeechPaused)
                    app.resume_speech()
                else
                    app.pause_speech()
            }
        }
    }

}

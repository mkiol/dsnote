/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Settings 1.0

CoverBackground {
    id: root


    /*Label {
        id: noteLabel
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        anchors.margins: Theme.paddingLarge
        wrapMode: Text.NoWrap
        text: _settings.note_short
        font.pixelSize: Theme.fontSizeSmall
    }*/

    Label {
        id: noteLabel
        anchors.margins: Theme.paddingLarge
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
        y: app.speech ? 2 * Theme.paddingLarge : (root.height - height) / 2
        anchors.horizontalCenter: parent.horizontalCenter
        width: Theme.coverSizeLarge.width - 2 * Theme.paddingMedium
        height: width / 2
        color: Theme.primaryColor
        active: app.speech
        Behavior on y { NumberAnimation { duration: 300; easing {type: Easing.OutBack} } }
    }
}

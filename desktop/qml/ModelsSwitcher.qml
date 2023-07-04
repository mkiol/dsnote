/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

RowLayout {
    id: root

    property alias sttIndex: sttCombo.currentIndex
    property alias ttsIndex: ttsCombo.currentIndex

    Layout.fillWidth: true

    Frame {
        Layout.fillWidth: true
        background: Item {}

        RowLayout {
            anchors.fill: parent
            enabled: app.stt_configured

            ToolButton {
                icon.name: "audio-input-microphone-symbolic"
                hoverEnabled: false
                down: false
            }

            ComboBox {
                id: sttCombo
                Layout.fillWidth: true

                currentIndex: app.active_stt_model_idx
                model: app.available_stt_models
                onActivated: app.set_active_stt_model_idx(index)

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Speech to Text model")
            }
        }
    }

    Frame {
        Layout.fillWidth: true
        background: Item {}

        RowLayout {
            anchors.fill: parent
            enabled: app.tts_configured

            ToolButton {
                icon.name: "audio-speakers-symbolic"
                hoverEnabled: false
                down: false
            }

            ComboBox {
                id: ttsCombo
                Layout.fillWidth: true

                currentIndex: app.active_tts_model_idx
                model: app.available_tts_models
                onActivated: app.set_active_tts_model_idx(index)

                ToolTip.visible: hovered
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.text: qsTr("Text to Speech model")
            }
        }
    }
}

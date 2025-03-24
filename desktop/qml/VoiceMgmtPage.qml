/* Copyright (C) 2024-2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Settings 1.0

DialogPage {
    id: root

    readonly property bool verticalMode: width < appWin.height
    readonly property real _rightMargin: (!root.mirrored && listViewExists && listViewStackItem.currentItem.ScrollBar.vertical.visible) ?
                                             appWin.padding + listViewStackItem.currentItem.ScrollBar.vertical.width :
                                             appWin.padding
    readonly property real _leftMargin: (root.mirrored && listViewExists && listViewStackItem.currentItem.ScrollBar.vertical.visible) ?
                                             appWin.padding + listViewStackItem.currentItem.ScrollBar.vertical.width :
                                             appWin.padding

    title: qsTr("Voice profiles")    

    function updateTypeView() {
        stop_playback()

        switch(_settings.active_voice_profile_type) {
        case Settings.VoiceProfileAudioSample:
            if (listViewStackItem.depth > 1) listViewStackItem.pop()
            break;
        case Settings.VoiceProfilePrompt:
            if (listViewStackItem.depth < 2) listViewStackItem.push(listViewPromptComp)
            break
        }
    }

    function stop_playback() {
        if (app.player_playing)
            app.player_stop_voice_ref()
    }

    function create_new() {
        stop_playback()

        switch(_settings.active_voice_profile_type) {
        case Settings.VoiceProfileAudioSample:
            appWin.openDialog("VoiceImportPage.qml")
            break;
        case Settings.VoiceProfilePrompt:
            appWin.openDialog("VoicePromptEditPage.qml")
            break
        }
    }

    Connections {
        target: _settings
        onActive_voice_profile_type_changed: root.updateTypeView()
    }

    Component.onCompleted: {
        listViewStackItem.push(listViewComp)
        updateTypeView()
    }

    Component {
        id: helpDialog

        HelpDialog {
            title: qsTr("Voice profile")

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Voice profile can be defined with an audio sample or text description.")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Audio sample") +
                      "<ul>" +
                      "<li>" + qsTr("Allows you to clone someone's voice.") + "</li>" +
                      "<li>" + qsTr("Can be used in models that support %1.").arg("<i>" + qsTr("Voice Cloning") + "</i>") + "</li>" +
                      "</ul>"
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Text voice profile") +
                      "<ul>" +
                      "<li>" + qsTr("Allows you to define different speaker characteristics, such as gender, mood or pace.") + "</li>" +
                      "<li>" + qsTr("Can be used in models that support %1.").arg("<i>" + qsTr("Voice Text Description") + "</i>") + "</li>" +
                      "</ul>"
            }
        }
    }

    header: ColumnLayout {
        visible: root.title.length !== 0
        spacing: appWin.padding

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: root.leftPadding
            Layout.rightMargin: root.rightPadding
            Layout.topMargin: root.topPadding

            Label {
                id: titleLabel

                text: root.title
                font.pixelSize: Qt.application.font.pixelSize * 1.2
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.leftMargin: appWin.padding
                Layout.alignment: Qt.AlignVCenter
            }
        }

        ColumnLayout {
            visible: root.verticalMode
            Layout.fillWidth: true
            Layout.leftMargin: root.leftPadding + appWin.padding
            Layout.rightMargin: root.rightPadding + appWin.padding

            ComboBox {
                id: voiceFeatureTypeComboBar

                Layout.fillWidth: true
                onCurrentIndexChanged: {
                    voiceFeatureTypeComboBar.currentIndex = currentIndex
                    if (currentIndex == 1) {
                        _settings.active_voice_profile_type = Settings.VoiceProfilePrompt
                    } else {
                        _settings.active_voice_profile_type = Settings.VoiceProfileAudioSample
                    }
                }

                model: [
                    qsTr("Audio sample"),
                    qsTr("Text voice profile")
                ]
            }
        }

        TabBar {
            id: voiceFeatureTypeTabBar

            Layout.fillWidth: true
            currentIndex: {
                switch(_settings.active_voice_profile_type) {
                case Settings.VoiceProfileAudioSample:
                    return 0
                case Settings.VoiceProfilePrompt:
                    return 1
                }
                return 0
            }

            onCurrentIndexChanged: {
                voiceFeatureTypeComboBar.currentIndex = currentIndex
                if (currentIndex == 1) {
                    _settings.active_voice_profile_type = Settings.VoiceProfilePrompt
                } else {
                    _settings.active_voice_profile_type = Settings.VoiceProfileAudioSample
                }
            }
            visible: !root.verticalMode
            Layout.leftMargin: root.leftPadding + appWin.padding
            Layout.rightMargin: root.rightPadding + appWin.padding

            TabButton {
                id: audioTabButton

                width: implicitWidth
                action: Action {
                    text: qsTr("Audio sample")
                    onTriggered: {
                        checked = true
                    }
                }
            }

            TabButton {
                id: promptTabButton

                width: implicitWidth
                action: Action {
                    text: qsTr("Text voice profile")
                    onTriggered: {
                        checked = true
                    }
                }
            }
        }
    }


    footer: RowLayout {
        height: closeButton.height + appWin.padding

        Button {
            id: createNewButton

            Layout.leftMargin: root.leftPadding + appWin.padding
            Layout.bottomMargin: root.bottomPadding
            text: {
                switch(_settings.active_voice_profile_type) {
                case Settings.VoiceProfileAudioSample:
                    return qsTr("Create a new audio sample")
                case Settings.VoiceProfilePrompt:
                    qsTr("Create a new text voice profile")
                    break
                }
            }

            icon.name: "special-effects-symbolic"
            onClicked: root.create_new()
            Keys.onReturnPressed: root.create_new()
        }

        Button {
            icon.name: "help-about-symbolic"
            display: AbstractButton.IconOnly
            text: qsTr("Help")
            onClicked: appWin.openPopup(helpDialog)
            Layout.bottomMargin: root.bottomPadding

            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: text
            hoverEnabled: true
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: closeButton

            Layout.rightMargin: root.leftPadding + appWin.padding
            Layout.bottomMargin: root.bottomPadding

            text: qsTr("Close")
            icon.name: "window-close-symbolic"
            onClicked: root.reject()
            Keys.onEscapePressed: root.reject()
        }
    }

    onOpenedChanged: {
        if (!opened) {
            app.player_reset()
            app.recorder_reset()
        }
    }

    Component {
        id: voicePromptDelegate

        Control {
            id: control

            readonly property bool compact: root.verticalMode
            readonly property string voiceName: modelData[0]
            readonly property string voiceDesc: modelData[1]

            background: Rectangle {
                id: bg

                anchors.fill: parent
                color: palette.text
                opacity: control.hovered ? 0.1 : 0.0
            }

            width: root.listViewStackItem.currentItem.width
            height: deleteButton.height
            leftPadding: root._leftMargin

            RowLayout {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: root._rightMargin
                anchors.left: parent.left
                anchors.leftMargin: root._leftMargin

                Label {
                    text: control.voiceName
                    elide: Text.ElideRight
                    Layout.alignment: Qt.AlignHCenter
                    Layout.leftMargin: appWin.padding
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Edit")
                    icon.name: "edit-entry-symbolic"
                    display: control.compact ? Button.IconOnly : Button.TextBesideIcon
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: appWin.openDialog("VoicePromptEditPage.qml", {data: modelData})

                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: control.compact
                }

                Button {
                    text: qsTr("Clone")
                    icon.name: "entry-clone-symbolic"
                    display: control.compact ? Button.IconOnly : Button.TextBesideIcon
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: app.clone_voice_prompt(control.voiceName)

                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: control.compact
                }

                Button {
                    id: deleteButton

                    icon.name: "edit-delete-symbolic"
                    display: control.compact ? Button.IconOnly : Button.TextBesideIcon
                    text: qsTr("Delete")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: app.delete_voice_prompt(control.voiceName)

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: control.compact
                }
            }
        }
    }

    Component {
        id: voiceDelegate

        Control {
            id: control

            readonly property bool compact: root.verticalMode
            readonly property string voiceName: modelData[0]
            property bool editActive: false

            background: Rectangle {
                id: bg

                anchors.fill: parent
                color: palette.text
                opacity: control.hovered ? 0.1 : 0.0
            }

            width: root.listViewStackItem.currentItem.width
            height: deleteButton.height
            leftPadding: root._leftMargin

            RowLayout {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: root._rightMargin
                anchors.left: parent.left
                anchors.leftMargin: root._leftMargin

                Label {
                    text: control.voiceName
                    elide: Text.ElideRight
                    Layout.alignment: Qt.AlignHCenter
                    Layout.leftMargin: appWin.padding
                    Layout.fillWidth: true
                }

                Button {
                    property bool playing: app.player_playing && app.player_current_voice_ref_idx === index

                    icon.name: playing ? "media-playback-stop-symbolic" : "media-playback-start-symbolic"
                    text: playing ? qsTr("Stop") : qsTr("Play")
                    display: control.compact ? Button.IconOnly : Button.TextBesideIcon
                    onClicked: {
                        if (playing)
                            app.player_stop_voice_ref()
                        else
                            app.player_play_voice_ref_idx(index)
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: control.compact
                }

                Button {
                    text: qsTr("Edit")
                    icon.name: "edit-entry-symbolic"
                    display: control.compact ? Button.IconOnly : Button.TextBesideIcon
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: appWin.openDialog("VoiceAudioSampleEditPage.qml", {data: modelData, index: index})

                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: control.compact
                }

                Button {
                    id: deleteButton

                    icon.name: "edit-delete-symbolic"
                    display: control.compact ? Button.IconOnly : Button.TextBesideIcon
                    text: qsTr("Delete")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        root.stop_playback()
                        app.delete_tts_ref_voice(index)
                    }

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: control.compact
                }
            }
        }
    }

    placeholderLabel {
        text: qsTr("You haven't created any voice profile yet.") + " " +
              qsTr("Use %1 to make a new one.").arg("<i>" + createNewButton.text + "</i>")
        enabled: root.listViewStackItem.currentItem.model.length === 0
    }

    Component {
        id: listViewComp

        ListView {
            id: listView

            focus: true
            clip: true
            spacing: appWin.padding

            Keys.onUpPressed: listViewScrollBar.decrease()
            Keys.onDownPressed: listViewScrollBar.increase()

            ScrollBar.vertical: ScrollBar {
                id: listViewScrollBar
            }

            model: app.available_tts_ref_voices
            delegate: voiceDelegate
            header: Item {
                height: appWin.padding
            }
        }
    }

    Component {
        id: listViewPromptComp

        ListView {
            id: listView

            focus: true
            clip: true
            spacing: appWin.padding

            Keys.onUpPressed: listViewScrollBar.decrease()
            Keys.onDownPressed: listViewScrollBar.increase()

            ScrollBar.vertical: ScrollBar {
                id: listViewScrollBar
            }

            model: _settings.tts_voice_prompts
            delegate: voicePromptDelegate
            header: Item {
                height: appWin.padding
            }
        }
    }
}

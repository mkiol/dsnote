/* Copyright (C) 2025 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

import org.mkiol.dsnote.Settings 1.0

DialogPage {
    id: root

    readonly property bool verticalMode: width < height
    property var data: null // null => new data

    readonly property string dataName: data ? data[0] : ""
    readonly property string dataDesc: data ? data[1] : ""
    readonly property bool canSave: testData()
    readonly property bool nameTaken: _nameForm.textField.text.trim().length > 0 &&
                                      _descForm.text.trim().length > 0 && !testData()
    readonly property int effectiveWidth: root.implicitWidth - root._leftMargin - root._rightMargin - appWin.padding

    title: data ? qsTr("Edit text voice profile") : qsTr("Create a new text voice profile")

    function cancel() {
        appWin.openDialog("VoiceMgmtPage.qml")
    }

    function saveData() {
        var name = dataName;
        var new_name = _nameForm.textField.text.trim()
        var desc = _descForm.text.trim()

        if (new_name.length === 0 || desc.length === 0) {
            return
        }

        app.update_voice_prompt(name, new_name, desc)

        appWin.openDialog("VoiceMgmtPage.qml")
    }

    function testData() {
        var name = dataName;
        var new_name = _nameForm.textField.text.trim()
        var desc = _descForm.text.trim()

        if (new_name.length === 0 || desc.length === 0) {
            return false
        }

        return app.test_voice_prompt(name, new_name, desc)
    }

    Component {
        id: helpDialog

        HelpDialog {
            title: qsTr("Text voice profile")

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Voice profile can be defined by text description.") + " " +
                      qsTr("A simple description could be, for example: %1")
                        .arg("<i>\"Jenna's voice is monotone, but somewhat fast, with no background noise.\"</i>")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Tips:") +
                      "<ul>" +
                      "<li>" + qsTr("Include the term %1 to generate the highest quality audio, and %2 for high levels of background noise")
                                .arg("<i>\"very clear audio\"</i>").arg("<i>\"very noisy audio\"</i>") + "</li>" +
                      "<li>" + qsTr("Use emotions: %1, %2, %3, %4")
                                .arg("<i>\"happy\"</i>").arg("<i>\"confused\"</i>").arg("<i>\"laughing\"</i>").arg("<i>\"sad\"</i>") + "</li>" +
                      "<li>" + qsTr("To ensure speaker consistency across generations, include speaker name.") + "</li>" +
                      "</ul>"
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Currently, text voice profiles can be used only in %1 models.").arg("<i>Parler TTS</i>") + " " +
                      qsTr("To learn more about how to create a good text description, check out the specific model's website.")
            }

            RichLabel {
                text: "<ul>" +
                      "<li>Parler TTS &rarr; <a href='https://github.com/huggingface/parler-tts'>https://github.com/huggingface/parler-tts</a></li>" +
                      "<li>Parler TTS Expresso &rarr; <a href='https://huggingface.co/parler-tts/parler-tts-mini-expresso'>https://huggingface.co/parler-tts/parler-tts-mini-expresso</a></li>" +
                      "</ul>"
            }
        }
    }

    footer: Item {
        height: closeButton.height + appWin.padding

        RowLayout {
            anchors {
                left: parent.left
                leftMargin: root.leftPadding + appWin.padding
                right: parent.right
                rightMargin: root.rightPadding + appWin.padding
                bottom: parent.bottom
                bottomMargin: root.bottomPadding
            }

            Button {
                icon.name: "help-about-symbolic"
                display: AbstractButton.IconOnly
                text: qsTr("Help")
                onClicked: appWin.openPopup(helpDialog)
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: text
                hoverEnabled: true
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Save")
                enabled: root.canSave
                icon.name: "document-save-symbolic"
                Keys.onReturnPressed: root.saveData()
                onClicked: root.saveData()
            }

            Button {
                id: closeButton

                text: qsTr("Cancel")
                icon.name: "action-unavailable-symbolic"
                onClicked: root.cancel()
                Keys.onEscapePressed: root.cancel()
            }
        }
    }

    ColumnLayout {
        property alias verticalMode: root.verticalMode
        Layout.fillWidth: true

        TextArea {
            id: _descForm

            selectByMouse: true
            wrapMode: TextEdit.Wrap
            verticalAlignment: TextEdit.AlignTop
            placeholderText: qsTr("Description")
            ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
            ToolTip.visible: hovered
            ToolTip.text: placeholderText
            hoverEnabled: true
            Layout.fillWidth: true
            text: root.dataDesc
            Layout.minimumHeight: _nameForm.textField.implicitHeight * 3

            TextContextMenu {}
        }

        TextFieldForm {
            id: _nameForm

            label.text: qsTr("Name")
            valid: !root.nameTaken
            toolTip: valid ? "" : qsTr("This name is already taken")
            textField {
                text: root.dataName
            }
        }
    }
}

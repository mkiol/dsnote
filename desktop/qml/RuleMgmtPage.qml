/* Copyright (C) 2024-2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    title: qsTr("Rules")

    function defaultRuleName(type, pattern, replace) {
        switch(type) {
        // case Settings.TransRuleTypeMatchSimple:
        // case Settings.TransRuleTypeMatchRe:
        //     return qsTr("Match: %1").arg(pattern)
        case Settings.TransRuleTypeReplaceSimple:
        case Settings.TransRuleTypeReplaceRe:
            return qsTr("Replace: %1").arg(pattern + " \u2192 " + replace)
        }
        return ""
    }

    Component.onCompleted: {
        listViewStackItem.push(listViewComp)
    }

    onOpened: {
        if ((_settings.hint_done_flags & Settings.HintDoneRules) == 0) {
            appWin.openPopup(helpDialog)
        }
    }

    Component {
        id: helpDialog

        HelpDialog {
            title: qsTr("Rules")

            onClosed: _settings.set_hint_done(Settings.HintDoneRules)

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                textFormat: Text.StyledText
                text: qsTr("%1 allows you to create text transformations that can be applied after Speech to Text or before Text to Speech.").arg("<i>" + qsTr("Rules") + "</i>") + " " +
                      qsTr("With %1, you can easily and flexibly correct errors in decoded text or correct mispronounced words.").arg("<i>" + qsTr("Rules") + "</i>")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                textFormat: Text.StyledText
                text: qsTr("Rules are applied to text sequentially, starting from the rule at the top to the bottom.") + " " +
                      qsTr("Text transformation is performed automatically in the background when text is received from the Speech to Text engine (%1) or " +
                           "before text is sent to the Text to Speech engine (%2).").arg("<i>" + qsTr("Listening") + "</i>").arg("<i>" + qsTr("Reading") + "</i>")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                textFormat: Text.RichText
                text: qsTr("Here are examples of typical use of the rules:") + " " +
                      "<ul>" +
                      "<li>" + qsTr("Converting a word (e.g., \"period\", \"comma\") into a punctuation mark in Speech to Text") + "</li>" +
                      "<li>" + qsTr("Inserting a new line into a note while dictating") + "</li>" +
                      "<li>" + qsTr("Changing the spelling of foreign language names (e.g., first or last names) to make the pronunciation correct") + "</li>" +
                      "<li>" + qsTr("Removing special characters from text that cause mispronunciation by Text to Speech engine") + "</li>" +
                      "</ul>"
            }
        }
    }

    footer: RowLayout {
        height: closeButton.height + appWin.padding

        Button {
            id: createNewButton

            Layout.leftMargin: root.leftPadding + appWin.padding
            Layout.bottomMargin: root.bottomPadding

            text: qsTr("Create a new rule")
            icon.name: "special-effects-symbolic"
            onClicked: appWin.openDialog("RuleEditPage.qml")
            Keys.onReturnPressed: appWin.openDialog("RuleEditPage.qml")
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

    Component {
        id: ruleDelegate

        ItemDelegate {
            id: itemDelegate

            readonly property bool ruleTargetStt: (modelData[0] & Settings.TransRuleTargetStt) != 0
            readonly property bool ruleTargetTts: (modelData[0] & Settings.TransRuleTargetTts) != 0
            readonly property string ruleName: modelData[2].length > 0 ? modelData[2] :
                                                                         defaultRuleName(modelData[1], modelData[3], modelData[4])

            text: ruleName
            width: ListView.view.width
            onClicked: openEdit()
            Accessible.name: ruleName
            leftPadding: root._leftMargin
            bottomInset: 0
            topInset: 0

            Component.onCompleted: {
                if (background && background.color) {
                    background.color = "transparent"
                }
            }

            function openEdit() {
                appWin.openDialog("RuleEditPage.qml", {rule: modelData, ruleIndex: index})
            }

            contentItem: RowLayout {
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                Label {
                    text: "" + (index + 1) + " "
                    font.pixelSize: appWin.textFontSizeBig
                }

                Button {
                    icon.name: "arrow-up-symbolic"
                    display: Button.IconOnly
                    text: qsTr("Move up")
                    enabled: index > 0
                    onClicked: _settings.trans_rule_move_up(index)

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: true
                }
                
                Button {
                    icon.name: "arrow-down-symbolic"
                    display: Button.IconOnly
                    text: qsTr("Move down")
                    enabled: (index + 1) < _settings.trans_rules.length
                    onClicked: _settings.trans_rule_move_down(index)

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: true
                }

                Label {
                    id: nameField

                    text: itemDelegate.ruleName
                    elide: Text.ElideRight
                    Layout.alignment: Qt.AlignHCenter
                    Layout.leftMargin: appWin.padding
                    Layout.fillWidth: true
                }

                Switch {
                    id: sttSwitch

                    checked: itemDelegate.ruleTargetStt
                    Layout.alignment: Qt.AlignHCenter
                    onCheckedChanged: {
                        _settings.trans_rule_set_target_stt(index, checked)
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Speech to Text")
                    hoverEnabled: true

                    Accessible.name: qsTr("Speech to Text")
                }

                Switch {
                    id: ttsSwitch

                    checked: itemDelegate.ruleTargetTts
                    Layout.alignment: Qt.AlignHCenter
                    onCheckedChanged: {
                        _settings.trans_rule_set_target_tts(index, checked)
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Text to Speech")
                    hoverEnabled: true

                    Accessible.name: qsTr("Text to Speech")
                }

                Button {
                    id: editButton

                    text: qsTr("Edit")
                    icon.name: "edit-entry-symbolic"
                    display: Button.IconOnly
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: itemDelegate.openEdit()

                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: true
                }

                Button {
                    id: cloneButton

                    text: qsTr("Clone")
                    icon.name: "entry-clone-symbolic"
                    display: Button.IconOnly
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: _settings.trans_rule_clone(index)

                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: true
                }

                Button {
                    id: deleteButton

                    icon.name: "edit-delete-symbolic"
                    display: Button.IconOnly
                    text: qsTr("Delete")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: _settings.trans_rule_delete(index)

                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: true
                }
            }
        }
    }

    placeholderLabel {
        text: qsTr("You haven't created any rules yet.") + " " +
              qsTr("Use %1 to make a new one.").arg("<i>" + createNewButton.text + "</i>")
        enabled: root.listViewStackItem.currentItem.model.length === 0
    }

    Component {
        id: listViewComp

        ListView {
            id: listView

            focus: true
            clip: true
            spacing: 0

            Keys.onUpPressed: listViewScrollBar.decrease()
            Keys.onDownPressed: listViewScrollBar.increase()

            ScrollBar.vertical: ScrollBar {
                id: listViewScrollBar
            }

            model: _settings.trans_rules
            delegate: ruleDelegate
        }
    }
}

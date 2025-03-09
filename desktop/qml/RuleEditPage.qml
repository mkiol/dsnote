/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
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
    property var rule: null // null => new rule
    property int ruleIndex: -1 // -1 index => new rule

    readonly property bool ruleTargetStt: rule ? rule[0] & Settings.TransRuleTargetStt : false
    readonly property bool ruleTargetTts: rule ? rule[0] & Settings.TransRuleTargetTts: false
    readonly property string ruleName: rule ? rule[2] : ""
    readonly property string rulePattern: rule ? rule[3] : ""
    readonly property string ruleReplace: rule ? rule[4] : ""
    readonly property int ruleType: rule ? rule[1] : Settings.TransRuleTypeReplaceSimple
    readonly property string ruleLangs: rule ? rule[5] : ""
    readonly property bool canSave: _patternForm.textField.text.length > 0
    readonly property int effectiveWidth: root.implicitWidth - root._leftMargin - root._rightMargin - appWin.padding

    title: rule ? qsTr("Edit rule") : qsTr("Create a new rule")

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

    function cancel() {
        appWin.openDialog("RuleMgmtPage.qml")
    }

    function saveRule() {
        var flags = rule ? rule[0] : 0
        if (sttCheckBox.checked)
            flags |= Settings.TransRuleTargetStt
        else
            flags &= ~Settings.TransRuleTargetStt

        if (ttsCheckBox.checked)
            flags |= Settings.TransRuleTargetTts
        else
            flags &= ~Settings.TransRuleTargetTts

        app.update_trans_rule(
                   /*index=*/root.ruleIndex,
                   /*flags=*/flags,
                   /*name=*/_nameForm.textField.text,
                   /*pattern=*/_patternForm.textField.text,
                   /*replace=*/_replaceForm.textField.text,
                   /*langs=*/langsCheckBox.checked ? "" : langsTextField.text.trim(),
                   /*type=*/_typeForm.type)
        appWin.openDialog("RuleMgmtPage.qml")
    }

    function testRule() {
        var result = app.test_trans_rule(
                    /*text=*/_testTextArea.textArea1.text,
                    /*pattern=*/_patternForm.textField.text,
                    /*replace=*/_replaceForm.textField.text,
                    /*type=*/_typeForm.type)

        _testTextArea.textArea2.text = result[1]

        if (result[0]) { // matched
            _testTextArea.textArea2.color = _testTextArea.dimColor
        } else {
            _testTextArea.textArea2.color = "red"
        }
    }

    Component {
        id: patterHelpDialog

        HelpDialog {
            title: qsTr("Pattern")

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("A pattern is used to find a match in the text and replace it with another text.") + " " +
                      qsTr("A simple pattern is just plain text.") + " " +
                      qsTr("To define a more advanced pattern, you can use a regular expression.")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                textFormat: Text.RichText
                text: qsTr("When defining a regular expression pattern, you can use tags for the following character classes:") + " " +
                      "<ul>" +
                      "<li><b>.</b> - " + qsTr("Matches any character (including newline).") + "</li>" +
                      "<li><b>\\n</b> - " + qsTr("Matches a newline.") + "</li>" +
                      "<li><b>\\d</b> - " + qsTr("Matches a non-digit.") + "</li>" +
                      "<li><b>\\s</b> - " + qsTr("Matches a whitespace character.") + "</li>" +
                      "<li><b>\\S</b> - " + qsTr("Matches a non-whitespace character.") + "</li>" +
                      "<li><b>\\w</b> - " + qsTr("Matches a word character.") + "</li>" +
                      "<li><b>\\W</b> - " + qsTr("Matches a non-word character.") + "</li>" +
                      "</ul>"
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                textFormat: Text.RichText
                text: qsTr("Here are some examples of regular expression patterns:") + " " +
                      "<ul>" +
                      "<li><b>new[\\s-]*line</b> - " + qsTr("Matches %1, %2 and %3.").arg("<i>newline</i>").arg("<i>new-line</i>").arg("<i>new line</i>") + "</li>" +
                      "<li><b>\\[.*\\]</b> - " + qsTr("Matches any text put in square brackets.") + "</li>" +
                      "<li><b>\\s(\\w+)\\.</b> - " + qsTr("Captures any word ending in a period.") + "</li>" +
                      "</ul>"
            }
        }
    }

    Component {
        id: replaceHelpDialog

        HelpDialog {
            title: qsTr("Replacement text")

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("A replacement text is inserted instead of the text matched by the pattern.") + " " +
                      qsTr("Simple replacement text is just plain text.") + " " +
                      qsTr("To define more advanced replacement, you can use a regular expression pattern that contains capturing groups.")
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                textFormat: Text.RichText
                text: qsTr("To pass the captured text from the pattern as a replacement, use the following tags:") + " " +
                      "<ul>" +
                      "<li><b>\\1</b> - " + qsTr("Resolves to the text captured in the first capture group.") + "</li>" +
                      "<li><b>\\2</b> - " + qsTr("Resolves to the text captured in the second capture group.") + "</li>" +
                      "<li><b>\\U\\1</b> - " + qsTr("Makes all characters in the first capture group be capitalized.") + "</li>" +
                      "<li><b>\\u\\1</b> - " + qsTr("Causes all characters in the first capture group to be lowercase.") + "</li>" +
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

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Save")
                enabled: root.canSave
                icon.name: "document-save-symbolic"
                Keys.onReturnPressed: root.saveRule()
                onClicked: root.saveRule()
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

        DoubleTextArea {
            id: _testTextArea

            preferredHeight: _patternForm.height

            toolTip1: qsTr("Type here to see how the rule changes the text.")
            textArea1 {
                placeholderText: qsTr("Type here to see how the rule changes the text.")
                onTextChanged: {
                    app.trans_rules_test_text = _testTextArea.textArea1.text
                    root.testRule()
                }
                font: _settings.notepad_font
            }

            toolTip2: qsTr("Text after applying the rule.") + " " + qsTr("Red color means that pattern doesn't match the text.")
            textArea2 {
                placeholderText: qsTr("Text after applying the rule.")
                font: _settings.notepad_font
                readOnly: true
                color: _testTextArea.dimColor
            }

            Component.onCompleted: _testTextArea.textArea1.text = app.trans_rules_test_text
        }

        ComboBoxForm {
            id: _typeForm

            property int type: {
                if (comboBox.currentIndex === 0)
                    return Settings.TransRuleTypeReplaceSimple
                if (comboBox.currentIndex === 1)
                    return Settings.TransRuleTypeReplaceRe
                if (comboBox.currentIndex === 2)
                    return Settings.TransRuleTypeMatchSimple
                if (comboBox.currentIndex === 3)
                    return Settings.TransRuleTypeMatchRe
                return 0
            }

            label.text: qsTr("Pattern type")
            compact: true
            toolTip: qsTr("Type of a pattern.") + " " +
                     qsTr("You can simply replace the found text or replace the text with a regular expression.")
            comboBox {
                currentIndex: {
                    switch(root.ruleType) {
                    case Settings.TransRuleTypeReplaceSimple: return 0
                    case Settings.TransRuleTypeReplaceRe: return 1
                    //case Settings.TransRuleTypeMatchSimple: return 2
                    //case Settings.TransRuleTypeMatchRe: return 3
                    }
                    return 0
                }
                model: [
                    qsTr("Replace"),
                    qsTr("Replace (Regular expression)"),
                    //qsTr("Match"),
                    //qsTr("Match (Regular expression)")
                ]
                onActivated: root.testRule()
            }
        }

        TextFieldWithHelpForm {
            id: _patternForm

            label.text: qsTr("Pattern")
            valid: app.trans_rule_re_pattern_valid(text)
            onHelpClicked: appWin.openPopup(patterHelpDialog)
            textField {
                text: root.rulePattern
                onTextChanged: root.testRule()
            }
        }

        TextFieldWithHelpForm {
            id: _replaceForm

            enabled: _typeForm.type === Settings.TransRuleTypeReplaceSimple ||
                     _typeForm.type === Settings.TransRuleTypeReplaceRe
            label.text: qsTr("Replacement text")
            onHelpClicked: appWin.openPopup(replaceHelpDialog)
            textField {
                text: root.ruleReplace
                onTextChanged: root.testRule()
            }
        }

        TextFieldForm {
            id: _nameForm

            label.text: qsTr("Name")
            toolTip: qsTr("Optional name of the rule. If the name is not specified, the default name will be used.")
            textField {
                text: root.ruleName
                placeholderText: root.defaultRuleName(_typeForm.type,
                                                      _patternForm.text,
                                                      _replaceForm.text)
            }
        }

        RowForm {
            label.text: qsTr("Languages")

            CheckBox {
                id: langsCheckBox

                checked: root.ruleLangs.length === 0
                text: qsTr("All languages")
            }

            TextField {
                id: langsTextField

                enabled: !langsCheckBox.checked
                text: root.ruleLangs
                Layout.fillWidth: true
                placeholderText: qsTr("List of language codes")
                ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                ToolTip.visible: hovered
                ToolTip.text: qsTr("A comma separated list of language codes for which the rule should be enabled.") + " " +
                              qsTr("Example: %1 means that the rule applies only to Czech, Slovak and Polish languages.").arg("<i>cs, sk, pl</i>")
                hoverEnabled: true
                Accessible.name: placeholderText
                TextContextMenu {}
            }
        }

        RowForm {
            label.text: qsTr("Types of usage")

            CheckBox {
                id: sttCheckBox

                checked: root.ruleTargetStt
                text: qsTr("Speech to Text")
            }

            RowLayout {
                Layout.fillWidth: true

                CheckBox {
                    id: ttsCheckBox

                    checked: root.ruleTargetTts
                    text: qsTr("Text to Speech")
                }

                Button {
                    icon.name: "audio-speakers-symbolic"
                    flat: true
                    visible: ttsCheckBox.checked
                    display: AbstractButton.IconOnly
                    enabled: ttsCheckBox.checked && _testTextArea.textArea2.text.length !== 0 && app.tts_configured
                    action: Action {
                        text: qsTr("Read the text")
                        onTriggered: app.play_speech_from_text(_testTextArea.textArea2.text, "")
                    }
                    ToolTip.delay: Qt.styleHints.mousePressAndHoldInterval
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    hoverEnabled: true
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}

/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts

import org.mkiol.dsnote.Settings 1.0

ColumnLayout {
    id: root

    property bool verticalMode: parent ? parent.verticalMode : false
    property color textColor: appWin.appTextColor

    CheckBoxSetting {
        optionName: "translate_when_typing"
        text: qsTranslate("SettingsPage", "Translate as you type")
    }

    CheckBoxSetting {
        optionName: "mnt_clean_text"
        text: qsTranslate("SettingsPage", "Clean up the text")
        toolTipText: qsTranslate("SettingsPage", "Remove duplicate whitespaces and extra line breaks in the text before translation.") + " " +
                     qsTranslate("SettingsPage", "If the input text is incorrectly formatted, this option may improve the translation quality.")
    }

    CheckBoxSetting {
        optionName: "mnt_echo"
        text: qsTranslate("SettingsPage", "Echo mode")
        toolTipText: qsTranslate("SettingsPage", "When text is inserted using Speech to Text (%1 button), it is translated to the chosen language and immediately read aloud using the selected Text to Speech model.")
                     .arg("<i>" + qsTranslate("SettingsPage", "Listen") + "</i>") + " " +
                     qsTranslate("SettingsPage", "This option controls Echo mode in Translator only. To echo text before translation, enable Echo mode in Speech to Text settings.")
    }

    CheckBoxSetting {
        optionName: "mnt_pinyin"
        text: qsTranslate("SettingsPage", "Translate to Pinyin")
        toolTipText: qsTranslate("SettingsPage", "When translating into Chinese, use Pinyin instead of Chinese characters.")
    }

    ComboBoxForm {
        id: uiTranslatorLayoutCombo

        label.text: qsTranslate("SettingsPage", "Voice options in translator mode")
        toolTip: qsTranslate("SettingsPage", "Choose which voice options appear in translator mode.")
        comboBox {
            currentIndex: {
                switch (_settings.ui_translator_layout) {
                    case Settings.UiTranslatorLayoutInTts: return 0
                    case Settings.UiTranslatorLayoutInStt: return 1
                    case Settings.UiTranslatorLayoutInBoth: return 0
                }
                return 0
            }
            model: [
                qsTranslate("SettingsPage", "Show %1 option").arg(qsTranslate("SettingsPage", "Read")),
                qsTranslate("SettingsPage", "Show %1 option").arg(qsTranslate("SettingsPage", "Listen"))
            ]
            onActivated: (index) => {
                if (index === 0) {
                    _settings.ui_translator_layout = Settings.UiTranslatorLayoutInBoth
                } else if (index === 1) {
                    _settings.ui_translator_layout = Settings.UiTranslatorLayoutInStt
                } else {
                    _settings.ui_translator_layout = Settings.UiTranslatorLayoutInBoth
                }
            }
        }
    }

    Item {
        Layout.fillHeight: true
    }
}


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
        optionName: "pinyin_text"
        text: qsTranslate("SettingsPage", "Use pinyin")
        toolTipText: qsTranslate("SettingsPage", "For Chinese, use pinyin instead of Chinese characters.")
    }

    Item {
        Layout.fillHeight: true
    }
}


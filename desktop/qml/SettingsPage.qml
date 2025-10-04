/* Copyright (C) 2021-2024 Michal Kosciesza <michal@mkiol.net>
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

DialogPage {
    id: root

    property bool verticalMode: bar.implicitWidth > (root.width - 2 * appWin.padding)

    title: qsTr("Settings")

    width: parent.width - 4 * appWin.padding
    height: parent.height - 4 * appWin.padding

    footerLabel {
        visible: _settings.restart_required
        text: qsTr("Restart the application to apply changes.")
        color: "red"
    }

    LayoutFixer {
        id: fixer
    }

    TabBar {
        id: bar

        visible: !root.verticalMode
        Layout.fillWidth: true
        onCurrentIndexChanged: {
            comboBar.currentIndex = currentIndex
            stack.idx = currentIndex
        }

        TabButton {
            text: qsTr("General")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("User Interface")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Speech to Text")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Text to Speech")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Accessibility")
            width: implicitWidth
        }

        TabButton {
            text: qsTr("Advanced")
            width: implicitWidth
        }
    }

    ColumnLayout {
        visible: root.verticalMode
        Layout.fillWidth: true

        ComboBox {
            id: comboBar

            Layout.fillWidth: true
            onCurrentIndexChanged: {
                bar.currentIndex = currentIndex
                stack.idx = currentIndex
            }

            model: [
                qsTr("General"),
                qsTr("User Interface"),
                qsTr("Speech to Text"),
                qsTr("Text to Speech"),
                qsTr("Accessibility"),
                qsTr("Advanced")
            ]
        }

        HorizontalLine{}
    }

    StackView {
        id: stack

        property int idx: -1
        property alias verticalMode: root.verticalMode

        Layout.fillWidth: true
        Layout.topMargin: appWin.padding
        height: currentItem.implicitHeight
        implicitHeight: currentItem.implicitHeight
        onIdxChanged: {
            switch(idx) {
            case 0:
                replace("SettingsGeneralPage.qml")
                break
            case 1:
                replace("SettingsUiPage.qml")
                break
            case 2:
                replace("SettingsSttPage.qml")
                break
            case 3:
                replace("SettingsTtsPage.qml")
                break
            case 4:
                replace("SettingsAccessebilityPage.qml")
                break
            case 5:
                replace("SettingsAdvancedPage.qml")
                break
            }

            fixer.fix()
        }

        Component.onCompleted: {
            idx = 0
        }
    }
}

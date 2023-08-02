/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
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

    RowLayout {
        spacing: 2 * appWin.padding
        Layout.fillWidth: true

        Image {
            Layout.alignment: Qt.AlignLeft
            source: _settings.app_icon()
        }

        ColumnLayout {
            id: info

            spacing: appWin.padding

            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: APP_NAME + " " + APP_VERSION
                font.pixelSize: Qt.application.font.pixelSize * 1.2
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Note taking, reading and translating with Speech to Text, Text to Speech and Machine Translation")
                font.pixelSize: Qt.application.font.pixelSize * 1.1
            }
        }
    }

    Button {
        text: qsTr("Changes")
        onClicked: appWin.openDialog("ChangelogPage.qml")
    }

    SectionLabel {
        text: qsTr("About")
    }

    InfoItem {
        label: qsTr("Project website:")
        value: "<a href='" + APP_WEBPAGE + "'>" + APP_WEBPAGE +
               "</a> or <a href='" + APP_WEBPAGE_ADDITIONAL + "'>" + APP_WEBPAGE_ADDITIONAL + "</a>"
    }

    InfoItem {
        label: qsTr("Report bugs at:")
        value: "<a href='" + APP_WEBPAGE + "/issues'>" + APP_WEBPAGE +
               "/issues</a> or <a href='" + APP_WEBPAGE_ADDITIONAL + "/-/issues'>" + APP_WEBPAGE_ADDITIONAL + "/-/issues</a>"
    }

    InfoItem {
        label: qsTr("Support e-mail:")
        value: "<a href='mailto:" + APP_SUPPORT_EMAIL + "'>" + APP_SUPPORT_EMAIL + "</a>"
    }

    Label {
        Layout.fillWidth: true
        textFormat: Text.StyledText
        text: qsTr("%1 is developed as an open source project under %2.")
        .arg(APP_NAME)
        .arg("<a href='" + APP_LICENSE_URL + "'>" + APP_LICENSE + "</a>")
        onLinkActivated: Qt.openUrlExternally(link)
        wrapMode: Text.Wrap
    }

    SectionLabel {
        text: qsTr("Authors")
    }

    Label {
        Layout.fillWidth: true
        textFormat: Text.RichText
        text: ("Copyright &copy; %1 %2")
        .arg(APP_COPYRIGHT_YEAR)
        .arg(APP_AUTHOR)
        wrapMode: Text.Wrap
    }

    SectionLabel {
        text: qsTr("Translators")
    }

    Label {
        Layout.fillWidth: true
        text: APP_TRANSLATORS_STR
        wrapMode: Text.Wrap
    }

    SectionLabel {
        text: qsTr("Libraries in use")
    }

    Label {
        Layout.fillWidth: true
        textFormat: Text.RichText
        wrapMode: Text.Wrap
        text: APP_LIBS_STR
    }
}

/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15

DialogPage {
    id: root

    objectName: "hello"

    title: qsTr("Welcome!")

    RichLabel {
        text: "<p>" + qsTr("%1 let you take, read and translate notes in multiple languages. " +
                   "It uses Speech to Text, Text to Speech and Machine Translation to do so.")
                   .arg("<i>" + qsTr("Speech Note") + "</i>") + "</p>" +
                   "<p>" + qsTr("Text and voice processing take place entirely offline, " +
                   "locally on your computer, without using a network connection. Your privacy is always " +
                   "respected. No data is sent to the Internet.") + "</p>" +
                   "<p>" + qsTr("To get started, you first need to set up the languages you want to use.") + "</p>" +
                   "<p>&rarr; " +
                   qsTr("Click the %1 button, select a language, and then download the model files you intend to use.")
                   .arg("<i>" + qsTr("Languages") + "</i>") + "</p>" +
                   "<p>" +
                   qsTr("To switch between %1 and %2 modes, use the toggle buttons in the upper right corner.")
                   .arg("<i>" + qsTr("Notepad") + "</i>").arg("<i>" + qsTr("Translator") + "</i>") + "</p>" +
                   "<p>" + qsTr("Have fun with %1!").arg("<i>" + qsTr("Speech Note") + "</i>") + "</p>"
    }
}

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

    SectionLabel {
        text: qsTr("Welcome!")
    }

    RichLabel {
        text: qsTr("<p><i>Speech Note</i> let you take and read notes with your voice. " +
                   "It uses Speech to Text and Text to Speech conversions to do so.</p>" +
                   "<p>All voice processing is entirely done off-line, locally on your " +
                   "computer without the use of network connection. Your privacy is always " +
                   "respected. No data is send to the Internet.</p>" +
                   "<p>To get started, you must first configure languages you'd like to use.</p>" +
                   "<p>&rarr; Click the <b>Languages</b> button, select a language and then download model files of your choice.</p>" +
                   "<p>Have fun with <i>Speech Note</i>!</p>")
    }
}

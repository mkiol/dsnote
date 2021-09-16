/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0

import harbour.dsnote.Dsnote 1.0

ApplicationWindow {
    id: root

    allowedOrientations: Orientation.All

    cover: Qt.resolvedUrl("CoverPage.qml")

    Component {
        id: notesPage
        NotesPage {}
    }

    initialPage: notesPage

    Notification {
        id: notification

        function show(summary) {
            notification.summary = summary
            notification.previewSummary = summary
            notification.publish()
        }
    }

    Dsnote {
        id: app

        onError: {
            switch (type) {
            case Dsnote.ErrorFileSource:
                notification.show("Audio file couldn't be transcribed.")
                break;
            case Dsnote.ErrorMicSource:
                notification.show("Couldn't connect to microphone")
                break;
            default:
                console.log("Unknown error")
            }
        }

        onTranscribe_done: notification.show("Audio file transcription is completed")
    }
}

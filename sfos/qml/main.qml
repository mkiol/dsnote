/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import harbour.dsnote.Dsnote 1.0

ApplicationWindow {
    id: root

    SttConfig {
        id: service
    }

    DsnoteApp {
        id: app
    }

    allowedOrientations: Orientation.All

    cover: Qt.resolvedUrl("CoverPage.qml")

    Component {
        id: notesPage
        NotesPage {}
    }

    initialPage: notesPage
}

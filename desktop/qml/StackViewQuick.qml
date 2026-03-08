/* Copyright (C) 2026 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


StackView {
    id: root

    property bool disableTransitions: true

    topPadding: 0
    topInset: 0
    verticalPadding: 0
    clip: true

    function fixDur(trans) {
        if (trans && trans.animations.length > 0 && trans.animations[0].duration && 
                trans.animations[0].duration > 200) {
            trans.animations[0].duration = 200
        }
    }

    Component.onCompleted: {
        if (disableTransitions) {
            popEnter = null
            popExit = null
            pushEnter = null
            pushExit = null
            replaceEnter = null
            replaceExit = null
        } else {
            root.currentItemChanged.connect(function() {
                fixDur(popEnter)
                fixDur(popExit)
                fixDur(pushEnter)
                fixDur(pushExit)
                fixDur(replaceEnter)
                fixDur(replaceExit)
            })
        }
    }
}
/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: root

    property alias title: titleLabel.text
    property alias value: valueLabel.text
    property alias description: descriptionLabel.text

    contentHeight: content.height + 2 * Theme.paddingLarge

    onClicked: openMenu();

    Column {
        id: content
        width: parent.width
        spacing: Theme.paddingSmall
        anchors.verticalCenter: parent.verticalCenter

        Flow {
            anchors {
                left: parent.left; right: parent.right
                leftMargin: Theme.horizontalPageMargin
                rightMargin: Theme.horizontalPageMargin
            }
            spacing: Theme.paddingMedium

            Label {
                id: titleLabel
            }

            Label {
                id: valueLabel
                color: Theme.highlightColor
            }
        }

        Label {
            id: descriptionLabel
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            wrapMode: Text.Wrap
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
        }
    }
}

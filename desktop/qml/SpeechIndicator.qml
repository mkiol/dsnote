/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0

Item {
    id: root

    property color color: "black"
    property bool active: false

    Component {
        id: indicator
        Rectangle {
            id: rect
            property double initValue: {
                if (model.modelData === 0)
                    return 0.5
                if (model.modelData === 1)
                    return 1.0
                if (model.modelData === 2)
                    return 0.5
            }
            property double value: initValue

            Connections {
                target: root
                function onActiveChanged() {
                    if (!active)
                        rect.value = rect.initValue
                }
            }

            radius: 10
            y: (root.height - height) / 2
            color: root.color
            opacity: root.active ? 1.0 : 0.5
            width: root.width / 4
            height: root.height * value

            SequentialAnimation {
                id: animation

                running: root.active
                loops: Animation.Infinite

                NumberAnimation {
                    target: rect
                    properties: "value"
                    from: model.modelData === 1 ? 1.0 : 0.5
                    to: model.modelData === 1 ? 0.5 : 1.0

                    duration: 200
                    easing {type: Easing.OutBack}
                }
                NumberAnimation {
                    target: rect
                    properties: "value"
                    from: model.modelData === 1 ? 0.5 : 1.0
                    to: model.modelData === 1 ? 1.0 : 0.5

                    duration: 200
                    easing {type: Easing.OutBack}
                }
            }
        }
    }

    Row {
        x: root.width / 16
        spacing: root.width / 16
        Repeater {
            delegate: indicator
            model: 3
        }
    }
}

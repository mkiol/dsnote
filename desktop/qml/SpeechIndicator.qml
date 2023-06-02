/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0
import QtQuick.Controls 2.15

Item {
    id: root

    // status values:
    // 0 - no speech, 1 - speech detected,
    // 2 - speech decoding, 3 - speech initializing,
    // 4 - speech playing
    property int status: 0

    property color color: "black"

    readonly property double _spacing: root.width / 7

    Component {
        id: waveIndicator
        Rectangle {
            id: waveRect
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
                function onStatusChanged() {
                    if (status !== 1 && status !== 4)
                        waveRect.value = waveRect.initValue
                }
            }

            radius: 10
            y: (root.height - height) / 2
            color: root.color
            opacity: root.status !== 0 ? 1.0 : 0.5
            width: (root.width - 2 * root._spacing) / 3
            height: root.height * value

            SequentialAnimation {
                running: root.status === 1 || root.status === 4
                loops: Animation.Infinite

                NumberAnimation {
                    target: waveRect
                    properties: "value"
                    from: model.modelData === 1 ? 1.0 : 0.5
                    to: model.modelData === 1 ? 0.5 : 1.0

                    duration: 200
                    easing {type: Easing.OutBack}
                }
                NumberAnimation {
                    target: waveRect
                    properties: "value"
                    from: model.modelData === 1 ? 0.5 : 1.0
                    to: model.modelData === 1 ? 1.0 : 0.5

                    duration: 200
                    easing {type: Easing.OutBack}
                }
            }
        }
    }

    Component {
        id: squareIndicator
        Rectangle {
            id: squareRect
            property int value: 0
            property int vid: {
                switch(model.modelData) {
                case 0:
                case 1:
                    return model.modelData
                case 2:
                    return model.modelData + 1
                case 3:
                   return model.modelData - 1
                }
            }

            Connections {
                target: root
                function onStatusChanged() {
                    if (status !== 2 && status !== 3)
                        squareRect.value = 0
                }
            }

            radius: 10
            color: root.color
            opacity: value == vid ? 1.0 : 0.0
            Behavior on opacity {
                 NumberAnimation { duration: 100 }
            }

            width: (root.width - root._spacing) / 2
            height: (root.height - root._spacing) / 2

            NumberAnimation {
                loops: Animation.Infinite
                running: root.status === 2 || root.status === 3
                target: squareRect
                properties: "value"
                from: 0
                to: 3

                duration: 800
            }
        }
    }

    Row {
        visible: root.status === 0 || root.status === 1 || root.status === 4
        spacing: root._spacing
        Repeater {
            delegate: waveIndicator
            model: 3
        }
    }

    Grid {
        visible: root.status === 2 || root.status === 3
        columns: 2
        spacing: root._spacing
        Repeater {
            delegate: squareIndicator
            model: 4
        }
    }
}

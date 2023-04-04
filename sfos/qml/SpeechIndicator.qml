/* Copyright (C) 2021-2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.0

Item {
    id: root

    // status values:
    // 0 - no speech, 1 - speech detected,
    // 2 - speech decoding, 3 - speech initializing
    property int status: 0

    property color color: "black"
    property bool off: false

    height: width * 0.55

    Component {
        id: waveIndicator
        Rectangle {
            id: waveRect
            property double value: {
                if (model.modelData === 0)
                    return 0.5
                if (model.modelData === 1)
                    return 1.0
                if (model.modelData === 2)
                    return 0.5
            }

            radius: root.width / 8
            y: (root.height - height) / 2
            color: root.color
            width: root.width / 6
            height: root.height * value
            opacity: root.status !== 0 ? 1.0 : 0.5
            Behavior on opacity { NumberAnimation { duration: 150 } }

            SequentialAnimation {
                id: animation

                running: root.status === 1
                loops: Animation.Infinite
                alwaysRunToEnd: true

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

            radius: root.width / 8
            //y: (root.height - height) / 2
            color: root.color
            opacity: value == vid ? 1.0 : 0.0
            Behavior on opacity {
                 NumberAnimation { duration: 100 }
            }

            width: root.width / 4
            height: root.height / 4

            NumberAnimation {
                loops: Animation.Infinite
                running: root.status === 2 || root.status === 3
                target: squareRect
                properties: "value"
                from: 0
                to: 3
                duration: 600
            }
        }
    }

    Row {
        visible: root.status === 0 || root.status === 1
        x: root.width / 8
        spacing: root.width / 8
        Repeater {
            delegate: waveIndicator
            model: 3
        }
    }


    Grid {
        visible: root.status === 2 || root.status === 3
        columns: 2
        anchors.centerIn: parent
        //x: root.width / 8
        spacing: root.width / 8
        Repeater {
            delegate: squareIndicator
            model: 4
        }
    }

    Rectangle {
        visible: root.off
        anchors.centerIn: parent
        width: root.width
        height: root.width / 12
        radius: height
        rotation: 45
        color: root.color
    }
}

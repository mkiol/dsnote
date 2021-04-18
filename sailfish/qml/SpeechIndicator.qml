import QtQuick 2.0

Item {
    id: root

    property color color: "black"
    property bool active: false

    Component {
        id: indicator
        Rectangle {
            id: rect
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
            opacity: root.active ? 1.0 : 0.5
            Behavior on opacity { NumberAnimation { duration: 150 } }

            SequentialAnimation {
                id: animation

                running: root.active
                loops: Animation.Infinite
                alwaysRunToEnd: true

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
        x: root.width / 8
        spacing: root.width / 8
        Repeater {
            delegate: indicator
            model: 3
        }
    }
}

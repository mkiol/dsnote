import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property bool invert
    property alias text: label.text
    property alias textColor: label.color
    property color backgroundColor: Theme.rgba(palette.highlightDimmerColor, 0.9)
    property alias palette: label.palette
    property int bottomMargin: invert ? width/3 : Theme.paddingLarge*2
    property int topMargin: invert ? Theme.paddingLarge*2 : width/3

    signal clicked()

    anchors.fill: parent
    opacity: enabled ? 1.0 : 0.0
    Behavior on opacity { FadeAnimator { duration: 100 } }

    Rectangle {
        anchors.bottom: parent.bottom
        height: implicitHeight
        implicitHeight: label.height + root.topMargin + root.bottomMargin
        width: parent.width
        gradient: Gradient {
            GradientStop { position: root.invert ? 1.0 : 0.0; color: "transparent" }
            GradientStop { position: root.invert ? 0.0 : 0.6; color: root.backgroundColor }
        }

        InfoLabel {
            id: label
            color: palette.highlightColor
            anchors {
                top: root.invert ? parent.top : undefined
                bottom: root.invert ? undefined : parent.bottom
                bottomMargin: root.bottomMargin
                topMargin: root.topMargin
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}

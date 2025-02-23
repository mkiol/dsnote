import QtQuick 2.0

Timer {
    id: root

    interval: 1
    onTriggered: {
        appWin.width -= 1
    }

    function fix() {
        appWin.width += 1
        start()
    }
}

QT += quick quickcontrols2

ROOT_DIR = $${PWD}/..
SRC_DIR = $${ROOT_DIR}/src

DEFINES += DESKTOP

include($${SRC_DIR}/dsnote.pri)

RESOURCES += qml.qrc

# Default rules for deployment.
target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

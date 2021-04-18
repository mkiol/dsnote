TARGET = harbour-dsnote

ROOT_DIR = $${PWD}/..
SRC_DIR = $${ROOT_DIR}/src

CONFIG += sailfishapp

include($${SRC_DIR}/dsnote.pri)

DISTFILES += \
    qml/*.qml \
    qml/ItemBox.qml \
    rpm/*.*

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

lib.files = $${LIB_DIR}/*
lib.path = /usr/share/$${TARGET}/lib
INSTALLS += lib

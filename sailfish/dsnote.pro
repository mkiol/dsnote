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

TRANSLATION_SOURCE_DIRS += $${ROOT_DIR}/src \
                           $${ROOT_DIR}/sailfish/qml \
                           $${ROOT_DIR}/desktop/qml
CONFIG += sailfishapp_i18n_include_obsolete
TRANSLATIONS_DIR = $${ROOT_DIR}/translations
TRANSLATIONS += \
    $${TRANSLATIONS_DIR}/dsnote-en.ts \
    $${TRANSLATIONS_DIR}/dsnote-pl.ts \
    $${TRANSLATIONS_DIR}/dsnote-zh_CN.ts
include(sailfishapp_i18n.pri)

lib.files = $${LIB_DIR}/*
lib.path = /usr/share/$${TARGET}/lib
INSTALLS += lib

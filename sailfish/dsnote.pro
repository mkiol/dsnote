TARGET = harbour-dsnote

CONFIG += sailfishapp

ROOT_DIR = $${PWD}/..
SRC_DIR = $${ROOT_DIR}/src
TRANSLATIONS_TS_DIR = $${ROOT_DIR}/translations

include($${SRC_DIR}/dsnote.pri)

DISTFILES += \
    qml/*.qml \
    rpm/*.* \
    $${ROOT_DIR}/systemd/*.* \
    $${ROOT_DIR}/dbus/*.*

RESOURCES += res.qrc

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

TRANSLATION_SOURCE_DIRS += $${ROOT_DIR}/src \
                           $${ROOT_DIR}/sailfish/qml \
                           $${ROOT_DIR}/desktop/qml
CONFIG += sailfishapp_i18n_include_obsolete
TRANSLATIONS += \
    $${TRANSLATIONS_TS_DIR}/dsnote-cs_CZ.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-en.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-pl.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-zh_CN.ts
include(sailfishapp_i18n.pri)

# install

install_libs.files = $${LIB_DIR}/*
install_libs.path = /usr/share/$${TARGET}/lib
INSTALLS += install_libs

install_systemd.path = /usr/lib/systemd/user
install_systemd.files = $${OUT_PWD}/systemd/$${TARGET}.service
install_systemd.CONFIG = no_check_exist
install_systemd.extra += mkdir -p $${OUT_PWD}/systemd && sed s/%TARGET%/\\\/usr\\\/bin\\\/$${TARGET}/g < $${ROOT_DIR}/systemd/dsnote.service > $${OUT_PWD}/systemd/$${TARGET}.service
INSTALLS += install_systemd

install_dbus.path = /usr/share/dbus-1/services
install_dbus.files = $${OUT_PWD}/dbus/org.mkiol.Stt.service
install_dbus.CONFIG = no_check_exist
install_dbus.extra += mkdir -p $${OUT_PWD}/dbus && sed s/%TARGET%/$${TARGET}/g < $${ROOT_DIR}/dbus/org.mkiol.Stt.service > $${OUT_PWD}/dbus/org.mkiol.Stt.service
INSTALLS += install_dbus

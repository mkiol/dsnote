TARGET = dsnote

TEMPLATE = app

QT += quick quickcontrols2

CONFIG(debug, debug|release) {
    CONFIG += sanitizer sanitize_address sanitize_undefined
}

ROOT_DIR = $${PWD}/..
SRC_DIR = $${ROOT_DIR}/src
QT_BIN_DIR = $${QT_INSTALL_PREFIX}/bin
TRANSLATION_SOURCE_DIRS += \
    $${ROOT_DIR}/src \
    $${ROOT_DIR}/sailfish/qml \
    $${ROOT_DIR}/sailfish/src \
    $${ROOT_DIR}/desktop/qml \
    $${ROOT_DIR}/desktop/src
TRANSLATIONS_TS_DIR = $${ROOT_DIR}/translations
TRANSLATIONS_QM_DIR = translations

DEFINES += DESKTOP

include($${SRC_DIR}/dsnote.pri)

RESOURCES += res.qrc

OTHER_FILES += \
    $${ROOT_DIR}/dbus/*.* \
    $${ROOT_DIR}/systemd/*.* \
    $${ROOT_DIR}/config/*.json \

TRANSLATIONS += \
    $${TRANSLATIONS_TS_DIR}/dsnote-cs_CZ.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-en.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-pl.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-zh_CN.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-fr.ts \
    $${TRANSLATIONS_TS_DIR}/dsnote-nl.ts

exists("$${QT_BIN_DIR}/lrelease-qt5") {
    LRELEASE_BIN = "$${QT_BIN_DIR}/lrelease-qt5"
} else {
    LRELEASE_BIN = "$${QT_BIN_DIR}/lrelease"
}

exists("$${QT_BIN_DIR}/lupdate-qt5") {
    LUPDATE_BIN = "$${QT_BIN_DIR}/lupdate-qt5"
} else {
    LUPDATE_BIN = "$${QT_BIN_DIR}/lupdate"
}

system(mkdir -p $${TRANSLATIONS_TS_DIR})
system(mkdir -p $${TRANSLATIONS_QM_DIR})
for(dir, TRANSLATION_SOURCE_DIRS) {
    exists($$dir) {
        TRANSLATION_SOURCES += $$clean_path($$dir)
    }
}
system("$$LUPDATE_BIN" -no-obsolete $${TRANSLATION_SOURCES} -ts $$TRANSLATIONS)
for(t, TRANSLATIONS) {
    qmfile = $$replace(t, \.ts, .qm)
    system("$$LRELEASE_BIN" "$$t" -qm $${TRANSLATIONS_QM_DIR}/$$basename(qmfile))
}

# install

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

install_bin.path = $${PREFIX}/bin
install_bin.files = $${OUT_PWD}/$${TARGET}
install_bin.CONFIG = executable

install_desktop.path = $${PREFIX}/share/applications
install_desktop.files = $${TARGET}.desktop

install_icons.path = $${PREFIX}/share/icons/hicolor/scalable/apps
install_icons.files = $${TARGET}.svg

install_libs.path = $${PREFIX}/lib
install_libs.files = $${ROOT_DIR}/libs/amd64/*.so

BIN_DIR = $${PREFIX}/bin/
USER_UNIT_DIR = $${PREFIX}/lib/systemd/user
install_systemd.path = $${USER_UNIT_DIR}
install_systemd.files = $${OUT_PWD}/systemd/$${TARGET}.service
install_systemd.CONFIG = no_check_exist
install_systemd.extra += mkdir -p $${OUT_PWD}/systemd && sed s/%TARGET%/$$replace(BIN_DIR, /, \\\/)$${TARGET}/g < $${ROOT_DIR}/systemd/$${TARGET}.service > $${OUT_PWD}/systemd/$${TARGET}.service

install_dbus.path = $${PREFIX}/share/dbus-1/services
install_dbus.files = $${OUT_PWD}/dbus/org.mkiol.Stt.service
install_dbus.CONFIG = no_check_exist
install_dbus.extra += mkdir -p $${OUT_PWD}/dbus && sed s/%TARGET%/$${TARGET}/g < $${ROOT_DIR}/dbus/org.mkiol.Stt.service > $${OUT_PWD}/dbus/org.mkiol.Stt.service

INSTALLS += install_bin install_desktop install_icons install_libs install_systemd install_dbus

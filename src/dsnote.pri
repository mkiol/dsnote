QT += dbus multimedia
CONFIG += c++1z

contains(QT_ARCH, i386){
   CONFIG += x86
   DEFINES += X86
   ARCH = x86
} else {
   contains(QT_ARCH, x86_64) {
       CONFIG += amd64
       DEFINES += X86
       ARCH = amd64
   } else {
       contains(QT_ARCH, arm){
           CONFIG += arm
           DEFINES += ARM
           ARCH = arm
       } else {
            contains(QT_ARCH, arm64) {
                CONFIG += arm64
                DEFINES += ARM
                ARCH = arm64
            }
       }
   }
}

ROOT_DIR = $${PWD}/..
SRC_DIR = $${ROOT_DIR}/src
LIB_DIR = $${ROOT_DIR}/libs/$${ARCH}
DBUS_DIR = $${ROOT_DIR}/dbus

#QT_BIN_DIR = "$${QT_INSTALL_PREFIX}/bin"
#exists("$${QT_BIN_DIR}/qdbusxml2cpp-qt5") {
#    QDBUSXML2CPP = "$${QT_BIN_DIR}/qdbusxml2cpp-qt5"
#} else {
#    QDBUSXML2CPP = "$${QT_BIN_DIR}/qdbusxml2cpp"
#}
#DBUS_STT_XML = "$${DBUS_DIR}/org.mkiol.Stt.xml"
#message("$${QDBUSXML2CPP}" "$${DBUS_STT_XML}" -a $${SRC_DIR}/dbus_stt_adaptor)
#system("$${QDBUSXML2CPP}" "$${DBUS_STT_XML}" -a $${SRC_DIR}/dbus_stt_adaptor)
#message("$${QDBUSXML2CPP}" "$${DBUS_STT_XML}" -p $${SRC_DIR}/dbus_stt_inf)
#system("$${QDBUSXML2CPP}" "$${DBUS_STT_XML}" -p $${SRC_DIR}/dbus_stt_inf)

SOURCES += \
    $${SRC_DIR}/log.cpp \
    $${SRC_DIR}/stt_service.cpp \
    $${SRC_DIR}/stt_config.cpp \
    $${SRC_DIR}/file_source.cpp \
    $${SRC_DIR}/deepspeech_wrapper.cpp \
    $${SRC_DIR}/main.cpp \
    $${SRC_DIR}/mic_source.cpp \
    $${SRC_DIR}/models_manager.cpp \
    $${SRC_DIR}/settings.cpp \
    $${SRC_DIR}/dsnote_app.cpp \
    $${SRC_DIR}/dbus_stt_adaptor.cpp \
    $${SRC_DIR}/dbus_stt_inf.cpp \
    $${SRC_DIR}/listmodel.cpp \
    $${SRC_DIR}/itemmodel.cpp \
    $${SRC_DIR}/models_list_model.cpp \
    $${SRC_DIR}/langs_list_model.cpp

HEADERS += \
    $${SRC_DIR}/log.h \
    $${SRC_DIR}/stt_service.h \
    $${SRC_DIR}/stt_config.h \
    $${SRC_DIR}/audio_source.h \
    $${SRC_DIR}/file_source.h \
    $${SRC_DIR}/info.h \
    $${SRC_DIR}/deepspeech_wrapper.h \
    $${SRC_DIR}/mic_source.h \
    $${SRC_DIR}/models_manager.h \
    $${SRC_DIR}/settings.h \
    $${SRC_DIR}/dsnote_app.h \
    $${SRC_DIR}/coqui-stt.h \
    $${SRC_DIR}/dbus_stt_adaptor.h \
    $${SRC_DIR}/dbus_stt_inf.h \
    $${SRC_DIR}/listmodel.h \
    $${SRC_DIR}/itemmodel.h \
    $${SRC_DIR}/models_list_model.h \
    $${SRC_DIR}/langs_list_model.h

sailfishapp {
    DEFINES += SAILFISH

    SOURCES += \
        $${SRC_DIR}/dirmodel.cpp

    HEADERS += \
        $${SRC_DIR}/dirmodel.h
}

LIBS += \
    -lz -llzma -larchive \
    -L$${LIB_DIR} -l:libstt.so -l:libtensorflowlite.so \
    -l:libtflitedelegates.so -l:libkenlm.so

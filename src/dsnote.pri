QT += multimedia
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

SOURCES += \
    $${SRC_DIR}/file_source.cpp \
    $${SRC_DIR}/deepspeech_wrapper.cpp \
    $${SRC_DIR}/main.cpp \
    $${SRC_DIR}/mic_source.cpp \
    $${SRC_DIR}/models_manager.cpp \
    $${SRC_DIR}/settings.cpp \
    $${SRC_DIR}/dsnote.cpp

HEADERS += \
    $${SRC_DIR}/audio_source.h \
    $${SRC_DIR}/file_source.h \
    $${SRC_DIR}/info.h \
    $${SRC_DIR}/deepspeech_wrapper.h \
    $${SRC_DIR}/mic_source.h \
    $${SRC_DIR}/models_manager.h \
    $${SRC_DIR}/settings.h \
    $${SRC_DIR}/dsnote.h

sailfishapp {
    DEFINES += TF_LITE SAILFISH

    SOURCES += \
        $${SRC_DIR}/listmodel.cpp \
        $${SRC_DIR}/itemmodel.cpp \
        $${SRC_DIR}/dirmodel.cpp

    HEADERS += \
        $${SRC_DIR}/listmodel.h \
        $${SRC_DIR}/itemmodel.h \
        $${SRC_DIR}/dirmodel.h
}

LIBS += -lz -llzma -larchive -L$${LIB_DIR} -l:libdeepspeech.so

set(mbrola_source_url "https://github.com/numediart/MBROLA/archive/bf17e9e1416a647979ac683657a536e8ca5d880e.zip")
set(mbrola_checksum "5a7c02a926dc48ab6d1af0e4c8ab53fc191a7e4337de6df57b6706e140fa3087")

set(espeak_source_url "https://github.com/rhasspy/espeak-ng/archive/8593723f10cfd9befd50de447f14bf0a9d2a14a4.zip")
set(espeak_checksum "cc8092f23a28ccd79b1c5e62984a4c4ac1959d2d0b8193ac208d728c620bd5ed")

if(${autoconf_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "autoconf not found but it is required to build espeak")
endif()
if(${automake_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "automake not found but it is required to build espeak")
endif()
if(${libtool_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "libtool not found but it is required to build espeak")
endif()

ExternalProject_Add(mbrola
    SOURCE_DIR ${external_dir}/mbrola
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/mbrola
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${mbrola_source_url}
    URL_HASH SHA256=${mbrola_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR>
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND mkdir -p ${external_bin_dir} && cp <BINARY_DIR>/Bin/mbrola ${external_bin_dir}
)

ExternalProject_Add(espeak
    SOURCE_DIR ${external_dir}/espeak
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/espeak
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${espeak_source_url}
    URL_HASH SHA256=${espeak_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> &&
        <BINARY_DIR>/autogen.sh &&
        <BINARY_DIR>/configure --prefix=<INSTALL_DIR> --libdir=<INSTALL_DIR>/lib --with-pic
        --with-pcaudiolib=no --with-sonic=no --with-speechplayer=no
        --with-mbrola=yes --enable-static=yes --enable-shared=yes
        --with-extdict-ru --enable-rpath=no
    BUILD_COMMAND export LD_LIBRARY_PATH=<BINARY_DIR>/src/.libs && make
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

ExternalProject_Add_StepDependencies(espeak configure mbrola)

list(APPEND deps_libs "${external_lib_dir}/libespeak-ng.a")
list(APPEND deps espeak mbrola)

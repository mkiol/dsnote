set(mbrola_source_url "https://github.com/numediart/MBROLA/archive/refs/tags/3.3.tar.gz")
set(mbrola_checksum "06993903c7b8d3a8d21cc66cd5a28219")

set(espeak_source_url "https://github.com/rhasspy/espeak-ng/archive/61504f6b76bf9ebbb39b07d21cff2a02b87c99ff.zip")
set(espeak_checksum "56e002023d76dc166f1843d066b8b1d6")

ExternalProject_Add(mbrola
    SOURCE_DIR ${external_dir}/mbrola
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/mbrola
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${mbrola_source_url}"
    URL_MD5 "${mbrola_checksum}"
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR>
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND mkdir -p ${external_bin_dir} && cp <BINARY_DIR>/Bin/mbrola ${external_bin_dir}
)

ExternalProject_Add(espeak
    SOURCE_DIR ${external_dir}/espeak
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/espeak
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${espeak_source_url}"
    URL_MD5 "${espeak_checksum}"
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> &&
        <BINARY_DIR>/autogen.sh &&
        <BINARY_DIR>/configure --prefix=<INSTALL_DIR> --with-pic
        --with-pcaudiolib=no --with-sonic=no --with-speechplayer=no
        --with-mbrola=yes --enable-static --with-extdict-ru
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

ExternalProject_Add_StepDependencies(espeak configure mbrola)

list(APPEND deps_libs "${external_lib_dir}/libespeak-ng.a")
list(APPEND deps espeak mbrola)

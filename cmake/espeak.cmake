set(mbrola_source_url "https://github.com/numediart/MBROLA/archive/refs/tags/3.3.tar.gz")
set(mbrola_checksum "06993903c7b8d3a8d21cc66cd5a28219")

set(piper_source_url "https://github.com/rhasspy/piper/archive/e967bdc1a838023fe3644f8ed7cf6e1e958b42ef.zip")
set(piper_checksum "c9924cdb6305f6982171d486d5d633e1")

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
    URL "${piper_source_url}"
    URL_MD5 "${piper_checksum}"
    CONFIGURE_COMMAND tar -x -C <BINARY_DIR> --strip=1 -f <SOURCE_DIR>/lib/espeak-ng-1.52-patched.tar.gz &&
        ./autogen.sh &&
        ./configure --prefix=<INSTALL_DIR> --with-pic --with-pcaudiolib=no --enable-static
            --disable-shared --disable-rpath --with-extdict-ru --with-extdict-zh --with-extdict-zhy
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

ExternalProject_Add_StepDependencies(espeak configure mbrola)

list(APPEND deps_libs "${external_lib_dir}/libespeak-ng.a")
list(APPEND deps espeak mbrola)

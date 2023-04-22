set(rnnoise_source_url "https://github.com/GregorR/rnnoise-nu/archive/26269304e120499485438cd93acf5127c6908c68.zip")
set(rnnoise_checksum "fafe9bbf0e2b15df4a628434bea99dd3")

ExternalProject_Add(rnnoise
    SOURCE_DIR ${external_dir}/rnnoise
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/rnnoise
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${rnnoise_source_url}"
    URL_MD5 "${rnnoise_checksum}"
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> && <BINARY_DIR>/autogen.sh &&
        <BINARY_DIR>/configure --prefix=<INSTALL_DIR>
        --disable-examples --disable-doc --disable-shared --enable-static --with-pic
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

list(APPEND deps_libs "${external_lib_dir}/librnnoise-nu.a")

list(APPEND deps rnnoise)

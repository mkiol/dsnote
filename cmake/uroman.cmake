set(uroman_source_url "https://github.com/isi-nlp/uroman/archive/refs/tags/v1.2.8.tar.gz")
set(uroman_checksum "08e5058341428f3f4ca2401d409df1f4341c386853c4de836e7ab2c2a750cb88")

ExternalProject_Add(uroman
    SOURCE_DIR ${external_dir}/uroman
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/uroman
    INSTALL_DIR ${external_share_dir}/uroman
    URL "${uroman_source_url}"
    URL_HASH SHA256=${uroman_checksum}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND mkdir -p <INSTALL_DIR>
        && cp -r <SOURCE_DIR>/bin <INSTALL_DIR>/
        && cp -r <SOURCE_DIR>/data <INSTALL_DIR>/
        && cp -r <SOURCE_DIR>/lib <INSTALL_DIR>/
)

list(APPEND deps uroman)

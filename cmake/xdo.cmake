set(xdo_source_url "https://github.com/jordansissel/xdotool/archive/33092d8a74d60c9ad3ab39c4f05b90e047ea51d8.zip")
set(xdo_checksum "cba1c7d0c23cc60cd0b1f5483f84296ff91360598e2c19c57395aec530af99ad")

ExternalProject_Add(xdo
    SOURCE_DIR ${external_dir}/xdo
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/xdo
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${xdo_source_url}
    URL_HASH SHA256=${xdo_checksum}
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR>
    BUILD_COMMAND make libxdo.a
    BUILD_ALWAYS False
    INSTALL_COMMAND make PREFIX=<INSTALL_DIR> install && cp <BINARY_DIR>/libxdo.a ${external_lib_dir}/
)

list(APPEND deps_libs "${external_lib_dir}/libxdo.a")
list(APPEND deps xdo)

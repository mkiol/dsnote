set(maddy_source_url "https://github.com/progsource/maddy/archive/refs/tags/1.3.0.tar.gz")
set(maddy_checksum "561681f8c8d2b998c153cda734107a0bc1dea4bb0df69fd813922da63fa9f3e7")

ExternalProject_Add(maddy
    SOURCE_DIR ${external_dir}/maddy
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/maddy
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${maddy_source_url}
    URL_HASH SHA256=${maddy_checksum}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND cp -r <SOURCE_DIR>/include/maddy ${external_include_dir}/
)

list(APPEND deps maddy)

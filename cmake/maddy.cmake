set(maddy_source_url "https://github.com/progsource/maddy/archive/refs/tags/1.3.0.tar.gz")
set(maddy_checksum "561681f8c8d2b998c153cda734107a0bc1dea4bb0df69fd813922da63fa9f3e7")

ExternalProject_Add(maddy
    SOURCE_DIR ${external_dir}/maddy
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/maddy
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${maddy_source_url}
    URL_HASH SHA256=${maddy_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND cp -r ${external_dir}/maddy/include/maddy ${external_include_dir}/
    BUILD_COMMAND cp -r ${external_dir}/maddy/include/maddy ${external_include_dir}/
    INSTALL_COMMAND cp -r ${external_dir}/maddy/include/maddy ${external_include_dir}/
)

list(APPEND deps maddy)

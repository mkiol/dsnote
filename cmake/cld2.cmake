set(cld2_source_url "https://github.com/CLD2Owners/cld2/archive/b56fa78a2fe44ac2851bae5bf4f4693a0644da7b.zip")
set(cld2_checksum "6d8681eadbb64d8fcd3c6c620e81bed16d99ff75627324d66ee2d54bf2b7d749")

ExternalProject_Add(cld2
    SOURCE_DIR ${external_dir}/cld2
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/cld2
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${cld2_source_url}"
    URL_HASH SHA256=${cld2_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/cld2.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_ALWAYS False
)

list(APPEND deps_libs ${external_lib_dir}/libcld2_api.a)
list(APPEND deps cld2)

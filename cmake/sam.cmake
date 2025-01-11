set(sam_source_url "https://github.com/s-macke/SAM/archive/a7b36efac730957b59471a42a45fd779f94d77dd.zip")
set(sam_checksum "342e245c84b14945b33dd8b272373eee517d239c612763ec01d3fa3bbee6bcb1")

ExternalProject_Add(sam
    SOURCE_DIR ${external_dir}/sam
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/sam
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${sam_source_url}"
    URL_HASH SHA256=${sam_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/sam.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_ALWAYS False
)

list(APPEND deps_libs ${external_lib_dir}/libsam_api.a)
list(APPEND deps sam)

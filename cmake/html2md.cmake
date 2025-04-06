set(html2md_source_url "https://github.com/tim-gromeyer/html2md/archive/refs/tags/v1.6.4.tar.gz")
set(html2md_checksum "e34c80981d6ee5f4a699985dc4a68b0b280542a5c5e5780fdebc3ba0ca30553d")

ExternalProject_Add(html2md
    SOURCE_DIR ${external_dir}/html2md
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/html2md
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${html2md_source_url}
    URL_HASH SHA256=${html2md_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/html2md.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_EXE=OFF
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    BUILD_ALWAYS False
)

list(APPEND deps_libs "${external_lib_dir}/libhtml2md.a")
list(APPEND deps html2md)

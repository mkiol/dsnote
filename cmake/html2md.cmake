set(html2md_source_url "https://github.com/tim-gromeyer/html2md/archive/refs/tags/v1.5.3.tar.gz")
set(html2md_checksum "9853eeb1d2b1ca34ee7143521d03daacbcfac57a1763180ebd0730595cb46961")

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
    BUILD_ALWAYS False
)

list(APPEND deps_libs "${external_lib_dir}/libhtml2md.a")
list(APPEND deps html2md)

set(html2md_source_url "https://github.com/tim-gromeyer/html2md/archive/refs/tags/v1.7.0.tar.gz")
set(html2md_checksum "234aa636afa2c6e22c842d8870e93c62a9c20267a2c7367218118a4b4fd1882c")

ExternalProject_Add(html2md
    SOURCE_DIR ${external_dir}/html2md
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/html2md
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${html2md_source_url}
    URL_HASH SHA256=${html2md_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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

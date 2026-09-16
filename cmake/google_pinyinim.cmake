set(google_pinyinim_source_url "https://github.com/aron566/google_pinyinim/archive/1a4fa548031ad53253003f0ad1cc66de0fd42a93.zip")
set(google_pinyinim_checksum "b1d09d99421c36bd91856dc21061a8e692d17dba56cb3357a9fd27623794504d")

ExternalProject_Add(google_pinyinim
    SOURCE_DIR ${external_dir}/google_pinyinim
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/google_pinyinim
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${google_pinyinim_source_url}"
    URL_HASH SHA256=${google_pinyinim_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/google_pinyinim.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DQT_VERSION_MAJOR=${QT_VERSION_MAJOR}
    BUILD_ALWAYS False
)

list(APPEND deps_libs ${external_lib_dir}/libgoogle_pinyinim_api.a)
list(APPEND deps google_pinyinim)

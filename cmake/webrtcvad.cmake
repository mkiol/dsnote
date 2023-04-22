set(webrtcvad_source_url "https://github.com/webrtc-mirror/webrtc/archive/ac87c8df2780cb12c74942ec8a473718c76cb5b7.zip")
set(webrtcvad_checksum "f1489c137b354594632d260978b283a4")

ExternalProject_Add(webrtcvad
    SOURCE_DIR ${external_dir}/webrtcvad
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/webrtcvad
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${webrtcvad_source_url}"
    URL_MD5 "${webrtcvad_checksum}"
    PATCH_COMMAND patch --forward --unified -p0 <SOURCE_DIR>/CMakeLists.txt
                -i ${patches_dir}/webrtcvad.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_ALWAYS False
)

list(APPEND deps_libs ${external_lib_dir}/libvad.a)
list(APPEND deps webrtcvad)

set(qhotkey_source_url "https://github.com/Skycoder42/QHotkey/archive/cd72a013275803fce33e028fc8b05ae32248da1f.zip")
set(qhotkey_checksum "55995114e8b5947cb6a59a19a53840eaf81fddc420fd3d180dcb0db9d8d33e13")

ExternalProject_Add(qhotkey
    SOURCE_DIR ${external_dir}/qhotkey
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/qhotkey
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${qhotkey_source_url}
    URL_HASH SHA256=${qhotkey_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/qhotkey.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DQT_DEFAULT_MAJOR_VERSION=6
    BUILD_ALWAYS False
)

# Qt6 removed X11Extras - functionality moved to Qt6::Gui
# No additional Qt library needed for X11 support

list(APPEND deps_libs "${external_lib_dir}/libqhotkey.a")
list(APPEND deps qhotkey)

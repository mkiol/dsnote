set(qhotkey_source_url "https://github.com/Skycoder42/QHotkey/archive/4e3a244d87f1f7e741e1395f2ffe825f3a8ada45.zip")
set(qhotkey_checksum "6055b2b91b955b8e1b24674c4250a9da87fc7e754a8001ef867e7edab934e854")

ExternalProject_Add(qhotkey
    SOURCE_DIR ${external_dir}/qhotkey
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/qhotkey
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${qhotkey_source_url}
    URL_HASH SHA256=${qhotkey_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/qhotkey.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DQT_DEFAULT_MAJOR_VERSION=${QT_VERSION_MAJOR}
    BUILD_ALWAYS False
)

# Qt6 removed X11Extras - functionality moved to Qt6::Gui
# Qt5 needs X11Extras (handled in main CMakeLists.txt) for X11 support

list(APPEND deps_libs "${external_lib_dir}/libqhotkey.a")
list(APPEND deps qhotkey)

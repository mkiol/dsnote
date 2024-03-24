set(qqc2breeze_source_url "https://invent.kde.org/plasma/qqc2-breeze-style/-/archive/752f8636203df7c358a14d5312f304d79437d869/qqc2-breeze-style-752f8636203df7c358a14d5312f304d79437d869.tar.bz2")
set(qqc2breeze_checksum "e9e2acf87f1ba16c72ac2453c49b8a872669f8c21cd7aef47fd96ca89028d48f")

set(kquickcharts_source_url "https://invent.kde.org/frameworks/kquickcharts/-/archive/v5.115.0/kquickcharts-v5.115.0.tar.bz2")
set(kquickcharts_checksum "060907025f3939532a4cefc2b5e852bcb2adbdb0e6bea22da508abab903a0f3e")

ExternalProject_Add(qqc2breeze
    SOURCE_DIR ${external_dir}/qqc2breeze
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/qqc2breeze
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${qqc2breeze_source_url}"
    URL_HASH SHA256=${qqc2breeze_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
    BUILD_ALWAYS False
)

ExternalProject_Add(kquickcharts
    SOURCE_DIR ${external_dir}/kquickcharts
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/kquickcharts
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${kquickcharts_source_url}"
    URL_HASH SHA256=${kquickcharts_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
    BUILD_ALWAYS False
)

list(APPEND deps qqc2breeze kquickcharts)

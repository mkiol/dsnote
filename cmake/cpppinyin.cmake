set(cpppinyin_source_url "https://github.com/wolfgitpr/cpp-pinyin/archive/e4dd8310e3bb6b8ac8edc42f2cbd80be92586a8e.zip")
set(cpppinyin_checksum "f831378e4af5e3609557eda84858439ca55441d11e3e9859b52b4420c547ede3")

ExternalProject_Add(cpppinyin
    SOURCE_DIR ${external_dir}/cpppinyin
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/cpppinyin
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${cpppinyin_source_url}"
    URL_HASH SHA256=${cpppinyin_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_SHARED_LIBS=OFF
    BUILD_ALWAYS False
)

list(APPEND deps_libs ${external_lib_dir}/libcpp-pinyin.a)
list(APPEND deps cpppinyin)

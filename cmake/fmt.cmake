set(fmt_source_url "https://github.com/fmtlib/fmt/releases/download/11.1.3/fmt-11.1.3.zip")
set(fmt_checksum "7df2fd3426b18d552840c071c977dc891efe274051d2e7c47e2c83c3918ba6df")

ExternalProject_Add(fmt
    SOURCE_DIR ${external_dir}/fmt
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/fmt
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${fmt_source_url}"
    URL_HASH SHA256=${fmt_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/ -DCMAKE_INSTALL_LIBDIR=/lib
        -DCMAKE_INSTALL_INCLUDEDIR=/include -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFMT_DOC=OFF
        -DFMT_TEST=OFF -DFMT_FUZZ=OFF
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=${PROJECT_BINARY_DIR}/external install/local
)

list(APPEND deps_libs ${external_lib_dir}/libfmt.a)
list(APPEND deps fmt)

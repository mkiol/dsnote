set(fmt_source_url "https://github.com/fmtlib/fmt/releases/download/11.2.0/fmt-11.2.0.zip")
set(fmt_checksum "203eb4e8aa0d746c62d8f903df58e0419e3751591bb53ff971096eaa0ebd4ec3")

ExternalProject_Add(fmt
    SOURCE_DIR ${external_dir}/fmt
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/fmt
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${fmt_source_url}"
    URL_HASH SHA256=${fmt_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/ -DCMAKE_INSTALL_LIBDIR=/lib
        -DCMAKE_INSTALL_INCLUDEDIR=/include -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFMT_DOC=OFF
        -DFMT_TEST=OFF -DFMT_FUZZ=OFF
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=${PROJECT_BINARY_DIR}/external install/local
)

list(APPEND deps_libs ${external_lib_dir}/libfmt.a)
list(APPEND deps fmt)

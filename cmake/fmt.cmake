set(fmt_source_url "https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip")
set(fmt_checksum "6133244fe8ef6f75c5601e8069b37b04")

ExternalProject_Add(fmt
    SOURCE_DIR ${external_dir}/fmt
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/fmt
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${fmt_source_url}"
    URL_MD5 "${fmt_checksum}"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFMT_DOC=OFF -DFMT_TEST=OFF -DFMT_FUZZ=OFF
    BUILD_ALWAYS False
)

list(APPEND deps_libs ${external_lib_dir}/libfmt.a)
list(APPEND deps fmt)

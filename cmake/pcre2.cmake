set(pcre2_source_url "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.45/pcre2-10.45.tar.bz2")
set(pcre2_checksum "21547f3516120c75597e5b30a992e27a592a31950b5140e7b8bfde3f192033c4")

ExternalProject_Add(pcre2
    SOURCE_DIR ${external_dir}/pcre2
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/pcre2
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${pcre2_source_url}
    URL_HASH SHA256=${pcre2_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=<INSTALL_DIR>/lib
        -DCMAKE_LIBRARY_PATH=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DPCRE2_STATIC_PIC=ON -DBUILD_STATIC_LIBS=ON
        -DPCRE2_SUPPORT_LIBBZ2=OFF -DPCRE2_SUPPORT_LIBZ=OFF -DPCRE2_SUPPORT_JIT=ON
    BUILD_ALWAYS False
)

list(APPEND deps_libs "${external_lib_dir}/libpcre2-8.a" "${external_lib_dir}/libpcre2-posix.a")
list(APPEND deps pcre2)

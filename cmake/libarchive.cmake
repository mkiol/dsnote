set(libarchive_source_url "https://libarchive.org/downloads/libarchive-3.6.2.tar.gz")
set(libarchive_checksum "b5b8efa8cba29396816d0dd5f61f3de3")

ExternalProject_Add(libarchive
    SOURCE_DIR ${external_dir}/libarchive
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/libarchive
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${libarchive_source_url}"
    URL_MD5 "${libarchive_checksum}"
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --bindir=<INSTALL_DIR>/bin
        --enable-shared=no --enable-static=yes --disable-acl --disable-xattr --disable-largefile
        --without-zstd --without-lz4 --without-libiconv-prefix --without-iconv --without-libb2
        --without-bz2lib --with-zlib --without-cng --without-openssl --without-xml2 --without-expat
        --without-lzma --with-pic=yes
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

list(APPEND deps libarchive)

list(APPEND deps_libs ${external_lib_dir}/libarchive.a)

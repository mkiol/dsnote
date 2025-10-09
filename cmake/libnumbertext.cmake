set(libnumbertext_source_url "https://github.com/Numbertext/libnumbertext/archive/a4b0225813b015a0f796754bc6718be20dd9943c.zip")
set(libnumbertext_checksum "eb4f91e4c97bb47f8cb7a31d80fdd500024bd9160c9045cdd8dff28beeac4b7a")

if(${autoconf_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "autoconf not found but it is required to build libnumbertext")
endif()
if(${automake_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "automake not found but it is required to build libnumbertext")
endif()
if(${libtool_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "libtool not found but it is required to build libnumbertext")
endif()

ExternalProject_Add(libnumbertext
    SOURCE_DIR ${external_dir}/libnumbertext
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/libnumbertext
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${libnumbertext_source_url}
    URL_HASH SHA256=${libnumbertext_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> && autoreconf -i &&
        <BINARY_DIR>/configure --prefix=<INSTALL_DIR> --libdir=<INSTALL_DIR>/lib
        --enable-shared=false --enable-static=true --with-pic=yes
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

list(APPEND deps_libs "${external_lib_dir}/libnumbertext-1.0.a")
list(APPEND deps libnumbertext)

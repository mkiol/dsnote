set(libpinyin_version 2.11.92)
set(libpinyin_source_url "https://github.com/libpinyin/libpinyin/releases/download/v${libpinyin_version}/libpinyin-${libpinyin_version}.tar.gz")
set(libpinyin_checksum "3fca1818aec7f17dfe807d9707adcb279bb8dcd6a2931cb9a3d2bfadc50029e9")

set(libpinyinmodel_version 20)
set(libpinyinmodel_filename model${libpinyinmodel_version}.text.tar.gz)
set(libpinyinmodel_url "https://downloads.sourceforge.net/libpinyin/${libpinyinmodel_filename}")
set(libpinyinmodel_url_checksum "59c68e89d43ff85f5a309489499cbcde282d2b04bd91888734884b7defcb1155")

set(kyotocabinet_version 1.2.80)
set(kyotocabinet_source_url "https://dbmx.net/kyotocabinet/pkg/kyotocabinet-${kyotocabinet_version}.tar.gz")
set(kyotocabinet_checksum "4c85d736668d82920bfdbdb92ac3d66b7db1108f09581a769dd9160a02def349")

ExternalProject_Add(kyotocabinet
    SOURCE_DIR ${external_dir}/kyotocabinet
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/kyotocabinet
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${kyotocabinet_source_url}"
    URL_HASH SHA256=${kyotocabinet_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_HANDLED_BY_BUILD true
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> &&
        <SOURCE_DIR>/configure
        LDFLAGS=-L/usr/lib64
        --prefix=<INSTALL_DIR> --libdir=<INSTALL_DIR>/lib --srcdir=<SOURCE_DIR>
        --disable-static --enable-shared
        --disable-zlib --disable-lzo --disable-lzma --disable-opt
    BUILD_COMMAND make -j
    INSTALL_COMMAND make DESTDIR=/ install && 
        rm -rf ${external_lib_dir}/libkyotocabinet.so ${external_lib_dir}/libkyotocabinet.so.16 ${external_lib_dir}/libkyotocabinet.so.16.14.0
    BUILD_ALWAYS False
)

set(libpinyinmodel_dir ${PROJECT_BINARY_DIR}/external/libpinyinmodel)

ExternalProject_Add(libpinyinmodel
    BINARY_DIR ${libpinyinmodel_dir}
    URL "${libpinyinmodel_url}"
    URL_HASH SHA256=${libpinyinmodel_url_checksum}
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_DIR ${libpinyinmodel_dir}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(libpinyin
    SOURCE_DIR ${external_dir}/libpinyin
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/libpinyin
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${libpinyin_source_url}"
    URL_HASH SHA256=${libpinyin_checksum}
    DEPENDS libpinyinmodel
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR> -i ${patches_dir}/libpinyin.patch
    CONFIGURE_HANDLED_BY_BUILD true
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
        PKG_CONFIG_PATH=${external_lib_dir}/pkgconfig
        LIBRARY_PATH=${external_lib_dir}
        LDFLAGS=-L${external_lib_dir}
        CFLAGS=-fPIC
        CXXFLAGS=-fPIC
        --prefix=<INSTALL_DIR> --libdir=<INSTALL_DIR>/lib --srcdir=<SOURCE_DIR>
        --enable-static --disable-shared
        --with-dbm=KyotoCabinet --enable-libzhuyin
    BUILD_COMMAND ln -sf ${libpinyinmodel_dir}/${libpinyinmodel_filename} <BINARY_DIR>/data/${libpinyinmodel_filename} && 
        make LIBRARY_PATH=${external_lib_dir} -j
    INSTALL_COMMAND make LIBRARY_PATH=${external_lib_dir} DESTDIR=/ install &&
        cp <BINARY_DIR>/src/lookup/liblookup.a ${external_lib_dir} &&
        cp <BINARY_DIR>/src/storage/libstorage.a ${external_lib_dir}
    BUILD_ALWAYS False
)

ExternalProject_Add_StepDependencies(libpinyin configure kyotocabinet)

pkg_search_module(glib REQUIRED glib-2.0)
list(APPEND deps_libs ${glib_LIBRARIES})
list(APPEND includes ${glib_INCLUDE_DIRS})

list(APPEND includes ${external_include_dir}/libpinyin-${libpinyin_version})
list(APPEND deps_libs ${external_lib_dir}/libpinyin.a 
                      ${external_lib_dir}/liblookup.a
                      ${external_lib_dir}/libstorage.a
                      ${external_lib_dir}/libkyotocabinet.a)
list(APPEND deps libpinyin)

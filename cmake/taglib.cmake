set(taglib_source_url "https://taglib.org/releases/taglib-1.13.1.tar.gz")
set(taglib_checksum "c8da2b10f1bfec2cd7dbfcd33f4a2338db0765d851a50583d410bacf055cfd0b")

ExternalProject_Add(taglib
    SOURCE_DIR ${external_dir}/taglib
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/taglib
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${taglib_source_url}"
    URL_HASH SHA256=${taglib_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_TESTING=OFF -DBUILD_BINDINGS=OFF -DBUILD_SHARED_LIBS=OFF
        -DENABLE_STATIC_RUNTIME=OFF
    BUILD_ALWAYS False
)

list(APPEND deps_libs ${external_lib_dir}/libtag.a)
list(APPEND deps taglib)

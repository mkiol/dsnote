set(rubberband_source_url "https://breakfastquay.com/files/releases/rubberband-3.3.0.tar.bz2")
set(rubberband_checksum "d9ef89e2b8ef9f85b13ac3c2faec30e20acf2c9f3a9c8c45ce637f2bc95e576c")

if(${meson_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "meson not found but it is required to build rubberband")
endif()

ExternalProject_Add(rubberband
    SOURCE_DIR ${external_dir}/rubberband
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/rubberband
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${rubberband_source_url}
    URL_HASH SHA256=${rubberband_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND PKG_CONFIG_PATH="${external_lib_dir}/pkgconfig"
        ${meson_bin} setup --prefix=<INSTALL_DIR> --buildtype=release --libdir=lib
        -Dauto_features=disabled
        -Dresampler=builtin -Dfft=builtin
        -Dextra_include_dirs=${external_include_dir}
        -Dextra_lib_dirs=${external_lib_dir}
        <BINARY_DIR> <SOURCE_DIR>
    BUILD_COMMAND ninja -C <BINARY_DIR>
    BUILD_ALWAYS False
    INSTALL_COMMAND ninja -C <BINARY_DIR> install
)

list(APPEND deps_libs "${external_lib_dir}/librubberband.a")
list(APPEND deps rubberband)

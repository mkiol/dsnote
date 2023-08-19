set(rubberband_source_url "https://breakfastquay.com/files/releases/rubberband-3.3.0.tar.bz2")
set(rubberband_checksum "d9ef89e2b8ef9f85b13ac3c2faec30e20acf2c9f3a9c8c45ce637f2bc95e576c")

set(samplerate_source_url "https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz")
set(samplerate_checksum "3258da280511d24b49d6b08615bbe824d0cacc9842b0e4caf11c52cf2b043893")

set(sleef_source_url "https://github.com/shibatch/sleef/archive/85440a5e87dae36ca1b891de14bc83b441ae7c43.zip")
set(sleef_checksum "9760b476af68182c8ffa40fd1809ce3f2a044a5f3057ff4dfa8d71737d8ffe16")

message(STATUS PKG=$ENV{PKG_CONFIG_PATH})

ExternalProject_Add(samplerate
    SOURCE_DIR ${external_dir}/samplerate
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/samplerate
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${samplerate_source_url}
    URL_HASH SHA256=${samplerate_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_ALWAYS False
)

ExternalProject_Add(sleef
    SOURCE_DIR ${external_dir}/sleef
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/sleef
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${sleef_source_url}
    URL_HASH SHA256=${sleef_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DBUILD_SHARED_LIBS=OFF
        -DDISABLE_FFTW=ON -DDISABLE_MPFR=ON -DDISABLE_SSL=ON
        -DBUILD_TESTS=OFF -DBUILD_GNUABI_LIBS=OFF -DBUILD_DFT=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_ALWAYS False
)

ExternalProject_Add(rubberband
    SOURCE_DIR ${external_dir}/rubberband
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/rubberband
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${rubberband_source_url}
    URL_HASH SHA256=${rubberband_checksum}
    CONFIGURE_COMMAND PKG_CONFIG_PATH=${external_lib_dir}/pkgconfig
        meson setup --prefix=<INSTALL_DIR> --buildtype=release
        -Dauto_features=disabled -Ddefault_library=static
        -Dresampler=libsamplerate -Dfft=sleef
        -Dextra_include_dirs=${external_include_dir}
        -Dextra_lib_dirs=${external_lib_dir}
        <BINARY_DIR> <SOURCE_DIR>
    BUILD_COMMAND ninja -C <BINARY_DIR>
    BUILD_ALWAYS False
    INSTALL_COMMAND ninja -C <BINARY_DIR> install
)

ExternalProject_Add_StepDependencies(rubberband configure samplerate)
ExternalProject_Add_StepDependencies(rubberband configure sleef)

list(APPEND deps_libs
    "${external_lib_dir}/librubberband.a"
    "${external_lib_dir}/libsamplerate.a"
    "${external_lib_dir}/libsleefdft.a"
    "${external_lib_dir}/libsleef.a"
    )

list(APPEND deps samplerate sleef rubberband)

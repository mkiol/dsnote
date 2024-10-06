set(rnnoise_source_url "https://github.com/GregorR/rnnoise-nu/archive/26269304e120499485438cd93acf5127c6908c68.zip")
set(rnnoise_checksum "fafe9bbf0e2b15df4a628434bea99dd3")

if(${autoconf_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "autoconf not found but it is required to build rnnoise")
endif()
if(${automake_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "automake not found but it is required to build rnnoise")
endif()
if(${libtool_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "libtool not found but it is required to build rnnoise")
endif()

# adding prefix rnnoise_ to avoid symbol collision with libopus
set(rnnoise_cflags
    "-Dpitch_downsample=rnnoise_pitch_downsample \
    -Dpitch_search=rnnoise_pitch_search \
    -Dremove_doubling=rnnoise_remove_doubling \
    -D_celt_lpc=rnnoise__celt_lpc \
    -Dcelt_iir=rnnoise_celt_iir \
    -D_celt_autocorr=rnnoise__celt_autocorr \
    -Dcompute_gru=rnnoise_compute_gru \
    -Dcompute_dense=rnnoise_compute_dense")

ExternalProject_Add(rnnoise
    SOURCE_DIR ${external_dir}/rnnoise
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/rnnoise
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${rnnoise_source_url}"
    URL_MD5 "${rnnoise_checksum}"
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> && <BINARY_DIR>/autogen.sh &&
        <BINARY_DIR>/configure --prefix=<INSTALL_DIR> --libdir=<INSTALL_DIR>/lib
        --disable-examples --disable-doc --disable-shared --enable-static --with-pic
        CFLAGS=${rnnoise_cflags}
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

list(APPEND deps_libs "${external_lib_dir}/librnnoise-nu.a")

list(APPEND deps rnnoise)

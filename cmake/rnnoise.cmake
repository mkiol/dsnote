set(rnnoise_source_url "https://github.com/xiph/rnnoise/archive/70f1d256acd4b34a572f999a05c87bf00b67730d.zip")
set(rnnoise_checksum "a8aeeb024e08d5884b9b0b2cddb9b0c357c872a3c7149ca7a23375ccdd7e1190")

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
    -Dcompute_dense=rnnoise_compute_dense \
    -fpie")

if(arch_x8664)
    # enable avx/avx2
    set(rnnoise_opts --enable-x86-rtcd)
endif()

ExternalProject_Add(rnnoise
    SOURCE_DIR ${external_dir}/rnnoise
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/rnnoise
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${rnnoise_source_url}
    URL_HASH SHA256=${rnnoise_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR> -i ${patches_dir}/rnnoise.patch
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> && <BINARY_DIR>/autogen.sh &&
        <BINARY_DIR>/configure --prefix=<INSTALL_DIR> --libdir=<INSTALL_DIR>/lib
        --disable-examples --disable-doc --disable-shared --enable-static --with-pic
        CFLAGS=${rnnoise_cflags}
        ${rnnoise_opts}
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

list(APPEND deps_libs "${external_lib_dir}/librnnoise.a")

list(APPEND deps rnnoise)

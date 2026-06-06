set(mbrola_source_url "https://github.com/numediart/MBROLA/archive/bf17e9e1416a647979ac683657a536e8ca5d880e.zip")
set(mbrola_checksum "5a7c02a926dc48ab6d1af0e4c8ab53fc191a7e4337de6df57b6706e140fa3087")

set(espeak_source_url "https://github.com/espeak-ng/espeak-ng/archive/fbe4b3764285c35b1f035cb8d09ad9fc19f71c30.zip")
set(espeak_checksum "0879c9daaea2a3777e61a5e2c6c5f2e91b312ab29c562091c99bbfd82d261bee")
set(espeak_so_version "1.52.0.1")

if(${autoconf_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "autoconf not found but it is required to build espeak")
endif()
if(${automake_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "automake not found but it is required to build espeak")
endif()
if(${libtool_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "libtool not found but it is required to build espeak")
endif()

ExternalProject_Add(mbrola
    SOURCE_DIR ${external_dir}/mbrola
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/mbrola
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${mbrola_source_url}
    URL_HASH SHA256=${mbrola_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR>
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND mkdir -p ${external_bin_dir} && cp <BINARY_DIR>/Bin/mbrola ${external_bin_dir}
)

ExternalProject_Add(espeak
    SOURCE_DIR ${external_dir}/espeak
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/espeak
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${espeak_source_url}
    URL_HASH SHA256=${espeak_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/espeak.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DENABLE_TESTS=OFF
        -DBUILD_SHARED_LIBS=ON
        -DBUILD_STATIC_LIBS=ON
        -DUSE_MBROLA=ON
        -DUSE_LIBSONIC=OFF
        -DUSE_LIBPCAUDIO=OFF
        -DUSE_SPEECHPLAYER=OFF
        -DESPEAK_BUILD_MANPAGES=OFF
    BUILD_ALWAYS False
)

ExternalProject_Add_StepDependencies(espeak configure mbrola)

list(APPEND deps_libs
        "${external_lib_dir}/libespeak-ng.a"
        "${external_lib_dir}/libucd.a")
list(APPEND deps espeak mbrola)

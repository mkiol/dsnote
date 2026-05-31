set(openblas_source_url "https://github.com/OpenMathLib/OpenBLAS/releases/download/v0.3.33/OpenBLAS-0.3.33.tar.gz")
set(openblas_checksum "6761af1d9f5d353ab4f0b7497be2643313b36c8f31caec0144bfef198e71e6ab")

set(openblas_opts
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/external
    -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
    -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir}
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DBUILD_TESTING=OFF
    -DBUILD_WITHOUT_LAPACK=OFF
    -DC_LAPACK=ON
    -DDYNAMIC_ARCH=ON
    -DBUILD_STATIC_LIBS=OFF
    -DBUILD_SHARED_LIBS=ON
    -DCMAKE_C_FLAGS=-Wno-error=incompatible-pointer-types -Wno-unused-variable -Wno-unused-but-set-variable
)

if(arch_x8664)
    list(APPEND openblas_opts -DTARGET=CORE2)
elseif(arch_arm32)
    list(APPEND openblas_opts -DTARGET=ARMV7)
elseif(arch_arm64)
    list(APPEND openblas_opts -DTARGET=ARMV8)
    list(APPEND openblas_opts "-DDYNAMIC_LIST:string=CORTEXA53\\\\\\\\\\\\;CORTEXA57")
    message(STATUS ${openblas_opts})
endif()

ExternalProject_Add(openblas
    SOURCE_DIR ${external_dir}/openblas
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/openblas
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${openblas_source_url}
    URL_HASH SHA256=${openblas_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS ${openblas_opts}
    BUILD_ALWAYS False
)

list(APPEND deps openblas)

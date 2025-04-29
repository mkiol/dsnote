pkg_search_module(openblas openblas)

if(NOT DEFINED ${openblas_FOUND})
    # check without pkg-config
    set(BLA_STATIC OFF)
    set(BLA_VENDOR "OpenBLAS")
    find_package(BLAS REQUIRED)

    message(STATUS "can't find openblas using pkg-config, but found openblas package: ${BLAS_LIBRARIES} ${BALS_VERSION}")

    cmake_path(GET BLAS_LIBRARIES PARENT_PATH openblas_lib_dir)
    message(STATUS "openblas lib dir=${openblas_lib_dir}")

    # Fedora doesn't provide pkg-config for openblas, so creating one
    file(WRITE "${PROJECT_BINARY_DIR}/openblas.pc" "libdir=${openblas_lib_dir}
libnameprefix=
libnamesuffix=
libsuffix=
includedir=/usr/include/openblas

openblas_config=USE_64BITINT= NO_CBLAS= NO_LAPACK= NO_LAPACKE= DYNAMIC_ARCH=ON DYNAMIC_OLDER=OFF NO_AFFINITY=ON USE_OPENMP=1 CORE2 MAX_THREADS=64
Name: OpenBLAS
Description: OpenBLAS
Version: ${BALS_VERSION}
URL: https://github.com/OpenMathLib/OpenBLAS
Libs: -fopenmp -L\${libdir\} -l\${libnameprefix}openblas\${libnamesuffix}\${libsuffix}
Cflags: -I\${includedir}
")
    # search again
    pkg_search_module(openblas REQUIRED openblas)
endif()

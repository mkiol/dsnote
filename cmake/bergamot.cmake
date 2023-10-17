set(bergamot_source_url "https://github.com/browsermt/bergamot-translator.git")
set(bergamot_tag "ada8c3922490cc6a507bcf81fa4882b435595323")

if(arch_x8664)
    set(bergamot_build_arch x86-64)
elseif(arch_arm64)
    set(bergamot_build_arch armv8-a)
elseif(arch_arm32)
    set(bergamot_build_arch armv7-a)
endif()

if(BUILD_OPENBLAS)
    set(blas_lib_path ${external_lib_dir}/libopenblas.so)
    set(blas_include_dir ${external_include_dir}/openblas)
else()
    set(BLA_STATIC OFF)
    set(BLA_VENDOR "OpenBLAS")
    find_package(BLAS REQUIRED)

    find_path(BLAS_INCLUDE_DIRS NAMES cblas.h
        PATHS ${CMAKE_PREFIX_PATH}/include/openblas /usr/include/openblas /usr/local/include/openblas $ENV{BLAS_HOME}/include
        REQUIRED NO_DEFAULT_PATH)

    set(blas_lib_path ${BLAS_LIBRARIES})
    set(blas_include_dir ${BLAS_INCLUDE_DIRS})
endif()

if(arch_x8664)
    ExternalProject_Add(bergamotfallback
        SOURCE_DIR ${external_dir}/bergamotfallback
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/bergamotfallback
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        GIT_REPOSITORY "${bergamot_source_url}"
        GIT_TAG ${bergamot_tag}
        GIT_SHALLOW OFF
        UPDATE_COMMAND ""
        PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                    -i ${patches_dir}/bergamot.patch ||
                        echo "patch cmd failed, likely already patched"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DBLAS_LIB_PATH=${blas_lib_path}
            -DBLAS_INC_DIR=${blas_include_dir}
            -DUSE_INTRINSICS_SSE2=ON
            -DUSE_INTRINSICS_SSE3=OFF
            -DUSE_INTRINSICS_SSE41=OFF
            -DUSE_INTRINSICS_SSE42=OFF
            -DUSE_INTRINSICS_AVX=ON
            -DUSE_INTRINSICS_AVX2=OFF
            -DUSE_INTRINSICS_AVX512=OFF
            -DUSE_INTRINSICS_FMA=OFF
            -DUSE_INTRINSICS_ARMV7_NEONVFPV4=OFF
            -DUSE_INTRINSICS_ARMV7_NEON=ON
            -DBUILD_ARCH=${bergamot_build_arch}
        INSTALL_COMMAND cp libbergamot_api.so ${external_lib_dir}/libbergamot_api-fallback.so
        BUILD_ALWAYS False
    )

    if(BUILD_OPENBLAS)
        ExternalProject_Add_StepDependencies(bergamotfallback configure openblas)
    endif()

    list(APPEND deps bergamotfallback)
endif(arch_x8664)

ExternalProject_Add(bergamot
    SOURCE_DIR ${external_dir}/bergamot
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/bergamot
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    GIT_REPOSITORY "${bergamot_source_url}"
    GIT_TAG ${bergamot_tag}
    GIT_SHALLOW OFF
    UPDATE_COMMAND ""
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/bergamot.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_VERBOSE_MAKEFILE=ON
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBLAS_LIB_PATH=${blas_lib_path}
        -DBLAS_INC_DIR=${blas_include_dir}
        -DUSE_INTRINSICS_SSE2=ON
        -DUSE_INTRINSICS_SSE3=ON
        -DUSE_INTRINSICS_SSE41=ON
        -DUSE_INTRINSICS_SSE42=ON
        -DUSE_INTRINSICS_AVX=ON
        -DUSE_INTRINSICS_AVX2=ON
        -DUSE_INTRINSICS_AVX512=OFF
        -DUSE_INTRINSICS_FMA=ON
        -DUSE_INTRINSICS_ARMV7_NEON=OFF
        -DUSE_INTRINSICS_ARMV7_NEONVFPV4=ON
        -DBUILD_ARCH=${bergamot_build_arch}
    BUILD_ALWAYS False
)

if(BUILD_OPENBLAS)
    ExternalProject_Add_StepDependencies(bergamot configure openblas)
endif()

list(APPEND deps bergamot)

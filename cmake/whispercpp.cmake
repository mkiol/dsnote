set(whispercpp_source_url "https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v1.4.3.tar.gz")
set(whispercpp_checksum "5f11c0542639bfb0b3c9d1b033d10ccd69ca26e739aec9366766617bc58a6e7c")

set(whispercpp_flags -O3 -ffast-math -I${external_include_dir}/openblas)
set(whispercppfallback_flags ${whispercpp_flags})
if(arch_arm32)
    list(APPEND whispercpp_flags -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access)
    list(APPEND whispercppfallback_flags -mfp16-format=ieee -mno-unaligned-access)
endif()
list(JOIN whispercpp_flags " " whispercpp_flags)
list(JOIN whispercppfallback_flags " " whispercppfallback_flags)

if(arch_x8664)
    if(BUILD_WHISPERCPP_CLBLAST)
        set(opencl_source_url "https://github.com/KhronosGroup/OpenCL-Headers.git")
        set(opencl_tag "4fdcfb0ae675f2f63a9add9552e0af62c2b4ed30")

        set(clblast_source_url "https://github.com/CNugteren/CLBlast.git")
        set(clblast_tag "e3ce21bb937f07b8282dccf4823e2acbdf286d17")

        ExternalProject_Add(opencl
            SOURCE_DIR ${external_dir}/opencl
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/opencl
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            GIT_REPOSITORY "${opencl_source_url}"
            GIT_TAG ${opencl_tag}
            UPDATE_COMMAND ""
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            BUILD_ALWAYS False
        )

        ExternalProject_Add(clblast
            SOURCE_DIR ${external_dir}/clblast
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/clblast
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            GIT_REPOSITORY "${clblast_source_url}"
            GIT_TAG ${clblast_tag}
            UPDATE_COMMAND ""
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                -DTUNERS=OFF
            BUILD_ALWAYS False
        )

        ExternalProject_Add(whispercppclblast
            SOURCE_DIR ${external_dir}/whispercppclblast
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercppclblast
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${whispercpp_source_url}"
            URL_HASH SHA256=${whispercpp_checksum}
            PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                        -i ${patches_dir}/whispercpp.patch ||
                            echo "patch cmd failed, likely already patched"
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DWHISPER_CLBLAST=ON
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
            INSTALL_COMMAND make install && cp ${external_lib_dir}/libwhisper.so ${external_lib_dir}/libwhisper-clblast.so
            BUILD_ALWAYS False
        )

        ExternalProject_Add_StepDependencies(clblast configure opencl)
        ExternalProject_Add_StepDependencies(whispercppclblast configure opencl)
        ExternalProject_Add_StepDependencies(whispercppclblast configure clblast)
        ExternalProject_Add_StepDependencies(whispercppclblast configure whispercppopenblas)

        list(APPEND deps whispercppclblast)
    endif(BUILD_WHISPERCPP_CLBLAST)

    if(BUILD_WHISPERCPP_CUBLAS)
        if (NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
            set(CMAKE_CUDA_ARCHITECTURES "50\\\\\\\\\\\\;52\\\\\\\\\\\\;53\\\\\\\\\\\\;60\\\\\\\\\\\\;61\\\\\\\\\\\\;62\\\\\\\\\\\\;70\\\\\\\\\\\\;72\\\\\\\\\\\\;75\\\\\\\\\\\\;80\\\\\\\\\\\\;86\\\\\\\\\\\\;87\\\\\\\\\\\\;89\\\\\\\\\\\\;90")
        endif()

        ExternalProject_Add(whispercppcublas
            SOURCE_DIR ${external_dir}/whispercppcublas
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercppcublas
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${whispercpp_source_url}"
            URL_HASH SHA256=${whispercpp_checksum}
            PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                        -i ${patches_dir}/whispercpp.patch ||
                            echo "patch cmd failed, likely already patched"
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DWHISPER_CUBLAS=ON
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DCMAKE_CUDA_ARCHITECTURES=${CMAKE_CUDA_ARCHITECTURES}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
            INSTALL_COMMAND make install && cp ${external_lib_dir}/libwhisper.so ${external_lib_dir}/libwhisper-cublas.so
            BUILD_ALWAYS False
        )

        if(BUILD_WHISPERCPP_CLBLAST)
            ExternalProject_Add_StepDependencies(whispercppclblast configure whispercppclblast)
        else()
            ExternalProject_Add_StepDependencies(whispercppclblast configure whispercppopenblas)
        endif()

        list(APPEND deps whispercppcublas)
    endif(BUILD_WHISPERCPP_CUBLAS)

    if(BUILD_WHISPERCPP_HIPBLAS)
        if (NOT DEFINED CMAKE_HIP_ARCHITECTURES)
            set(CMAKE_HIP_ARCHITECTURES "gfx701 gfx801 gfx802 gfx803 gfx900 gfx906 gfx908 gfx1010 gfx1011 gfx1012 gfx1030 gfx1031")
        endif()

        ExternalProject_Add(whispercpphipblas
            SOURCE_DIR ${external_dir}/whispercpphipblas
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercpphipblas
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${whispercpp_source_url}"
            URL_HASH SHA256=${whispercpp_checksum}
            PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                        -i ${patches_dir}/whispercpp.patch ||
                            echo "patch cmd failed, likely already patched"
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DWHISPER_HIPBLAS=ON
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DCMAKE_HIP_ARCHITECTURES=${CMAKE_HIP_ARCHITECTURES}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
            INSTALL_COMMAND make install && cp ${external_lib_dir}/libwhisper.so ${external_lib_dir}/libwhisper-hipblas.so
            BUILD_ALWAYS False
        )

        if(BUILD_WHISPERCPP_CUBLAS)
            ExternalProject_Add_StepDependencies(whispercppclblast configure whispercppcublas)
        elseif(BUILD_WHISPERCPP_CLBLAST)
            ExternalProject_Add_StepDependencies(whispercppclblast configure whispercppclblast)
        else()
            ExternalProject_Add_StepDependencies(whispercppclblast configure whispercppopenblas)
        endif()

        list(APPEND deps whispercpphipblas)
    endif(BUILD_WHISPERCPP_HIPBLAS)
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

message(STATUS "OpenBLAS lib: ${blas_lib_path}")
message(STATUS "OpenBLAS include: ${blas_include_dir}")

ExternalProject_Add(whispercppfallback
    SOURCE_DIR ${external_dir}/whispercppfallback
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercppfallback
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${whispercpp_source_url}"
    URL_HASH SHA256=${whispercpp_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/whispercpp.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DBLAS_LIB_PATH=${blas_lib_path}
        -DBLAS_INC_DIR=${blas_include_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DWHISPER_NO_AVX=ON -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON -DWHISPER_NO_F16C=ON
        -DCMAKE_C_FLAGS=${whispercppfallback_flags} -DCMAKE_CXX_FLAGS=${whispercppfallback_flags}
        -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
    INSTALL_COMMAND make install && cp ${external_lib_dir}/libwhisper.so ${external_lib_dir}/libwhisper-fallback.so
    BUILD_ALWAYS False
)

ExternalProject_Add(whispercppopenblas
    SOURCE_DIR ${external_dir}/whispercppopenblas
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercppopenblas
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${whispercpp_source_url}"
    URL_HASH SHA256=${whispercpp_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/whispercpp.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DBLAS_LIB_PATH=${blas_lib_path}
        -DBLAS_INC_DIR=${blas_include_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
        -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
    INSTALL_COMMAND make install && cp ${external_lib_dir}/libwhisper.so ${external_lib_dir}/libwhisper-openblas.so
    BUILD_ALWAYS False
)

ExternalProject_Add_StepDependencies(whispercppopenblas configure whispercppfallback)

if(BUILD_OPENBLAS)
    ExternalProject_Add_StepDependencies(whispercppfallback configure openblas)
    ExternalProject_Add_StepDependencies(whispercppopenblas configure openblas)
endif()

list(APPEND deps whispercppopenblas whispercppfallback)

set(whispercpp_source_url "https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v1.6.2.tar.gz")
set(whispercpp_checksum "da7988072022acc3cfa61b370b3c51baad017f1900c3dc4e68cb276499f66894")

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
        set(clblast_source_url "https://github.com/CNugteren/CLBlast.git")
        set(clblast_tag "e3ce21bb937f07b8282dccf4823e2acbdf286d17")

        find_package(OpenCL)
        if(NOT ${OpenCL_FOUND})
           message(FATAL_ERROR "OpenCL not found but it is required by whisper.cpp-clblast")
        endif()

        ExternalProject_Add(clblast
            SOURCE_DIR ${external_dir}/clblast
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/clblast
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            GIT_REPOSITORY "${clblast_source_url}"
            GIT_TAG ${clblast_tag}
            UPDATE_COMMAND ""
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                -DCMAKE_INSTALL_LIBDIR=lib
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
                -DCMAKE_INSTALL_LIBDIR=lib
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DWHISPER_CLBLAST=ON
                -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-clblast
            BUILD_ALWAYS False
        )

        ExternalProject_Add_StepDependencies(whispercppclblast configure clblast)

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
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_INSTALL_LIBDIR=lib
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DWHISPER_CUDA=ON
                -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DGGML_CUDA_ARCHITECTURES=${CMAKE_CUDA_ARCHITECTURES}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-cublas
            BUILD_ALWAYS False
        )

        list(APPEND deps whispercppcublas)
    endif(BUILD_WHISPERCPP_CUBLAS)

    if(BUILD_WHISPERCPP_HIPBLAS)
        if (NOT DEFINED CMAKE_HIP_ARCHITECTURES)
            set(CMAKE_HIP_ARCHITECTURES "gfx801 gfx802 gfx803 gfx805 gfx810 gfx900 gfx902 gfx904 gfx906 gfx908 gfx909 gfx90a gfx90c gfx940 gfx1010 gfx1011 gfx1012 gfx1013 gfx1030 gfx1031 gfx1032 gfx1033 gfx1034 gfx1035 gfx1036 gfx1100 gfx1101 gfx1102 gfx1103")
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
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_INSTALL_LIBDIR=lib
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DWHISPER_HIPBLAS=ON
                -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DGGML_ROCM_ARCHITECTURES=${CMAKE_HIP_ARCHITECTURES}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-hipblas
            BUILD_ALWAYS False
        )

        list(APPEND deps whispercpphipblas)
    endif(BUILD_WHISPERCPP_HIPBLAS)

    if(BUILD_WHISPERCPP_OPENVINO)
        ExternalProject_Add(whispercppopenvino
            SOURCE_DIR ${external_dir}/whispercppopenvino
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercppopenvino
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${whispercpp_source_url}"
            URL_HASH SHA256=${whispercpp_checksum}
            PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                        -i ${patches_dir}/whispercpp.patch ||
                            echo "patch cmd failed, likely already patched"
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_INSTALL_LIBDIR=lib
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DWHISPER_OPENVINO=ON
                -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-openvino
            BUILD_ALWAYS False
        )

        list(APPEND deps whispercppopenvino)
    endif(BUILD_WHISPERCPP_OPENVINO)
endif()

if(BUILD_OPENBLAS)
    set(blas_lib_path ${external_lib_dir}/libopenblas.so)
    set(blas_include_dir ${external_include_dir}/openblas)
else()
    set(BLA_STATIC OFF)
    set(BLA_VENDOR "OpenBLAS")
    find_package(BLAS REQUIRED)

    find_path(BLAS_INCLUDE_DIRS NAMES cblas.h
        PATHS ${CMAKE_PREFIX_PATH}/include/openblas /usr/include/openblas
        /usr/local/include/openblas $ENV{BLAS_HOME}/include REQUIRED)

    set(blas_lib_path ${BLAS_LIBRARIES})
    set(blas_include_dir ${BLAS_INCLUDE_DIRS})
endif()

message(STATUS "OpenBLAS lib: ${blas_lib_path}")
message(STATUS "OpenBLAS include: ${blas_include_dir}")

ExternalProject_Add(whispercppfallback1
    SOURCE_DIR ${external_dir}/whispercppfallback1
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercppfallback1
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${whispercpp_source_url}"
    URL_HASH SHA256=${whispercpp_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/whispercpp.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
        -DBLAS_LIB_PATH=${blas_lib_path}
        -DBLAS_INC_DIR=${blas_include_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON
        -DCMAKE_C_FLAGS=${whispercppfallback_flags} -DCMAKE_CXX_FLAGS=${whispercppfallback_flags}
        -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
        -DWHISPER_TARGET_NAME=whisper-fallback1
    BUILD_ALWAYS False
)

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
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
        -DBLAS_LIB_PATH=${blas_lib_path}
        -DBLAS_INC_DIR=${blas_include_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DWHISPER_NO_AVX=ON -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON -DWHISPER_NO_F16C=ON
        -DCMAKE_C_FLAGS=${whispercppfallback_flags} -DCMAKE_CXX_FLAGS=${whispercppfallback_flags}
        -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
        -DWHISPER_TARGET_NAME=whisper-fallback
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
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
        -DBLAS_LIB_PATH=${blas_lib_path}
        -DBLAS_INC_DIR=${blas_include_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
        -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
        -DWHISPER_TARGET_NAME=whisper-openblas
    BUILD_ALWAYS False
)

if(BUILD_OPENBLAS)
    ExternalProject_Add_StepDependencies(whispercppfallback configure openblas)
    ExternalProject_Add_StepDependencies(whispercppfallback1 configure openblas)
    ExternalProject_Add_StepDependencies(whispercppopenblas configure openblas)
endif()

list(APPEND deps whispercppopenblas whispercppfallback1)
list(APPEND deps whispercppopenblas whispercppfallback)

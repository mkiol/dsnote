set(whispercpp_source_url "https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v1.7.1.tar.gz")
set(whispercpp_checksum "97f19a32212f2f215e538ee37a16ff547aaebc54817bd8072034e02466ce6d55")

set(whispercpp_flags -O3 -ffast-math -fno-finite-math-only -I${external_include_dir}/openblas)
set(whispercppfallback_flags ${whispercpp_flags})
if(arch_arm32)
    list(APPEND whispercpp_flags -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access)
    list(APPEND whispercppfallback_flags -mfp16-format=ieee -mno-unaligned-access)
endif()
list(JOIN whispercpp_flags " " whispercpp_flags)
list(JOIN whispercppfallback_flags " " whispercppfallback_flags)

if(arch_x8664)
    if(BUILD_WHISPERCPP_CLBLAST)
        # Using older whisper.cpp version because the latest one doesn't support OpenCL
        set(whispercpp_clblast_source_url "https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v1.6.2.tar.gz")
        set(whispercpp_clblast_checksum "da7988072022acc3cfa61b370b3c51baad017f1900c3dc4e68cb276499f66894")

        set(clblast_source_url "https://github.com/CNugteren/CLBlast/archive/refs/tags/1.6.3.tar.gz")
        set(clblast_checksum "c05668c7461e8440fce48c9f7a8966a6f9e0923421acd7c0357ece9b1d83f20e")

        find_package(OpenCL)
        if(NOT ${OpenCL_FOUND})
           message(FATAL_ERROR "OpenCL not found but it is required by whisper.cpp-clblast")
        endif()

        ExternalProject_Add(clblast
            SOURCE_DIR ${external_dir}/clblast
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/clblast
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${clblast_source_url}"
            URL_HASH SHA256=${clblast_checksum}
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
            URL "${whispercpp_clblast_source_url}"
            URL_HASH SHA256=${whispercpp_clblast_checksum}
            PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                        -i ${patches_dir}/whispercpp-clblast.patch ||
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

        if(BUILD_WHISPERCPP_VULKAN)
            ExternalProject_Add_StepDependencies(whispercppclblast install whispercppvulkan)
        else()
            ExternalProject_Add_StepDependencies(whispercppclblast install whispercppfallback)
        endif()

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
                -DGGML_NATIVE=OFF
                -DGGML_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=${CMAKE_CUDA_ARCHITECTURES}
                -DGGML_AVX=ON -DGGML_AVX2=OFF -DGGML_FMA=OFF -DGGML_F16C=ON
                -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-cublas
            BUILD_ALWAYS False
        )

        if(BUILD_WHISPERCPP_CLBLAST)
            ExternalProject_Add_StepDependencies(whispercppcublas install whispercppclblast)
        else()
            if(BUILD_WHISPERCPP_VULKAN)
                ExternalProject_Add_StepDependencies(whispercppcublas install whispercppvulkan)
            else()
                ExternalProject_Add_StepDependencies(whispercppcublas install whispercppfallback)
            endif()
        endif()

        list(APPEND deps whispercppcublas)
    endif(BUILD_WHISPERCPP_CUBLAS)

    if(BUILD_WHISPERCPP_HIPBLAS)
        if (NOT DEFINED CMAKE_HIP_ARCHITECTURES)
            set(CMAKE_HIP_ARCHITECTURES "gfx801\\\\\\\\\\\\;gfx802\\\\\\\\\\\\;gfx803\\\\\\\\\\\\;gfx805\\\\\\\\\\\\;gfx810\\\\\\\\\\\\;gfx900\\\\\\\\\\\\;gfx902\\\\\\\\\\\\;gfx904\\\\\\\\\\\\;gfx906\\\\\\\\\\\\;gfx908\\\\\\\\\\\\;gfx909\\\\\\\\\\\\;gfx90a\\\\\\\\\\\\;gfx90c\\\\\\\\\\\\;gfx940\\\\\\\\\\\\;gfx1010\\\\\\\\\\\\;gfx1011\\\\\\\\\\\\;gfx1012\\\\\\\\\\\\;gfx1013\\\\\\\\\\\\;gfx1030\\\\\\\\\\\\;gfx1031\\\\\\\\\\\\;gfx1032\\\\\\\\\\\\;gfx1033\\\\\\\\\\\\;gfx1034\\\\\\\\\\\\;gfx1035\\\\\\\\\\\\;gfx1036\\\\\\\\\\\\;gfx1100\\\\\\\\\\\\;gfx1101\\\\\\\\\\\\;gfx1102\\\\\\\\\\\\;gfx1103")
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
                -DGGML_NATIVE=OFF
                -DGGML_HIPBLAS=ON -DCMAKE_HIP_ARCHITECTURES=${CMAKE_HIP_ARCHITECTURES}
                -DGGML_AVX=ON -DGGML_AVX2=OFF -DGGML_FMA=OFF -DGGML_F16C=ON
                -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-hipblas
            BUILD_ALWAYS False
        )

        if(BUILD_WHISPERCPP_CUBLAS)
            ExternalProject_Add_StepDependencies(whispercpphipblas install whispercppcublas)
        else()
            if(BUILD_WHISPERCPP_CLBLAST)
                ExternalProject_Add_StepDependencies(whispercpphipblas install whispercppclblast)
            else()
                if(BUILD_WHISPERCPP_VULKAN)
                    ExternalProject_Add_StepDependencies(whispercpphipblas install whispercppvulkan)
                else()
                    ExternalProject_Add_StepDependencies(whispercpphipblas install whispercppfallback)
                endif()
            endif()
        endif()

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
                -DGGML_NATIVE=OFF
                -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS
                -DGGML_AVX=ON -DGGML_AVX2=OFF -DGGML_FMA=OFF -DGGML_F16C=ON
                -DWHISPER_OPENVINO=ON
                -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DCMAKE_C_FLAGS=${whispercppfallback_flags} -DCMAKE_CXX_FLAGS=${whispercppfallback_flags}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-openvino
            BUILD_ALWAYS False
        )

        if(BUILD_WHISPERCPP_HIPBLAS)
            ExternalProject_Add_StepDependencies(whispercppopenvino install whispercpphipblas)
        else()
            if(BUILD_WHISPERCPP_CUBLAS)
                ExternalProject_Add_StepDependencies(whispercppopenvino install whispercppcublas)
            else()
                if(BUILD_WHISPERCPP_CLBLAST)
                    ExternalProject_Add_StepDependencies(whispercppopenvino install whispercppclblast)
                else()
                    if(BUILD_WHISPERCPP_VULKAN)
                        ExternalProject_Add_StepDependencies(whispercppopenvino install whispercppvulkan)
                    else()
                        ExternalProject_Add_StepDependencies(whispercppopenvino install whispercppfallback)
                    endif()
                endif()
            endif()
        endif()

        list(APPEND deps whispercppopenvino)
    endif(BUILD_WHISPERCPP_OPENVINO)

    if(BUILD_WHISPERCPP_VULKAN)
        find_package(Vulkan)
        if(NOT ${Vulkan_FOUND})
           message(FATAL_ERROR "Vulkan not found but it is required by whisper.cpp-vulkan")
        endif()

        ExternalProject_Add(whispercppvulkan
            SOURCE_DIR ${external_dir}/whispercppvulkan
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercppvulkan
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${whispercpp_source_url}"
            URL_HASH SHA256=${whispercpp_checksum}
            PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                        -i ${patches_dir}/whispercpp.patch ||
                            echo "patch cmd failed, likely already patched"
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_INSTALL_LIBDIR=lib
                -DGGML_NATIVE=OFF
                -DGGML_VULKAN=ON
                -DGGML_AVX=ON -DGGML_AVX2=OFF -DGGML_FMA=OFF -DGGML_F16C=ON
                -DBUILD_SHARED_LIBS=ON
                -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
                -DCMAKE_C_FLAGS=${whispercppfallback_flags} -DCMAKE_CXX_FLAGS=${whispercppfallback_flags}
                -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
                -DWHISPER_TARGET_NAME=whisper-vulkan
            BUILD_ALWAYS False
        )

        ExternalProject_Add_StepDependencies(whispercppvulkan install whispercppfallback)

        list(APPEND deps whispercppvulkan)
    endif(BUILD_WHISPERCPP_VULKAN)
endif()

if(NOT ${BUILD_OPENBLAS})
    set(BLA_STATIC OFF)
    set(BLA_VENDOR "OpenBLAS")
    find_package(BLAS REQUIRED)
endif()

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
        -DGGML_NATIVE=OFF
        -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS
        -DGGML_AVX=ON -DGGML_AVX2=OFF -DGGML_FMA=OFF -DGGML_F16C=ON
        -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
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
        -DGGML_NATIVE=OFF
        -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS
        -DGGML_AVX=OFF -DGGML_AVX2=OFF -DGGML_FMA=OFF -DGGML_F16C=OFF
        -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
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
        -DGGML_NATIVE=OFF
        -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS
        -DGGML_AVX=ON -DGGML_AVX2=ON -DGGML_FMA=ON -DGGML_F16C=ON
        -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
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

# make sequential rather than parallel installing of different types of whisper.cpp
# order: whispercppopenblas => whispercppfallback1 => whispercppfallback => whispercppvulkan
#        => whispercppclblast => whispercppcublas => whispercpphipblas => whispercppopenvino
ExternalProject_Add_StepDependencies(whispercppfallback install whispercppfallback1)
ExternalProject_Add_StepDependencies(whispercppfallback1 install whispercppopenblas)

list(APPEND deps whispercppopenblas whispercppfallback1)
list(APPEND deps whispercppopenblas whispercppfallback)

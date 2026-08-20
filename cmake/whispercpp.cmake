set(whispercpp_source_url "https://github.com/ggml-org/whisper.cpp/archive/refs/tags/v1.9.1.tar.gz")
set(whispercpp_checksum "147267177eef7b22ec3d2476dd514d1b12e160e176230b740e3d1bd600118447")

set(whispercpp_flags
    -ffast-math
    -fno-finite-math-only 
    -I${external_include_dir}
    -I${external_include_dir}/openblas)
list(JOIN whispercpp_flags " " whispercpp_flags)

if(NOT ${BUILD_OPENBLAS})
    set(BLA_STATIC OFF)
    set(BLA_VENDOR "OpenBLAS")
    find_package(BLAS REQUIRED)

    cmake_path(GET BLAS_LIBRARIES PARENT_PATH openblas_lib_dir)
    set(whispercpp_additional_lib_path "${openblas_lib_dir}")
endif()

if(BUILD_WHISPERCPP_OPENCL)
    set(clblast_source_url "https://github.com/CNugteren/CLBlast/archive/refs/tags/1.6.3.tar.gz")
    set(clblast_checksum "c05668c7461e8440fce48c9f7a8966a6f9e0923421acd7c0357ece9b1d83f20e")

    find_package(OpenCL REQUIRED)

    ExternalProject_Add(clblast
        SOURCE_DIR ${external_dir}/clblast
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/clblast
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${clblast_source_url}"
        URL_HASH SHA256=${clblast_checksum}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_INSTALL_LIBDIR=lib
            -DTUNERS=OFF
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        BUILD_ALWAYS False
    )
endif()

if(BUILD_WHISPERCPP_CUDA)
    find_package(CUDA REQUIRED)

    message(STATUS "Detected CUDA version: ${CUDA_VERSION}")

    if (NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        if(CUDA_VERSION VERSION_GREATER_EQUAL 12.9)
            set(CMAKE_CUDA_ARCHITECTURES "75;80;86;89;90;100;103")
        elseif(CUDA_VERSION VERSION_GREATER_EQUAL 12.0)
            set(CMAKE_CUDA_ARCHITECTURES "75;80;86;89;90")
        elseif(CUDA_VERSION VERSION_GREATER_EQUAL 11.0)
            set(CMAKE_CUDA_ARCHITECTURES "60;61;70;75;80;86")
        else()
            set(CMAKE_CUDA_ARCHITECTURES "60;61;70;75")
        endif()
    endif()

    list(JOIN CMAKE_CUDA_ARCHITECTURES "\\\\\\\\\\;" CMAKE_CUDA_ARCHITECTURES_STRING)
endif()

if(BUILD_WHISPERCPP_HIP)
    find_package(HIP REQUIRED)

    message(STATUS "Detected HIP version: ${HIP_VERSION}")

    find_package(hipblas REQUIRED)
    find_package(rocblas REQUIRED)

    if (NOT DEFINED CMAKE_HIP_ARCHITECTURES)
        if(HIP_VERSION VERSION_LESS 6.0)
            set(CMAKE_HIP_ARCHITECTURES "gfx900;gfx906;gfx908;gfx1010;gfx1030;gfx1100")
        elseif(HIP_VERSION VERSION_LESS 7.0)
            set(CMAKE_HIP_ARCHITECTURES "gfx900;gfx906;gfx908;gfx1010;gfx1030;gfx1100;gfx1101;gfx1102")
        else()
            set(CMAKE_HIP_ARCHITECTURES "gfx900;gfx906;gfx908;gfx1010;gfx1030;gfx1100;gfx1101;gfx1102;gfx1200;gfx1201")
        endif()
    endif()
    
    list(JOIN CMAKE_HIP_ARCHITECTURES "\\\\\\\\\\;" CMAKE_HIP_ARCHITECTURES_STRING)
endif()

if(BUILD_WHISPERCPP_VULKAN)
    set(spirvheaders_source_url "https://github.com/KhronosGroup/SPIRV-Headers/archive/refs/tags/vulkan-sdk-1.4.350.1.tar.gz")
    set(spirvheaders_checksum "9e6d5c78878172d2b810e97f3a74ecbbb14b4ad52b07384ce915fbbeb226d610")

    find_package(Vulkan REQUIRED)

    ExternalProject_Add(spirvheaders
        SOURCE_DIR ${external_dir}/spirvheaders
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/spirvheaders
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${spirvheaders_source_url}"
        URL_HASH SHA256=${spirvheaders_checksum}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DSPIRV_HEADERS_ENABLE_TESTS=OFF
            -DSPIRV_HEADERS_ENABLE_INSTALL=ON
        BUILD_ALWAYS False
    )

    if(BUILD_WHISPERCPP_VULKAN_SHADERC)
        set(glslang_source_url "https://github.com/KhronosGroup/glslang/archive/refs/tags/16.3.0.tar.gz")
        set(glslang_checksum "efff5a15258dce1ca2d323bf64c974f5fca03778174615dbc30c8d36db645bf5")

        set(spirvtools_source_url "https://github.com/KhronosGroup/SPIRV-Tools/archive/refs/tags/v2026.2.tar.gz")
        set(spirvtools_checksum "b4390652e64ab188eede9d556cf183b3c1c4be44d4fcd104b0d4af3bea2712d1")

        set(shaderc_source_url "https://github.com/google/shaderc/archive/refs/tags/v2026.3.tar.gz")
        set(shaderc_checksum "ee493ccf1b3038b4ef2fe024664c5eb2dc4bcc1f6b05b33e3909de0e19c81024")

        ExternalProject_Add(glslang
            SOURCE_DIR ${external_dir}/glslang
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/glslang
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${glslang_source_url}"
            URL_HASH SHA256=${glslang_checksum}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            UPDATE_COMMAND ""
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        ExternalProject_Add(spirvtools
            SOURCE_DIR ${external_dir}/spirvtools
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/spirvtools
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${spirvtools_source_url}"
            URL_HASH SHA256=${spirvtools_checksum}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            UPDATE_COMMAND ""
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        ExternalProject_Add(shaderc
            SOURCE_DIR ${external_dir}/shaderc
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/shaderc
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${shaderc_source_url}"
            URL_HASH SHA256=${shaderc_checksum}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            UPDATE_COMMAND ""
            CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DCMAKE_INSTALL_LIBDIR=lib
                -DSHADERC_SKIP_INSTALL=OFF
                -DSHADERC_SKIP_TESTS=ON
                -DSHADERC_SKIP_EXAMPLES=ON
                -DSHADERC_SKIP_EXECUTABLES=OFF
                -DSHADERC_SKIP_COPYRIGHT_CHECK=ON
                -DSHADERC_ENABLE_WERROR_COMPILE=OFF
                -DSHADERC_SPIRV_HEADERS_DIR=${external_dir}/spirvheaders
                -DSHADERC_SPIRV_TOOLS_DIR=${external_dir}/spirvtools
                -DSHADERC_GLSLANG_DIR=${external_dir}/glslang
            BUILD_ALWAYS False
        )
    endif(BUILD_WHISPERCPP_VULKAN_SHADERC)
endif(BUILD_WHISPERCPP_VULKAN)

ExternalProject_Add(whispercpp
    SOURCE_DIR ${external_dir}/whispercpp
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercpp
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${whispercpp_source_url}"
    URL_HASH SHA256=${whispercpp_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i "${patches_dir}/whispercpp.patch" ||
                echo "patch cmd failed, likely already patched"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}
        ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR>
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
        -DCMAKE_INSTALL_RPATH=${rpath_install_dir}
        -DCMAKE_LIBRARY_PATH=${whispercpp_additional_lib_path}
        -DGGML_BACKEND_DIR=${external_lib_dir}
        -DGGML_NATIVE=OFF -DGGML_CPU=ON -DGGML_CPU_ALL_VARIANTS=ON -DGGML_BACKEND_DL=ON
        -DGGML_BLAS=ON -DGGML_BLAS_VENDOR=OpenBLAS -DBLAS_INCLUDE_DIRS=${external_include_dir}/openblas
        -DGGML_CUDA=${BUILD_WHISPERCPP_CUDA} -DCMAKE_CUDA_ARCHITECTURES=${CMAKE_CUDA_ARCHITECTURES_STRING}
        -DGGML_HIP=${BUILD_WHISPERCPP_HIP} -DCMAKE_HIP_ARCHITECTURES=${CMAKE_HIP_ARCHITECTURES_STRING}
        -DGGML_VULKAN=${BUILD_WHISPERCPP_VULKAN}
        -DGGML_OPENCL=${BUILD_WHISPERCPP_OPENCL}
        -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
    BUILD_ALWAYS False
)

if(BUILD_WHISPERCPP_OPENCL)
    ExternalProject_Add_StepDependencies(whispercpp configure clblast)
endif()
if(BUILD_WHISPERCPP_VULKAN)
    ExternalProject_Add_StepDependencies(whispercpp configure spirvheaders)
    if(BUILD_WHISPERCPP_VULKAN_SHADERC)
        ExternalProject_Add_StepDependencies(whispercpp configure shaderc)
        ExternalProject_Add_StepDependencies(spirvtools configure spirvheaders)
        ExternalProject_Add_StepDependencies(glslang configure spirvheaders)
        ExternalProject_Add_StepDependencies(shaderc configure spirvheaders)
        ExternalProject_Add_StepDependencies(shaderc configure glslang)
        ExternalProject_Add_StepDependencies(shaderc configure spirvtools)
    endif()
endif()
if(BUILD_OPENBLAS)
    ExternalProject_Add_StepDependencies(whispercpp configure openblas)
endif()

list(APPEND deps whispercpp)

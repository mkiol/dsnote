set(whispercpp_source_url "https://github.com/ggerganov/whisper.cpp/archive/85ed71aaec8e0612a84c0b67804bde75aa75a273.zip")
set(whispercpp_checksum "cd505a7012f4d5be7c3d61fca9b46722b37eea226035c66f3a20d77d78dd6366")

set(whispercpp_flags -O3 -ffast-math -I${external_include_dir}/openblas)
set(whispercppfallback_flags ${whispercpp_flags})
if(arch_arm32)
    list(APPEND whispercpp_flags -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access)
    list(APPEND whispercppfallback_flags -mfp16-format=ieee -mno-unaligned-access)
endif()
list(JOIN whispercpp_flags " " whispercpp_flags)
list(JOIN whispercppfallback_flags " " whispercppfallback_flags)

if(arch_x8664)
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
        INSTALL_COMMAND cp libwhisper.so ${external_lib_dir}/libwhisper-clblast.so
        BUILD_ALWAYS False
    )

    ExternalProject_Add_StepDependencies(clblast configure opencl)
    ExternalProject_Add_StepDependencies(whispercppclblast configure opencl)
    ExternalProject_Add_StepDependencies(whispercppclblast configure clblast)

    list(APPEND deps whispercppclblast)
endif()

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
        -DBLAS_LIB_PATH=${external_lib_dir}/libopenblas.so
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DWHISPER_NO_AVX=ON -DWHISPER_NO_AVX2=ON -DWHISPER_NO_FMA=ON -DWHISPER_NO_F16C=ON
        -DCMAKE_C_FLAGS=${whispercppfallback_flags} -DCMAKE_CXX_FLAGS=${whispercppfallback_flags}
    INSTALL_COMMAND cp libwhisper.so ${external_lib_dir}/libwhisper-fallback.so
    BUILD_ALWAYS False
)

ExternalProject_Add(whispercpp
    SOURCE_DIR ${external_dir}/whispercpp
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercpp
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${whispercpp_source_url}"
    URL_HASH SHA256=${whispercpp_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/whispercpp.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DBLAS_LIB_PATH=${external_lib_dir}/libopenblas.so
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
    BUILD_ALWAYS False
)

ExternalProject_Add_StepDependencies(whispercppfallback configure openblas)
ExternalProject_Add_StepDependencies(whispercpp configure openblas)

list(APPEND deps whispercpp whispercppfallback)

#set(whispercpp_source_url "https://github.com/ggerganov/whisper.cpp/archive/ea1f8a50d4f70b54d9dd03205207a80019e243f9.zip")
#set(whispercpp_checksum "8bf8086d3c43966f8adc8e2d1596de42")

set(whispercpp_source_url "https://github.com/ggerganov/whisper.cpp/archive/85ed71aaec8e0612a84c0b67804bde75aa75a273.zip")
set(whispercpp_checksum "cd505a7012f4d5be7c3d61fca9b46722b37eea226035c66f3a20d77d78dd6366")

set(whispercpp_flags -O3 -ffast-math -I${external_include_dir}/openblas)
if(arch_arm32)
    list(APPEND whispercpp_flags -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access)
endif()
list(JOIN whispercpp_flags " " whispercpp_flags)

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
        -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF
        -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF
        -DWHISPER_OPENBLAS=ON
        -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
    BUILD_ALWAYS False
)

ExternalProject_Add_StepDependencies(whispercpp configure openblas)

list(APPEND deps_libs ${external_lib_dir}/libwhisper.a)
list(APPEND deps whispercpp)

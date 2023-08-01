set(piper_source_url "https://github.com/rhasspy/piper/archive/e268564deb779af984ac8f632c98727447632124.zip")
set(piper_checksum "213a31c23c862cbcd9de4231c07d32de35f4ee0b5b5dec52e9ae6dd3aa70ac12")

set(piperphonemize_source_url "https://github.com/rhasspy/piper-phonemize/archive/7f7b5bd4de22f7fe24341c5bedda0dc1e33f3666.zip")
set(piperphonemize_checksum "6bdcb21f6c5ae0deff7c9ae26bf07b994791dc800c1962fd216727e66a409929")

set(onnx_x8664_url "https://github.com/microsoft/onnxruntime/releases/download/v1.14.1/onnxruntime-linux-x64-1.14.1.tgz")
set(onnx_x8664_checksum "9a3b855e2b22ace4ab110cec10b38b74")
set(onnx_arm64_url "https://github.com/microsoft/onnxruntime/releases/download/v1.14.1/onnxruntime-linux-aarch64-1.14.1.tgz")
set(onnx_arm64_checksum "17556490ce7d111205c5c829acf509bf")
set(onnx_arm32_url "https://github.com/mkiol/dsnote/releases/download/v2.0.1/onnxruntime-linux-arm32-1.14.tgz")
set(onnx_arm32_checksum "5055ce0867a5c7c7a1920d2d76b8aace")

set(spdlog_source_url "https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz")
set(spdlog_checksum "ca5cae8d6cac15dae0ec63b21d6ad3530070650f68076f3a4a862ca293a858bb")

if(arch_x8664)
    set(onnx_url ${onnx_x8664_url})
    set(onnx_checksum ${onnx_x8664_checksum})
elseif(arch_arm32)
    set(onnx_url ${onnx_arm32_url})
    set(onnx_checksum ${onnx_arm32_checksum})
elseif(arch_arm64)
    set(onnx_url ${onnx_arm64_url})
    set(onnx_checksum ${onnx_arm64_checksum})
endif()

ExternalProject_Add(onnx
    SOURCE_DIR ${external_dir}/onnx
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/onnx
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${onnx_url}"
    URL_MD5 "${onnx_checksum}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    BUILD_ALWAYS False
    INSTALL_COMMAND mkdir -p ${external_include_dir} && mkdir -p ${external_lib_dir} &&
        cp -r --no-target-directory <SOURCE_DIR>/include ${external_include_dir} &&
        cp -r --no-target-directory <SOURCE_DIR>/lib ${external_lib_dir}
)

ExternalProject_Add(piperphonemize
    SOURCE_DIR ${external_dir}/piperphonemize
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/piperphonemize
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${piperphonemize_source_url}"
    URL_HASH SHA256=${piperphonemize_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/piperphonemize.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_CXX_FLAGS=-O3
    BUILD_ALWAYS False
)

ExternalProject_Add(spdlog
    SOURCE_DIR ${external_dir}/spdlog
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/spdlog
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${spdlog_source_url}"
    URL_HASH SHA256=${spdlog_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_INSTALL_BINDIR=${external_bin_dir}
        -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_ALWAYS False
)

ExternalProject_Add(piper
    SOURCE_DIR ${external_dir}/piper
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/piper
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${piper_source_url}"
    URL_HASH SHA256=${piper_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/piper.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_CXX_FLAGS=-O3
    BUILD_ALWAYS False
)

add_library(onnxruntime SHARED IMPORTED)
set_property(TARGET onnxruntime PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libonnxruntime.so)

ExternalProject_Add_StepDependencies(piperphonemize configure onnx)
ExternalProject_Add_StepDependencies(piper configure espeak)
ExternalProject_Add_StepDependencies(piper configure onnx)
ExternalProject_Add_StepDependencies(piper configure piperphonemize)
ExternalProject_Add_StepDependencies(piper configure spdlog)

list(APPEND deps_libs "${external_lib_dir}/libpiper_api.a" "${external_lib_dir}/libspdlog.a" "${external_lib_dir}/libpiper_phonemize.a" onnxruntime)
list(APPEND deps piper piperphonemize spdlog onnxruntime)

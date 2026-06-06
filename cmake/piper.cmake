set(piper_source_url "https://github.com/rhasspy/piper/archive/73c04d81d5590ecc46e522de3601ce7fb29fc2be.zip")
set(piper_checksum "765086f555a3bc8550ff4700ee074de691a1ab44c12c549035ae17b6a143d27e")

set(piperphonemize_source_url "https://github.com/rhasspy/piper-phonemize/archive/ba3cc06c5248215928821f1393b2b854a936991a.zip")
set(piperphonemize_checksum "802ccf0cb234dc848106d5b3682a2ec543f27723734e4352b49fccab8214d740")

set(onnx_arm32_url "https://github.com/mkiol/dsnote/releases/download/v2.0.1/onnxruntime-linux-arm32-1.14.tgz")
set(onnx_arm32_checksum "4e221f5da63526cd7060a0e21350afc5eb9ba9c050f7ae15d4f34ce1c1d1480f")
set(onnx_x8664_url "https://github.com/microsoft/onnxruntime/releases/download/v1.16.1/onnxruntime-linux-x64-1.16.1.tgz")
set(onnx_x8664_checksum "53a0f03f71587ed602e99e82773132fc634b74c2d227316fbfd4bf67181e72ed")
set(onnx_arm64_url "https://github.com/microsoft/onnxruntime/releases/download/v1.16.1/onnxruntime-linux-aarch64-1.16.1.tgz")
set(onnx_arm64_checksum "f10851b62eb44f9e811134737e7c6edd15733d2c1549cb6ce403808e9c047385")

set(spdlog_source_url "https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz")
set(spdlog_checksum "d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744")

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
    URL_HASH SHA256=${onnx_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/piperphonemize.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DESPEAK_NG_DIR=${PROJECT_BINARY_DIR}/external
        -DONNXRUNTIME_DIR=${PROJECT_BINARY_DIR}/external
    BUILD_ALWAYS False
)

ExternalProject_Add(spdlog
    SOURCE_DIR ${external_dir}/spdlog
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/spdlog
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${spdlog_source_url}"
    URL_HASH SHA256=${spdlog_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/piper.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DPIPER_PHONEMIZE_DIR=${PROJECT_BINARY_DIR}/external
        -DSPDLOG_DIR=${PROJECT_BINARY_DIR}/external
        -DFMT_DIR=${PROJECT_BINARY_DIR}/external
    BUILD_ALWAYS False
)

add_library(onnxruntime SHARED IMPORTED)
set_property(TARGET onnxruntime PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libonnxruntime.so)

ExternalProject_Add_StepDependencies(piperphonemize configure fmt)
ExternalProject_Add_StepDependencies(piperphonemize configure onnx)
ExternalProject_Add_StepDependencies(piperphonemize configure espeak)
ExternalProject_Add_StepDependencies(piper configure espeak)
ExternalProject_Add_StepDependencies(piper configure onnx)
ExternalProject_Add_StepDependencies(piper configure piperphonemize)
ExternalProject_Add_StepDependencies(piper configure spdlog)

list(APPEND deps_libs
    "${external_lib_dir}/libpiper_api.a"
    "${external_lib_dir}/libspdlog.a"
    "${external_lib_dir}/libpiper_phonemize.a"
    "${external_lib_dir}/libonnxruntime.so")
list(APPEND deps piper piperphonemize spdlog onnx)

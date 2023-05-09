set(piper_source_url "https://github.com/rhasspy/piper/archive/e967bdc1a838023fe3644f8ed7cf6e1e958b42ef.zip")
set(piper_checksum "c9924cdb6305f6982171d486d5d633e1")

set(onnx_x8664_url "https://github.com/microsoft/onnxruntime/releases/download/v1.14.1/onnxruntime-linux-x64-1.14.1.tgz")
set(onnx_x8664_checksum "9a3b855e2b22ace4ab110cec10b38b74")
set(onnx_arm64_url "https://github.com/microsoft/onnxruntime/releases/download/v1.14.1/onnxruntime-linux-aarch64-1.14.1.tgz")
set(onnx_arm64_checksum "17556490ce7d111205c5c829acf509bf")
set(onnx_arm32_url "https://github.com/mkiol/dsnote/releases/download/v2.0.1/onnxruntime-linux-arm32-1.14.tgz")
set(onnx_arm32_checksum "5055ce0867a5c7c7a1920d2d76b8aace")

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

ExternalProject_Add(espeakng
    SOURCE_DIR ${external_dir}/espeak-ng
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/espeak-ng
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${piper_source_url}"
    URL_MD5 "${piper_checksum}"
    CONFIGURE_COMMAND tar -x -C <INSTALL_DIR> -f <SOURCE_DIR>/lib/espeak-ng-1.52-patched.tar.gz && ./autogen.sh &&
        ./configure --prefix=<INSTALL_DIR> --with-pic --with-pcaudiolib=no --enable-static --disable-shared --disable-rpath
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

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

ExternalProject_Add(piper
    SOURCE_DIR ${external_dir}/piper
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/piper
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${piper_source_url}"
    URL_MD5 "${piper_checksum}"
    PATCH_COMMAND patch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/piper.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_LIBRARY_PATH=${external_lib_dir} -DCMAKE_INCLUDE_PATH=${external_include_dir}
        -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DCMAKE_CXX_FLAGS=-O3
    BUILD_ALWAYS False
)

add_library(onnxruntime SHARED IMPORTED)
set_property(TARGET onnxruntime PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libonnxruntime.so)

ExternalProject_Add_StepDependencies(piper configure espeakng)
ExternalProject_Add_StepDependencies(piper configure piper)
ExternalProject_Add_StepDependencies(piper configure onnx)

list(APPEND deps_libs "${external_lib_dir}/libpiper_api.a")
list(APPEND deps_libs "${external_lib_dir}/libespeak-ng.a" onnxruntime)

list(APPEND deps piper onnxruntime)

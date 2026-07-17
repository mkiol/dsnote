set(aprilasr_source_url "https://github.com/abb128/april-asr/archive/8b52f65dd3fbfa58064de85ab78237335b9de911.zip")
set(aprilasr_checksum "4500dc2231290e899d0e249317819c97cfbde2d9a6f240000d12603ae264b09f")

ExternalProject_Add(aprilasr
    SOURCE_DIR ${external_dir}/aprilasr
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/aprilasr
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${aprilasr_source_url}"
    URL_HASH SHA256=${aprilasr_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=lib
    BUILD_ALWAYS False
)

if(BUILD_PIPER)
    ExternalProject_Add_StepDependencies(aprilasr configure onnx)
endif()

list(APPEND deps_libs "${external_lib_dir}/libaprilasr.so")
list(APPEND deps aprilasr)

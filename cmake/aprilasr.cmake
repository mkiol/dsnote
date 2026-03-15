set(aprilasr_source_url "https://github.com/abb128/april-asr/archive/3308e68442664552de593957cad0fa443ea183dd.zip")
set(aprilasr_checksum "0640fc16a43018afb1db9f17e93f545d7ac1f07e4bc47ba318d2c7311bef2df5")

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

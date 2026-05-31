set(pybind11_source_url "https://github.com/pybind/pybind11/archive/refs/tags/v3.0.4.tar.gz")
set(pybind11_checksum "74b6a2c2b4573a400cafb6ecbf60c98df300cd3d0041296b913d02b2cbbb2676")

ExternalProject_Add(pybind11
    SOURCE_DIR ${external_dir}/pybind11
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/pybind11
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${pybind11_source_url}
    URL_HASH SHA256=${pybind11_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF
        -DPYBIND11_INSTALL=ON -DPYBIND11_TEST=OFF -DPYBIND11_FINDPYTHON=OFF -DPYBIND11_NOPYTHON=ON
    BUILD_ALWAYS False
)

list(APPEND deps pybind11)

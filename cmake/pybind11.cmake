set(pybind11_source_url "https://github.com/pybind/pybind11/archive/refs/tags/v2.10.4.tar.gz")
set(pybind11_checksum "812eda11d2a114fc0e841faf9626d2c9")

ExternalProject_Add(pybind11
    SOURCE_DIR ${external_dir}/pybind11
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/pybind11
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${pybind11_source_url}"
    URL_MD5 "${pybind11_checksum}"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF
        -DPYBIND11_INSTALL=ON -DPYBIND11_TEST=OFF -DPYBIND11_FINDPYTHON=ON
    BUILD_ALWAYS False
)

list(APPEND deps pybind11)

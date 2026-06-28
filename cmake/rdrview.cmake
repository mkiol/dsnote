set(rdrview_source_url "https://github.com/eafer/rdrview/archive/refs/tags/v0.1.5.tar.gz")
set(rdrview_checksum "e83266cb2e3b16a42f3433101d1f312350ce1442561eaded67efb51c2e8e8aab")

ExternalProject_Add(rdrview
    SOURCE_DIR ${external_dir}/rdrview
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/rdrview
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${rdrview_source_url}"
    URL_HASH SHA256=${rdrview_checksum}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
            -i ${patches_dir}/rdrview.patch ||
                echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    BUILD_ALWAYS False
)

include(FindPkgConfig)

pkg_search_module(libxml2 REQUIRED libxml-2.0)
list(APPEND deps_libs ${libxml2_LIBRARIES})
list(APPEND includes ${libxml2_INCLUDE_DIRS})


list(APPEND deps_libs 
    "${external_lib_dir}/librdrview_api.a" 
    ${libxml2_LIBRARIES})

list(APPEND deps rdrview)

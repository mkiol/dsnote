set(rhvoice_source_url "https://github.com/RHVoice/RHVoice.git")
set(rhvoice_tag "c1e897d90c663613086398cd019882e907d02314")

ExternalProject_Add(rhvoice
    SOURCE_DIR ${external_dir}/rhvoice
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/rhvoice
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    GIT_REPOSITORY "${rhvoice_source_url}"
    GIT_TAG ${rhvoice_tag}
    GIT_SHALLOW ON
    GIT_SUBMODULES "cmake/thirdParty/sanitizers" "cmake/thirdParty/CCache"
        "data/languages/Polish" "data/languages/Albanian" "data/languages/Brazilian-Portuguese"
        "data/languages/English" "data/languages/Esperanto" "data/languages/Georgian"
        "data/languages/Kyrgyz" "data/languages/Macedonian" "data/languages/Russian"
        "data/languages/Tatar" "data/languages/Ukrainian" "data/languages/Czech"
    URL "${rhvoice_source_url}"
    URL_MD5 "${rhvoice_checksum}"
    PATCH_COMMAND patch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/rhvoice.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_PREFIX_PATH=<INSTALL_DIR> -DWITH_DATA=OFF -DENABLE_MAGE=OFF -DBUILD_CLIENT=OFF
        -DBUILD_UTILS=OFF -DBUILD_TESTS=OFF -DBUILD_SERVICE=OFF -DBUILD_SPEECHDISPATCHER_MODULE=OFF
    BUILD_ALWAYS False
)

list(APPEND deps_libs "${external_lib_dir}/libRHVoice_core.so.1" "${external_lib_dir}/libRHVoice.so.1")
list(APPEND deps rhvoice)

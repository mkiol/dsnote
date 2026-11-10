set(ssplitcpp_source_url "https://github.com/ugermann/ssplit-cpp/archive/49a8e12f11945fac82581cf056560965dcb641e6.zip")
set(ssplitcpp_checksum "8255bd212ad639592b4e212b0353509a")

ExternalProject_Add(ssplitcpp
    SOURCE_DIR ${external_dir}/ssplitcpp
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/ssplitcpp
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${ssplitcpp_source_url}"
    URL_MD5 "${ssplitcpp_checksum}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/ssplitcpp.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=<INSTALL_DIR>/lib
        -DCMAKE_LIBRARY_PATH=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSSPLIT_COMPILE_LIBRARY_ONLY=ON
        -DSSPLIT_PREFER_STATIC_COMPILE=${BUILD_PCRE2} -DBUILD_SHARED_LIBS=OFF
    BUILD_ALWAYS False
)

if(BUILD_PCRE2)
    ExternalProject_Add_StepDependencies(ssplitcpp configure pcre2)
endif()

list(APPEND deps_libs "${external_lib_dir}/libssplit.a")
list(APPEND deps ssplitcpp)

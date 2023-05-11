set(ssplitcpp_source_url "https://github.com/ugermann/ssplit-cpp/archive/49a8e12f11945fac82581cf056560965dcb641e6.zip")
set(ssplitcpp_checksum "8255bd212ad639592b4e212b0353509a")

set(pcre2_source_url "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.42/pcre2-10.42.tar.bz2")
set(pcre2_checksum "a8e9ab2935d428a4807461f183034abe")

ExternalProject_Add(pcre2
    SOURCE_DIR ${external_dir}/pcre2
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/pcre2
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${pcre2_source_url}"
    URL_MD5 "${pcre2_checksum}"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=<INSTALL_DIR>/lib
        -DCMAKE_LIBRARY_PATH=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DPCRE2_STATIC_PIC=ON -DBUILD_STATIC_LIBS=ON
        -DPCRE2_SUPPORT_LIBBZ2=OFF -DPCRE2_SUPPORT_LIBZ=OFF -DPCRE2_SUPPORT_JIT=ON
    BUILD_ALWAYS False
)

ExternalProject_Add(ssplitcpp
    SOURCE_DIR ${external_dir}/ssplitcpp
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/ssplitcpp
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${ssplitcpp_source_url}"
    URL_MD5 "${ssplitcpp_checksum}"
    PATCH_COMMAND patch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/ssplitcpp.patch ||
                    echo "patch cmd failed, likely already patched"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_INSTALL_LIBDIR=<INSTALL_DIR>/lib
        -DCMAKE_LIBRARY_PATH=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSSPLIT_COMPILE_LIBRARY_ONLY=ON
        -DSSPLIT_PREFER_STATIC_COMPILE=ON -DBUILD_SHARED_LIBS=OFF
    BUILD_ALWAYS False
)

ExternalProject_Add_StepDependencies(ssplitcpp configure pcre2)

list(APPEND deps_libs "${external_lib_dir}/libssplit.a")
list(APPEND deps_libs "${external_lib_dir}/libpcre2-8.a" "${external_lib_dir}/libpcre2-posix.a")
list(APPEND deps ssplitcpp)

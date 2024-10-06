if(BUILD_VOSK)
    set(openfst_source_url "https://github.com/alphacep/openfst/archive/7dfd808194105162f20084bb4d8e4ee4b65266d5.zip")
    set(openfst_checksum "734084e424d42f16424bd5279e4f0786")
    set(kaldi_source_url "https://github.com/alphacep/kaldi/archive/2abed6b15990d9438f70863f2b58bd8af8432043.zip")
    set(kaldi_checksum "4923c5b7599184c36db3342579676fbc")
    set(vosk_source_url "https://github.com/alphacep/vosk-api/archive/128c216c6137a36fbf5b0bf64d03501e91a6eeaa.zip")
    set(vosk_checksum "1f716b8132d5678823db0531b2c8285a")

    if(${autoconf_bin} MATCHES "-NOTFOUND$")
       message(FATAL_ERROR "autoconf not found but it is required to build vosk")
    endif()
    if(${automake_bin} MATCHES "-NOTFOUND$")
       message(FATAL_ERROR "automake not found but it is required to build vosk")
    endif()
    if(${libtool_bin} MATCHES "-NOTFOUND$")
       message(FATAL_ERROR "libtool not found but it is required to build vosk")
    endif()

    ExternalProject_Add(openfst
        SOURCE_DIR ${external_dir}/openfst
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/openfst
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${openfst_source_url}"
        URL_MD5 "${openfst_checksum}"
        PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                    -i ${patches_dir}/openfst.patch ||
                        echo "patch cmd failed, likely already patched"
        CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> && autoreconf -fi &&
            <BINARY_DIR>/configure --prefix=<INSTALL_DIR> --libdir=<INSTALL_DIR>/lib
            --disable-bin --disable-dependency-tracking --enable-compact-fsts --enable-compress
            --enable-const-fsts --enable-far --enable-linear-fsts --enable-lookahead-fsts
            --enable-mpdt --enable-ngram-fsts --enable-pdt --disable-shared --enable-static
            --with-pic
        BUILD_COMMAND ${MAKE}
        BUILD_ALWAYS False
        INSTALL_COMMAND make DESTDIR=/ install
    )

    set(kaldi_flags "-O3 -I${external_include_dir}")

    ExternalProject_Add(kaldi
        SOURCE_DIR ${external_dir}/kaldi
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/kaldi
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${kaldi_source_url}"
        URL_MD5 "${kaldi_checksum}"
        PATCH_COMMAND patch --batch --forward --unified <SOURCE_DIR>/CMakeLists.txt
            -i ${patches_dir}/kaldi.patch ||
                echo "patch cmd failed, likely already patched"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
            -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DBUILD_SHARED_LIBS=OFF -DKALDI_BUILD_EXE=OFF -DKALDI_BUILD_TEST=OFF -DKALDI_VERSION=1
            -DCMAKE_CXX_FLAGS=${kaldi_flags} -DENABLE_CUDA=OFF
        BUILD_ALWAYS False
    )

    ExternalProject_Add_StepDependencies(kaldi configure openfst)

    set(vosk_flags "-O3 -I${external_include_dir}")

    ExternalProject_Add(vosk
        SOURCE_DIR ${external_dir}/vosk
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/vosk
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${vosk_source_url}"
        URL_MD5 "${vosk_checksum}"
        PATCH_COMMAND patch --batch --forward --unified <SOURCE_DIR>/CMakeLists.txt
            -i ${patches_dir}/vosk.patch ||
                echo "patch cmd failed, likely already patched"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
            -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_LIBRARY_PATH=${external_lib_dir} -DCMAKE_INCLUDE_PATH=${external_include_dir}
            -DCMAKE_CXX_FLAGS=${vosk_flags}
        BUILD_ALWAYS False
    )

    ExternalProject_Add_StepDependencies(vosk configure kaldi)

    if(BUILD_OPENBLAS)
        ExternalProject_Add_StepDependencies(vosk configure openblas)
    endif()
endif()

if(DOWNLOAD_VOSK)
    set(vosk_x8664_url "https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-x86_64-0.3.45.zip")
    set(vosk_arm64_url "https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-aarch64-0.3.45.zip")
    set(vosk_arm32_url "https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-armv7l-0.3.45.zip")

    if(arch_x8664)
        set(vosk_url ${vosk_x8664_url})
    elseif(arch_arm32)
        set(vosk_url ${vosk_arm32_url})
    elseif(arch_arm64)
        set(vosk_url ${vosk_arm64_url})
    endif()

    set(vosk_archive "${PROJECT_BINARY_DIR}/vosk.zip")

    file(DOWNLOAD ${vosk_url} ${vosk_archive} ${external_lib_dir} STATUS vosk_status)
    file(ARCHIVE_EXTRACT INPUT ${vosk_archive} DESTINATION ${external_dir}/vosk)
    find_file(vosk_lib_path libvosk.so PATHS ${external_dir}/vosk/*/ REQUIRED NO_DEFAULT_PATH)
    find_file(vosk_header_path vosk_api.h PATHS ${external_dir}/vosk/*/ REQUIRED NO_DEFAULT_PATH)
    file(COPY ${vosk_lib_path} DESTINATION ${external_lib_dir} FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
    file(COPY ${vosk_header_path} DESTINATION ${external_include_dir} FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

    add_library(vosk SHARED IMPORTED)
    set_property(TARGET vosk PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libvosk.so)
endif()

list(APPEND deps vosk)

set(openfst_source_url "https://github.com/alphacep/openfst/archive/7dfd808194105162f20084bb4d8e4ee4b65266d5.zip")
set(openfst_checksum "734084e424d42f16424bd5279e4f0786")
set(kaldi_source_url "https://github.com/alphacep/kaldi/archive/2abed6b15990d9438f70863f2b58bd8af8432043.zip")
set(kaldi_checksum "4923c5b7599184c36db3342579676fbc")
set(vosk_source_url "https://github.com/alphacep/vosk-api/archive/128c216c6137a36fbf5b0bf64d03501e91a6eeaa.zip")
set(vosk_checksum "1f716b8132d5678823db0531b2c8285a")

ExternalProject_Add(openfst
    SOURCE_DIR ${external_dir}/openfst
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/openfst
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${openfst_source_url}"
    URL_MD5 "${openfst_checksum}"
    CONFIGURE_COMMAND cp -r --no-target-directory <SOURCE_DIR> <BINARY_DIR> && autoreconf -fi &&
        <BINARY_DIR>/configure --prefix=<INSTALL_DIR>
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
        -DCMAKE_CXX_FLAGS=-O3
    BUILD_ALWAYS False
)

ExternalProject_Add_StepDependencies(vosk configure kaldi)

if(BUILD_OPENBLAS)
    ExternalProject_Add_StepDependencies(vosk configure openblas)
endif()

list(APPEND deps vosk)

# Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.5)

option(with_desktop "enable desktop ui" OFF)
option(with_sfos "enable sfos ui" OFF)

option(with_flatpak "enable flatpak build" OFF)
option(with_tests "enable tests" OFF)

option(with_trace_logs "enable trace logging" OFF)
option(with_sanitizers "enable asan and ubsan in debug build" ON)
option(with_static_sanitizers "link asan and ubsan statically in debug build" OFF)

option(download_libstt "download and install libstt shared lib" ON)
option(build_vosk "download vosk, build and install shared lib" ON)
option(build_libarchive "download libarchive, build and link statically" ON)
option(build_fmt "download fmt, build and link statically" ON)
option(build_catch2 "download catch2, build and link statically" ON)
option(build_whispercpp "download whisper.cpp, build and link statically" ON)
option(build_webrtcvad "download webrtc vad, build and link statically" ON)
option(build_openblas "download openblas, build and link shared lib" ON)
option(build_xz "download xz lib, build and link statically lib" ON)

if(${with_desktop} AND ${with_sfos})
    message(FATAL_ERROR "options with_desktop and with_sfos cannot be set together")
endif()

if(NOT ${with_desktop} AND NOT ${with_sfos})
    message(FATAL_ERROR "one of the options should be set (with_desktop or with_sfos)")
endif()

if(with_sfos)
    set(download_libstt ON)
    set(build_vosk ON)
    set(build_openblas ON)
endif()

set(info_id "dsnote")
set(info_binary_id ${info_id})
set(info_translations_id ${info_id})
if(with_flatpak)
    set(info_id "org.mkiol.Dsnote")
endif()
if(with_sfos)
    set(info_binary_id "harbour-dsnote")
endif()
set(info_name "Speech Note")
set(info_org "org.mkiol")
set(info_author "Michal Kosciesza")
set(info_author_email "michal@mkiol.net")
set(info_copyright_year "2021-2023")
set(info_support_email "dsnote@mkiol.net")
set(info_webpage "https://github.com/mkiol/dsnote")
set(info_license "Mozilla Public License 2.0")
set(info_license_url "http://mozilla.org/MPL/2.0")
set(info_license_spdx "MPL-2.0")
set(info_dbus_service "org.mkiol.Stt")
set(info_dbus_interface "org.mkiol.Stt")
set(info_conf_version "10")
set(info_translators_str "Heimen Stoffels · Bérenger Arnaud · Karry · dashinfantry")
set(info_libs_str "Qt · Coqui STT · Vosk · whisper.cpp · WebRTC VAD · libarchive · {fmt}")

set(libstt_x8664_url "https://github.com/coqui-ai/STT/releases/download/v1.4.0/native_client.tflite.Linux.tar.xz")
set(libstt_arm64_url "https://github.com/coqui-ai/STT/releases/download/v1.4.0/native_client.tflite.linux.aarch64.tar.xz")
set(libstt_arm32_url "https://github.com/coqui-ai/STT/releases/download/v1.4.0/native_client.tflite.linux.armv7.tar.xz")
set(fmt_source_url "https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip")
set(fmt_checksum "6133244fe8ef6f75c5601e8069b37b04")
set(libarchive_source_url "https://libarchive.org/downloads/libarchive-3.6.2.tar.gz")
set(libarchive_checksum "b5b8efa8cba29396816d0dd5f61f3de3")
set(catch2_source_url "https://github.com/catchorg/Catch2/archive/refs/tags/v3.3.2.tar.gz")
set(catch2_checksum "6a15aa49dbdee13f279b73cab16ae203")
set(whispercpp_source_url "https://github.com/ggerganov/whisper.cpp/archive/refs/tags/v1.2.1.tar.gz")
set(whispercpp_checksum "fe9516678337cc5dc4a44d0c3ea18c6c")
set(webrtcvad_source_url "https://github.com/webrtc-mirror/webrtc/archive/ac87c8df2780cb12c74942ec8a473718c76cb5b7.zip")
set(webrtcvad_checksum "f1489c137b354594632d260978b283a4")
set(openblas_source_url "https://github.com/xianyi/OpenBLAS/releases/download/v0.3.21/OpenBLAS-0.3.21.tar.gz")
set(openblas_checksum "ffb6120e2309a2280471716301824805")
set(openfst_source_url "https://github.com/alphacep/openfst/archive/7dfd808194105162f20084bb4d8e4ee4b65266d5.zip")
set(openfst_checksum "734084e424d42f16424bd5279e4f0786")
set(kaldi_source_url "https://github.com/alphacep/kaldi/archive/2abed6b15990d9438f70863f2b58bd8af8432043.zip")
set(kaldi_checksum "4923c5b7599184c36db3342579676fbc")
set(vosk_source_url "https://github.com/alphacep/vosk-api/archive/128c216c6137a36fbf5b0bf64d03501e91a6eeaa.zip")
set(vosk_checksum "1f716b8132d5678823db0531b2c8285a")
set(xz_source_url "https://downloads.sourceforge.net/lzmautils/xz-5.4.2.tar.gz")
set(xz_checksum "4ac4e5da95aa8604a81e32079cb00d42")

project(${info_id} VERSION 2.0.0)

if(CMAKE_SYSTEM_PROCESSOR MATCHES ".*arm.*")
    message(STATUS "arm architecture detected")
    set(arch_arm32 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES ".*aarch64.*")
    message(STATUS "arm64 architecture detected")
    set(arch_arm64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES ".*86_64.*")
    message(STATUS "x86_64 architecture detected")
    set(arch_x8664 1)
else()
    message(FATAL_ERROR "unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# dirs

set(external_dir "${PROJECT_SOURCE_DIR}/external")
set(external_include_dir "${PROJECT_BINARY_DIR}/external/include")
set(external_lib_dir "${PROJECT_BINARY_DIR}/external/lib")
set(desktop_dir "${PROJECT_SOURCE_DIR}/desktop")
set(sfos_dir "${PROJECT_SOURCE_DIR}/sfos")
set(resources_dir "${PROJECT_SOURCE_DIR}/resources")
set(flatpak_dir "${PROJECT_SOURCE_DIR}/flatpak")
set(translations_dir "${PROJECT_SOURCE_DIR}/translations")
set(tools_dir "${PROJECT_SOURCE_DIR}/tools")
set(patches_dir "${PROJECT_SOURCE_DIR}/patches")
set(tests_dir "${PROJECT_SOURCE_DIR}/tests")
set(sources_dir "${PROJECT_SOURCE_DIR}/src")
set(systemd_dir "${PROJECT_SOURCE_DIR}/systemd")
set(dbus_dir "${PROJECT_SOURCE_DIR}/dbus")
set(rpath "${CMAKE_INSTALL_PREFIX}/share/${info_binary_id}/lib")
set(binary_path "${CMAKE_INSTALL_PREFIX}/bin/${info_binary_id}")

# global config

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_SKIP_RPATH ON)

# includes

set(includes ${sources_dir} ${PROJECT_BINARY_DIR} ${external_include_dir})

# dsnote_lib

set(dsnote_lib_sources
    ${sources_dir}/audio_source.h
    ${sources_dir}/dbus_stt_adaptor.cpp
    ${sources_dir}/dbus_stt_adaptor.h
    ${sources_dir}/dbus_stt_inf.cpp
    ${sources_dir}/dbus_stt_inf.h
    ${sources_dir}/dirmodel.cpp
    ${sources_dir}/dirmodel.h
    ${sources_dir}/dsnote_app.cpp
    ${sources_dir}/dsnote_app.h
    ${sources_dir}/file_source.cpp
    ${sources_dir}/file_source.h
    ${sources_dir}/itemmodel.cpp
    ${sources_dir}/itemmodel.h
    ${sources_dir}/langs_list_model.cpp
    ${sources_dir}/langs_list_model.h
    ${sources_dir}/listmodel.cpp
    ${sources_dir}/listmodel.h
    ${sources_dir}/mic_source.cpp
    ${sources_dir}/mic_source.h
    ${sources_dir}/models_list_model.cpp
    ${sources_dir}/models_list_model.h
    ${sources_dir}/models_manager.cpp
    ${sources_dir}/models_manager.h
    ${sources_dir}/settings.cpp
    ${sources_dir}/settings.h
    ${sources_dir}/singleton.h
    ${sources_dir}/stt_config.cpp
    ${sources_dir}/stt_config.h
    ${sources_dir}/stt_service.cpp
    ${sources_dir}/stt_service.h
    ${sources_dir}/logger.cpp
    ${sources_dir}/logger.hpp
    ${sources_dir}/qtlogger.cpp
    ${sources_dir}/qtlogger.hpp
    ${sources_dir}/engine_wrapper.cpp
    ${sources_dir}/engine_wrapper.hpp
    ${sources_dir}/deepspeech_wrapper.cpp
    ${sources_dir}/deepspeech_wrapper.hpp
    ${sources_dir}/whisper_wrapper.cpp
    ${sources_dir}/whisper_wrapper.hpp
    ${sources_dir}/vosk_wrapper.cpp
    ${sources_dir}/vosk_wrapper.hpp
    ${sources_dir}/vad_wrapper.cpp
    ${sources_dir}/vad_wrapper.hpp
    ${sources_dir}/cpu_tools.cpp
    ${sources_dir}/cpu_tools.hpp
    ${sources_dir}/comp_tools.cpp
    ${sources_dir}/comp_tools.hpp
    ${sources_dir}/checksum_tools.cpp
    ${sources_dir}/checksum_tools.hpp)

configure_file(config.h.in config.h)

add_library(dsnote_lib STATIC ${dsnote_lib_sources})

# resources

set(resources "")

if(with_desktop)
    list(APPEND resources
        "${desktop_dir}/resources.qrc"
        "${CMAKE_BINARY_DIR}/translations.qrc")
endif()

if(with_sfos)
    list(APPEND resources
        "${sfos_dir}/resources.qrc")
endif()

# executable

add_executable(${info_binary_id} ${resources} "${sources_dir}/main.cpp")
target_link_libraries(${info_binary_id} dsnote_lib)

add_custom_command(TARGET ${info_binary_id} POST_BUILD
    COMMAND $<$<CONFIG:Release>:${CMAKE_STRIP}>
    ARGS --strip-all $<TARGET_FILE:${info_binary_id}>
)

# tests

if(with_tests)
    if(build_catch2)
        include(FetchContent)

        FetchContent_Declare(
          Catch2
          URL ${catch2_source_url}
          URL_HASH MD5=${catch2_checksum}
          SOURCE_DIR "${external_dir}/catch2"
          BINARY_DIR "${PROJECT_BINARY_DIR}/external/catch2"
          INSTALL_DIR "${PROJECT_BINARY_DIR}/external"
        )
        FetchContent_MakeAvailable(Catch2)
    else()
        find_package(Catch2 3 REQUIRED)
    endif()

    file(GLOB tests_src
        "${tests_dir}/*.hpp"
        "${tests_dir}/*.cpp"
    )

    add_executable(tests ${tests_src})
    target_link_libraries(tests Catch2::Catch2WithMain)
    target_link_libraries(tests dsnote_lib)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

    include(CTest)
    include(Catch)

    catch_discover_tests(tests)
endif()

# flags and definitions

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif ()

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_17)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(with_sanitizers)
        target_compile_options(compiler_flags INTERFACE
            "-pie;-Wall;-Wextra;-Wpedantic;-O1;-g3;-fsanitize=address,undefined")
        if(with_static_sanitizers)
            target_link_options(compiler_flags INTERFACE
              "-pie;-static-libasan;-static-libubsan;-fsanitize=address,undefined")
        else()
            target_link_options(compiler_flags INTERFACE "-pie;-fsanitize=address,undefined")
        endif()
    else()
        target_compile_options(compiler_flags INTERFACE
            "-pie;-Wall;-Wextra;-Wpedantic;-O1;-g3")
        target_link_options(compiler_flags INTERFACE "-pie")
    endif()
else()
    target_compile_options(compiler_flags INTERFACE "-pie;-Wall;-Wextra;-Wpedantic;-O3;-g")
    target_link_options(compiler_flags INTERFACE "-pie")
endif()

target_link_options(compiler_flags INTERFACE "-Wl,-rpath,${rpath}")

target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${with_trace_logs}>:USE_TRACE_LOGS>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${with_sfos}>:USE_SFOS>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${with_desktop}>:USE_DESKTOP>")
target_compile_definitions(compiler_flags INTERFACE "$<$<CONFIG:Debug>:DEBUG>")
target_compile_definitions(compiler_flags INTERFACE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_x8664}>:ARCH_X86_64>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_arm32}>:ARCH_ARM_32>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_arm64}>:ARCH_ARM_64>")

# translations

set(enabled_translations cs_CZ de en fr nl pl sv zh_CN)

find_package(Qt5 COMPONENTS Core LinguistTools)

set(ts_files "")
foreach(lang ${enabled_translations})
    list(APPEND ts_files "${translations_dir}/dsnote-${lang}.ts")
endforeach()

qt5_create_translation(qm_files ${CMAKE_SOURCE_DIR}/src ${desktop_dir}/qml ${sfos_dir}/qml ${ts_files})

string(REPLACE ";" " " enabled_translations_str "${enabled_translations}")
add_custom_command(
  OUTPUT translations.qrc
  COMMAND sh -c "${tools_dir}/make_translations_qrc.sh ${info_translations_id} /translations ${CMAKE_BINARY_DIR}/translations.qrc ${enabled_translations_str}"
  DEPENDS ${qm_files}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM)
add_library(translations STATIC "${CMAKE_BINARY_DIR}/translations.qrc")
target_link_libraries(${info_binary_id} translations)

# dependencies

set(deps "")
set(deps_libs compiler_flags pthread ${CMAKE_DL_LIBS})
set(deps_dirs "")

include(FindPkgConfig)
include(ExternalProject)

pkg_search_module(zlib REQUIRED zlib)
list(APPEND deps_libs ${zlib_LIBRARIES})
list(APPEND includes ${zlib_INCLUDE_DIRS})

find_package(Qt5 COMPONENTS Core Network Multimedia Qml Xml Sql Gui Quick DBus LinguistTools REQUIRED)
list(APPEND deps_libs Qt5::Core Qt5::Network Qt5::Multimedia Qt5::Gui Qt5::Quick Qt5::DBus)

if(with_desktop)
    find_package(Qt5 COMPONENTS QuickControls2 Widgets REQUIRED)
    list(APPEND deps_libs Qt5::QuickControls2 Qt5::Widgets)
endif()

if(build_libarchive)
    ExternalProject_Add(libarchive
        SOURCE_DIR ${external_dir}/libarchive
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/libarchive
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${libarchive_source_url}"
        URL_MD5 "${libarchive_checksum}"
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --bindir=<INSTALL_DIR>/bin
            --enable-shared=no --enable-static=yes --disable-acl --disable-xattr --disable-largefile
            --without-zstd --without-lz4 --without-libiconv-prefix --without-iconv --without-libb2
            --without-bz2lib --with-zlib --without-cng --without-openssl --without-xml2 --without-expat
            --without-lzma --with-pic=yes
        BUILD_COMMAND ${MAKE}
        BUILD_ALWAYS False
        INSTALL_COMMAND make DESTDIR=/ install)
    list(APPEND deps libarchive)
    list(APPEND deps_libs ${external_lib_dir}/libarchive.a)
else()
    pkg_search_module(libarchive REQUIRED libarchive)
    list(APPEND deps_libs ${libarchive_LIBRARIES})
    list(APPEND includes ${libarchive_INCLUDE_DIRS})
endif()

if(download_libstt)
    if(arch_x8664)
        set(libstt_url ${libstt_x8664_url})
    elseif(arch_arm32)
        set(libstt_url ${libstt_arm32_url})
    elseif(arch_arm64)
        set(libstt_url ${libstt_arm64_url})
    endif()

    set(libstt_archive "${PROJECT_BINARY_DIR}/libstt.tar.xz")

    file(DOWNLOAD ${libstt_url} ${libstt_archive} ${external_lib_dir} STATUS libstt_status)
    file(ARCHIVE_EXTRACT INPUT ${libstt_archive} DESTINATION ${external_lib_dir}
        PATTERNS *.so VERBOSE)

    add_library(stt SHARED IMPORTED)
    set_property(TARGET stt PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libstt.so)
    list(APPEND deps stt)

    add_library(kenlm SHARED IMPORTED)
    set_property(TARGET kenlm PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libkenlm.so)
    list(APPEND deps kenlm)
endif()

if(build_vosk)
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
            INSTALL_COMMAND make DESTDIR=/ install)

    set(kaldi_flags "-O3 -I${external_include_dir}")
    ExternalProject_Add(kaldi
        SOURCE_DIR ${external_dir}/kaldi
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/kaldi
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${kaldi_source_url}"
        URL_MD5 "${kaldi_checksum}"
        PATCH_COMMAND patch --forward --unified <SOURCE_DIR>/CMakeLists.txt
            -i ${patches_dir}/kaldi.patch ||
                echo "patch cmd failed, likely already patched"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
            -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DBUILD_SHARED_LIBS=OFF -DKALDI_BUILD_EXE=OFF -DKALDI_BUILD_TEST=OFF -DKALDI_VERSION=1
            -DCMAKE_CXX_FLAGS=${kaldi_flags} -DENABLE_CUDA=OFF
        BUILD_ALWAYS False)
    ExternalProject_Add_StepDependencies(kaldi configure openfst)

    ExternalProject_Add(vosk
        SOURCE_DIR ${external_dir}/vosk
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/vosk
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${vosk_source_url}"
        URL_MD5 "${vosk_checksum}"
        PATCH_COMMAND patch --forward --unified <SOURCE_DIR>/CMakeLists.txt
            -i ${patches_dir}/vosk.patch ||
                echo "patch cmd failed, likely already patched"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
            -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_LIBRARY_PATH=${external_lib_dir} -DCMAKE_INCLUDE_PATH=${external_include_dir}
            -DCMAKE_CXX_FLAGS=-O3
        BUILD_ALWAYS False)
    ExternalProject_Add_StepDependencies(vosk configure kaldi)
    ExternalProject_Add_StepDependencies(vosk configure openblas)

    list(APPEND deps vosk)
endif()

if(build_fmt)
    ExternalProject_Add(fmt
        SOURCE_DIR ${external_dir}/fmt
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/fmt
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${fmt_source_url}"
        URL_MD5 "${fmt_checksum}"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
            -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir} -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFMT_DOC=OFF -DFMT_TEST=OFF -DFMT_FUZZ=OFF
        BUILD_ALWAYS False)
    list(APPEND deps_libs ${external_lib_dir}/libfmt.a)
    list(APPEND deps fmt)
else()
    pkg_search_module(fmt REQUIRED fmt)
    list(APPEND deps_libs ${fmt_LIBRARIES})
    list(APPEND includes ${fmt_INCLUDE_DIRS})
endif()

if(build_whispercpp)
    set(whispercpp_flags -O3 -ffast-math -I${external_include_dir}/openblas)
    if(arch_arm32)
        list(APPEND whispercpp_flags -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access)
    endif()
    list(JOIN whispercpp_flags " " whispercpp_flags)

    ExternalProject_Add(whispercpp
        SOURCE_DIR ${external_dir}/whispercpp
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/whispercpp
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${whispercpp_source_url}"
        URL_MD5 "${whispercpp_checksum}"
        PATCH_COMMAND patch --unified -p1 --directory=<SOURCE_DIR>
                    -i ${patches_dir}/whispercpp.patch ||
                        echo "patch cmd failed, likely already patched"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_LIBRARY_PATH=${external_lib_dir}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF
            -DWHISPER_BUILD_TESTS=OFF -DWHISPER_BUILD_EXAMPLES=OFF -DWHISPER_SUPPORT_OPENBLAS=ON
            -DCMAKE_C_FLAGS=${whispercpp_flags} -DCMAKE_CXX_FLAGS=${whispercpp_flags}
        BUILD_ALWAYS False)
    ExternalProject_Add_StepDependencies(whispercpp configure openblas)

    list(APPEND deps_libs ${external_lib_dir}/static/libwhisper.a)
    list(APPEND deps whispercpp)
else()
    find_library(whispercpp_path whisper REQUIRED)
    list(APPEND deps_libs ${whispercpp_path})
endif()

if(build_webrtcvad)
    ExternalProject_Add(webrtcvad
        SOURCE_DIR ${external_dir}/webrtcvad
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/webrtcvad
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${webrtcvad_source_url}"
        URL_MD5 "${webrtcvad_checksum}"
        PATCH_COMMAND patch --forward --unified -p0 <SOURCE_DIR>/CMakeLists.txt
                    -i ${patches_dir}/webrtcvad.patch ||
                        echo "patch cmd failed, likely already patched"
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        BUILD_ALWAYS False)
    list(APPEND deps_libs ${external_lib_dir}/libvad.a)
    list(APPEND deps webrtcvad)
else()
    find_library(webrtcvad_path vad REQUIRED)
    list(APPEND deps_libs ${webrtcvad_path})
endif()

if(build_openblas)
    set(openblas_opts
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/external
        -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_INSTALL_INCLUDEDIR=${external_include_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_TESTING=OFF
        -DBUILD_WITHOUT_LAPACK=OFF
        -DC_LAPACK=ON
        -DDYNAMIC_ARCH=ON
        -DBUILD_STATIC_LIBS=OFF
        -DBUILD_SHARED_LIBS=ON
    )
    if(arch_arm32)
        list(APPEND openblas_opts
            "-DCMAKE_C_FLAGS=-mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access"
            -DTARGET=ARMV7)
    elseif(arch_arm64)
        list(APPEND openblas_opts -DTARGET=ARMV8)
    endif()

    ExternalProject_Add(openblas
        SOURCE_DIR ${external_dir}/openblas
        BINARY_DIR ${PROJECT_BINARY_DIR}/external/openblas
        INSTALL_DIR ${PROJECT_BINARY_DIR}/external
        URL "${openblas_source_url}"
        URL_MD5 "${openblas_checksum}"
        CMAKE_ARGS ${openblas_opts}
        BUILD_ALWAYS False)

    list(APPEND deps_libs "${external_lib_dir}/libopenblas.so")
    list(APPEND deps openblas)
else()
    pkg_search_module(openblas REQUIRED openblas)
    list(APPEND deps_libs ${openblas_LIBRARIES})
    list(APPEND includes ${openblas_INCLUDE_DIRS})
endif()

if(build_xz)
    ExternalProject_Add(xz
            SOURCE_DIR ${external_dir}/xz
            BINARY_DIR ${PROJECT_BINARY_DIR}/external/xz
            INSTALL_DIR ${PROJECT_BINARY_DIR}/external
            URL "${xz_source_url}"
            URL_MD5 "${xz_checksum}"
            CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
                --disable-encoders --disable-xz --disable-xzdec --disable-lzmadec --disable-lzmainfo
                --disable-lzma-links --disable-scripts --disable-doc --disable-shared --enable-static
                --with-pic
            BUILD_COMMAND ${MAKE}
            BUILD_ALWAYS False
            INSTALL_COMMAND make DESTDIR=/ install)
    list(APPEND deps_libs "${external_lib_dir}/liblzma.a")
    list(APPEND deps xz)
else()
    pkg_search_module(xz REQUIRED liblzma>=5.4.1)
    list(APPEND deps_libs ${xz_LIBRARIES})
    list(APPEND includes ${xz_INCLUDE_DIRS})
endif()

if(with_sfos)
    pkg_search_module(sailfish REQUIRED sailfishapp)
    list(APPEND deps_libs ${sailfish_LIBRARIES})
    list(APPEND includes ${sailfish_INCLUDE_DIRS})

    pkg_search_module(mlite5 REQUIRED mlite5)
    list(APPEND deps_libs ${mlite5_LIBRARIES})
    list(APPEND includes ${mlite5_INCLUDE_DIRS})
endif()

target_include_directories(${info_binary_id} PRIVATE ${includes})
target_link_libraries(${info_binary_id} ${deps_libs})
if(deps)
    add_dependencies(${info_binary_id} ${deps})
endif()

target_include_directories(dsnote_lib PRIVATE ${includes})
target_link_libraries(dsnote_lib ${deps_libs})
target_link_directories(dsnote_lib PRIVATE ${deps_dirs})
if(deps)
    add_dependencies(dsnote_lib ${deps})
endif()

if(with_tests)
    target_include_directories(tests PRIVATE ${includes})
    target_link_libraries(tests ${deps_libs})
    if(deps)
        add_dependencies(tests ${deps})
    endif()
endif()

# install

install(TARGETS ${info_binary_id} RUNTIME DESTINATION bin)

if(with_sfos)
    install(FILES "${sfos_dir}/${info_binary_id}.desktop" DESTINATION share/applications)

    if(download_libstt)
        install(FILES "${external_lib_dir}/libstt.so" DESTINATION share/${info_binary_id}/lib)
        install(FILES "${external_lib_dir}/libkenlm.so" DESTINATION share/${info_binary_id}/lib)
    endif()

    if(build_openblas)
        install(FILES "${external_lib_dir}/libopenblas.so.0.3" DESTINATION share/${info_binary_id}/lib RENAME libopenblas.so.0)
    endif()

    if(build_vosk)
        install(FILES "${external_lib_dir}/libvosk.so" DESTINATION share/${info_binary_id}/lib)
    endif()

    install(FILES "${sfos_dir}/icons/86x86/${info_binary_id}.png" DESTINATION share/icons/hicolor/86x86/apps)
    install(FILES "${sfos_dir}/icons/108x108/${info_binary_id}.png" DESTINATION share/icons/hicolor/108x108/apps)
    install(FILES "${sfos_dir}/icons/128x128/${info_binary_id}.png" DESTINATION share/icons/hicolor/128x128/apps)
    install(FILES "${sfos_dir}/icons/172x172/${info_binary_id}.png" DESTINATION share/icons/hicolor/172x172/apps)

    install(FILES ${qm_files} DESTINATION "share/${info_binary_id}/translations")

    install(DIRECTORY "${sfos_dir}/qml" DESTINATION share/${info_binary_id})

    configure_file("${systemd_dir}/${info_id}.service" "${PROJECT_BINARY_DIR}/${info_binary_id}.service")
    install(FILES "${PROJECT_BINARY_DIR}/${info_binary_id}.service" DESTINATION lib/systemd/user)

    configure_file("${dbus_dir}/${info_dbus_service}.service" "${PROJECT_BINARY_DIR}/${info_dbus_service}.service")
    install(FILES "${PROJECT_BINARY_DIR}/${info_dbus_service}.service" DESTINATION "share/dbus-1/services")
endif()

if(with_desktop)
    install(FILES "${desktop_dir}/${info_binary_id}.desktop" DESTINATION share/applications)
    install(FILES "${desktop_dir}/${info_binary_id}.svg" DESTINATION share/icons/hicolor/scalable/apps)

    if(download_libstt)
        install(FILES "${external_lib_dir}/libstt.so" DESTINATION lib)
        install(FILES "${external_lib_dir}/libkenlm.so" DESTINATION lib)
    endif()

    if(build_openblas)
        install(FILES "${external_lib_dir}/libopenblas.so.0.3" DESTINATION lib RENAME libopenblas.so.0)
    endif()

    if(build_vosk)
        install(FILES "${external_lib_dir}/libvosk.so" DESTINATION lib)
    endif()

    configure_file("${systemd_dir}/${info_id}.service" "${PROJECT_BINARY_DIR}/${info_binary_id}.service")
    install(FILES "${PROJECT_BINARY_DIR}/${info_binary_id}.service" DESTINATION lib/systemd/user)

    configure_file("${dbus_dir}/${info_dbus_service}.service" "${PROJECT_BINARY_DIR}/${info_dbus_service}.service")
    install(FILES "${PROJECT_BINARY_DIR}/${info_dbus_service}.service" DESTINATION lib/systemd/user)
endif()

# misc

if(with_sfos)
    file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
        "${CMAKE_INSTALL_PREFIX}\n${CMAKE_BINARY_DIR}/${info_binary_id}:bin\n")
endif()
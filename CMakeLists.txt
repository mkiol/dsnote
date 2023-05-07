# Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.5)

option(WITH_DESKTOP "enable desktop ui" OFF)
option(WITH_SFOS "enable sfos ui" OFF)

option(WITH_FLATPAK "enable flatpak build" OFF)
option(WITH_TESTS "enable tests" OFF)

option(WITH_TRACE_LOGS "enable trace logging" OFF)
option(WITH_SANITIZERS "enable asan and ubsan in debug build" ON)
option(WITH_STATIC_SANITIZERS "link asan and ubsan statically in debug build" OFF)
option(WITH_PY "enable python libraries" ${WITH_DESKTOP})

option(DOWNLOAD_LIBSTT "download and install libstt shared lib" ON)
option(BUILD_VOSK "download vosk, build and install shared lib" ON)
option(BUILD_LIBARCHIVE "download libarchive, build and link statically" ON)
option(BUILD_FMT "download fmt, build and link statically" ON)
option(BUILD_CATCH2 "download catch2, build and link statically" ON)
option(BUILD_WHISPERCPP "download whisper.cpp, build and link statically" ON)
option(BUILD_WEBRTCVAD "download webrtc vad, build and link statically" ON)
option(BUILD_OPENBLAS "download openblas, build and link shared lib" ON)
option(BUILD_XZ "download xz lib, build and link statically" ON)
option(BUILD_RNNOISE "download rnnoise lib, build and link statically" ON)
option(BUILD_PYBIND11 "download pybind11, build and link statically" ON)
option(BUILD_PYMODULES "make python modules archive" OFF)
option(BUILD_PIPER "download piper lib, build and link statically" ON)

if(${WITH_DESKTOP} AND ${WITH_SFOS})
    message(FATAL_ERROR "options WITH_DESKTOP and WITH_SFOS cannot be set together")
endif()

if(NOT ${WITH_DESKTOP} AND NOT ${WITH_SFOS})
    message(FATAL_ERROR "one of the options should be set (WITH_DESKTOP or WITH_SFOS)")
endif()

if(WITH_DESKTOP)
    set(BUILD_PYMODULES OFF)
endif()
if(WITH_SFOS)
    if (WITH_PY)
        set(BUILD_PYMODULES ON)
    endif()
endif()

set(info_id "dsnote")
set(info_binary_id ${info_id})
set(info_translations_id ${info_id})
if(WITH_FLATPAK)
    set(info_id "org.mkiol.Dsnote")
endif()
if(WITH_SFOS)
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
set(info_dbus_service "org.mkiol.Speech")
set(info_dbus_interface "org.mkiol.Speech")
set(info_conf_version "11")
set(info_translators_str "Heimen Stoffels · Bérenger Arnaud · Karry · dashinfantry · Åke Engelbrektson")
set(info_libs_str "Qt · Coqui STT · Vosk · whisper.cpp · WebRTC-VAD · libarchive · RNNoise-nu · {fmt} · 🤗Transformers")

project(${info_id} VERSION 2.0.1)

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

include(GNUInstallDirs)

set(external_dir "${PROJECT_SOURCE_DIR}/external")
set(external_include_dir "${PROJECT_BINARY_DIR}/external/include")
set(external_lib_dir "${PROJECT_BINARY_DIR}/external/lib")
set(external_share_dir "${PROJECT_BINARY_DIR}/external/share")
set(external_bin_dir "${PROJECT_BINARY_DIR}/external/bin")
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
set(cmake_path "${PROJECT_SOURCE_DIR}/cmake")

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
    ${sources_dir}/speech_config.cpp
    ${sources_dir}/speech_config.h
    ${sources_dir}/speech_service.cpp
    ${sources_dir}/speech_service.h
    ${sources_dir}/logger.cpp
    ${sources_dir}/logger.hpp
    ${sources_dir}/qtlogger.cpp
    ${sources_dir}/qtlogger.hpp
    ${sources_dir}/stt_engine.cpp
    ${sources_dir}/stt_engine.hpp
    ${sources_dir}/ds_engine.cpp
    ${sources_dir}/ds_engine.hpp
    ${sources_dir}/whisper_engine.cpp
    ${sources_dir}/whisper_engine.hpp
    ${sources_dir}/vosk_engine.cpp
    ${sources_dir}/vosk_engine.hpp
    ${sources_dir}/vad.cpp
    ${sources_dir}/vad.hpp
    ${sources_dir}/cpu_tools.cpp
    ${sources_dir}/cpu_tools.hpp
    ${sources_dir}/comp_tools.cpp
    ${sources_dir}/comp_tools.hpp
    ${sources_dir}/checksum_tools.cpp
    ${sources_dir}/checksum_tools.hpp
    ${sources_dir}/denoiser.hpp
    ${sources_dir}/denoiser.cpp
    ${sources_dir}/punctuator.hpp
    ${sources_dir}/punctuator.cpp
    ${sources_dir}/py_tools.hpp
    ${sources_dir}/py_tools.cpp
    ${sources_dir}/tts_engine.hpp
    ${sources_dir}/tts_engine.cpp
    ${sources_dir}/piper_engine.hpp
    ${sources_dir}/piper_engine.cpp
    ${sources_dir}/coqui_engine.hpp
    ${sources_dir}/coqui_engine.cpp
    ${sources_dir}/simdjson.h
    ${sources_dir}/simdjson.cpp
    ${sources_dir}/py_executor.hpp
    ${sources_dir}/py_executor.cpp
)

configure_file(config.h.in config.h)

add_library(dsnote_lib STATIC ${dsnote_lib_sources})

# resources

set(resources "")

if(WITH_DESKTOP)
    list(APPEND resources
        "${desktop_dir}/resources.qrc"
        "${CMAKE_BINARY_DIR}/translations.qrc")
endif()

if(WITH_SFOS)
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

if(WITH_TESTS)
    include(${cmake_path}/tests.cmake)
endif()

# flags and definitions

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif ()

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_17)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(WITH_SANITIZERS)
        target_compile_options(compiler_flags INTERFACE
            "-pie;-Wall;-Wextra;-Wpedantic;-O1;-g3;-fsanitize=address,undefined")
        if(WITH_STATIC_SANITIZERS)
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

target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_TRACE_LOGS}>:USE_TRACE_LOGS>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_SFOS}>:USE_SFOS>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_DESKTOP}>:USE_DESKTOP>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_PY}>:USE_PY>")
target_compile_definitions(compiler_flags INTERFACE "$<$<CONFIG:Debug>:DEBUG>")
target_compile_definitions(compiler_flags INTERFACE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_x8664}>:ARCH_X86_64>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_arm32}>:ARCH_ARM_32>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_arm64}>:ARCH_ARM_64>")

# translations

include(${cmake_path}/translations.cmake)

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

if(WITH_DESKTOP)
    find_package(Qt5 COMPONENTS QuickControls2 Widgets REQUIRED)
    list(APPEND deps_libs Qt5::QuickControls2 Qt5::Widgets)
endif()

if(BUILD_PYMODULES)
    include(${cmake_path}/pymodules.cmake)
endif()

if(BUILD_RNNOISE)
    include(${cmake_path}/rnnoise.cmake)
endif()

if(BUILD_LIBARCHIVE)
    include(${cmake_path}/libarchive.cmake)
else()
    pkg_search_module(libarchive REQUIRED libarchive)
    list(APPEND deps_libs ${libarchive_LIBRARIES})
    list(APPEND includes ${libarchive_INCLUDE_DIRS})
endif()

if(DOWNLOAD_LIBSTT)
    include(${cmake_path}/libstt.cmake)
endif()

if(BUILD_VOSK)
    include(${cmake_path}/vosk.cmake)
endif()

if(BUILD_FMT)
    include(${cmake_path}/fmt.cmake)
else()
    pkg_search_module(fmt REQUIRED fmt)
    list(APPEND deps_libs ${fmt_LIBRARIES})
    list(APPEND includes ${fmt_INCLUDE_DIRS})
endif()

if(BUILD_WHISPERCPP)
    include(${cmake_path}/whispercpp.cmake)
else()
    find_library(whispercpp_path whisper REQUIRED)
    list(APPEND deps_libs ${whispercpp_path})
endif()

if(BUILD_WEBRTCVAD)
    include(${cmake_path}/webrtcvad.cmake)
else()
    find_library(webrtcvad_path vad REQUIRED)
    list(APPEND deps_libs ${webrtcvad_path})
endif()

if(BUILD_OPENBLAS)
    include(${cmake_path}/openblas.cmake)
else()
    pkg_search_module(openblas REQUIRED openblas)
    list(APPEND deps_libs ${openblas_LIBRARIES})
    list(APPEND includes ${openblas_INCLUDE_DIRS})
endif()

if(BUILD_XZ)
    include(${cmake_path}/xz.cmake)
else()
    pkg_search_module(xz REQUIRED liblzma>=5.4.1)
    list(APPEND deps_libs ${xz_LIBRARIES})
    list(APPEND includes ${xz_INCLUDE_DIRS})
endif()

if(BUILD_PIPER)
    include(${cmake_path}/piper.cmake)
else()
    find_library(piper_path piper_api REQUIRED)
    list(APPEND deps_libs ${piper_path})
endif()

find_package(Python COMPONENTS Development REQUIRED)
list(APPEND deps_dirs ${Python_LIBRARY_DIRS})
list(APPEND deps_libs ${Python_LIBRARIES})
list(APPEND includes ${Python_INCLUDE_DIRS})

if(BUILD_PYBIND11)
    include(${cmake_path}/pybind11.cmake)
else()
    pkg_search_module(pybind11 REQUIRED pybind11)
    list(APPEND deps_libs ${pybind11_LIBRARIES})
    list(APPEND includes ${pybind11_INCLUDE_DIRS})
endif()

if(WITH_SFOS)
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

if(WITH_TESTS)
    target_include_directories(tests PRIVATE ${includes})
    target_link_libraries(tests ${deps_libs})
    if(deps)
        add_dependencies(tests ${deps})
    endif()
endif()

# install

if(WITH_SFOS)
    include(${cmake_path}/install_sfos.cmake)
endif()

if(WITH_DESKTOP)
    include(${cmake_path}/install_desktop.cmake)
endif()

# misc

if(WITH_SFOS)
    file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
        "${CMAKE_INSTALL_PREFIX}\n${CMAKE_BINARY_DIR}/${info_binary_id}:bin\n")
endif()

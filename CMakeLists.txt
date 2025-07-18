# Copyright (C) 2023-2025 Michal Kosciesza <michal@mkiol.net>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.19)

option(WITH_DESKTOP "enable desktop ui" OFF)
option(WITH_SFOS "enable sfos ui" OFF)

option(WITH_FLATPAK "enable flatpak build" OFF)
option(WITH_TESTS "enable tests" OFF)

option(WITH_TRACE_LOGS "enable trace logging" OFF)
option(WITH_SANITIZERS "enable asan and ubsan in debug build" ON)
option(WITH_STATIC_SANITIZERS "link asan and ubsan statically in debug build" OFF)
option(WITH_PY "enable python libraries" ${WITH_DESKTOP})
option(WITH_SYSTEMD_SERVICE "enable systemd service" ${WITH_SFOS})
option(WITH_X11_FEATURES "enable x11 features" ON)

option(DOWNLOAD_LIBSTT "download libstt binary and install shared lib" ON)
option(DOWNLOAD_VOSK "download vosk binary and install shared lib (don't use together with BUILD_VOSK)" OFF)
option(BUILD_VOSK "download sources of vosk, build and install shared lib (don't use together with DOWNLOAD_VOSK)" ON)
option(BUILD_LIBARCHIVE "download sources of libarchive, build and link statically" ON)
option(BUILD_FMT "download sources of fmt, build and link statically" ON)
option(BUILD_CATCH2 "download sources of catch2, build and link statically" ON)
option(BUILD_WHISPERCPP "download sources of whisper.cpp, build and install shared lib" ON)
option(BUILD_WHISPERCPP_CUBLAS "build also cublas version of whisper.cpp" OFF)
option(BUILD_WHISPERCPP_HIPBLAS "build also hipblas version of whisper.cpp" OFF)
option(BUILD_WHISPERCPP_CLBLAST "build also clblast version of whisper.cpp" OFF)
option(BUILD_WHISPERCPP_OPENVINO "build also open-vino version of whisper.cpp" OFF)
option(BUILD_WHISPERCPP_VULKAN "build also vulkan version of whisper.cpp" OFF)
option(BUILD_WEBRTCVAD "download sources of webrtc vad, build and link statically" ON)
option(BUILD_OPENBLAS "download sources of openblas, build and install shared lib" ON)
option(BUILD_XZ "download sources of xz lib, build and link statically" ON)
option(BUILD_RNNOISE "download sources of rnnoise-nu lib, build and link statically" ON)
option(BUILD_PYBIND11 "download sources of pybind11, build and link statically" ON)
option(BUILD_PYTHON_MODULE "make python module archive" OFF)
option(BUILD_ESPEAK "download sources of espeak-ng lib, build and link statically" ON)
option(BUILD_ESPEAK_MODULE "make espeak module archive" ON)
option(BUILD_PIPER "download sources of piper lib, build and link statically" ON)
option(BUILD_SSPLITCPP "download sources of ssplitcpp lib, build and link statically" ON)
option(BUILD_RHVOICE "download sources of rhvoice lib, build and install shared lib" ON)
option(BUILD_RHVOICE_MODULE "make rhvoice module archive" ON)
option(BUILD_BERGAMOT "download sources of bergamot-translator lib, build and install shared lib" ON)
option(BUILD_RUBBERBAND "download sources of rubberband lib, build and link statically" ON)
option(BUILD_FFMPEG "download sources of ffmpeg libs, build and link statically" ON)
option(BUILD_TAGLIB "download sources of taglib, build and link statically" ON)
option(BUILD_LIBNUMBERTEXT "download sources of libnumbertext, build and link statically" ON)
option(BUILD_QHOTKEY "download sources of qhotkey lib, build and link statically" ON)
option(BUILD_APRILASR "download sources of april-asr lib, build and link statically" ON)
option(BUILD_HTML2MD "download sources of html2md lib, build and link statically" ON)
option(BUILD_MADDY "download sources of maddy lib" ON)
option(BUILD_XDO "download sources of xdo lib and link statically" ON)
option(BUILD_SAM "download sources of sam and link statically" ON)
option(BUILD_XKBCOMMON "download sources of xkbcommon build and link statically" OFF)
option(BUILD_QQC2_BREEZE_STYLE "download sources of QQC2 Breeze Style, build and install" OFF)

# options used only for testing
option(DEV_USE_PYTHON311 OFF)
option(DEV_USE_PYTHON312 OFF)

if(${WITH_DESKTOP} AND ${WITH_SFOS})
    message(FATAL_ERROR "options WITH_DESKTOP and WITH_SFOS cannot be set together")
endif()

if(${DOWNLOAD_VOSK} AND ${BUILD_VOSK})
    set(BUILD_VOSK OFF)
    message(WARNING "disabling BUILD_VOSK because DOWNLOAD_VOSK is on")
endif()

if(NOT ${WITH_DESKTOP} AND NOT ${WITH_SFOS})
    message(FATAL_ERROR "one of the options should be set (WITH_DESKTOP or WITH_SFOS)")
endif()

if(WITH_DESKTOP)
    if(WITH_FLATPAK)
        set(BUILD_PYTHON_MODULE OFF)
        set(BUILD_QQC2_BREEZE_STYLE OFF)
    endif()
endif()

if(WITH_SFOS)
    if(${WITH_PY} AND NOT ${BUILD_PYTHON_MODULE})
        set(BUILD_PYTHON_MODULE ON)
        message(WARNING "enabling BUILD_PYTHON_MODULE because WITH_PY is on")
    endif()
    if(WITH_X11_FEATURES)
        set(WITH_X11_FEATURES OFF)
        message(WARNING "disabling WITH_X11_FEATURES on sfos")
    endif()
    if(BUILD_XDO)
        set(BUILD_XDO OFF)
        message(WARNING "disabling BUILD_XDO on sfos")
    endif()
    if(BUILD_QQC2_BREEZE_STYLE)
        set(BUILD_QQC2_BREEZE_STYLE OFF)
        message(WARNING "disabling BUILD_QQC2_BREEZE_STYLE on sfos")
    endif()
endif()

if(NOT ${WITH_X11_FEATURES})
    if(BUILD_XDO)
        set(BUILD_XDO OFF)
        message(WARNING "disabling BUILD_XDO because WITH_X11_FEATURES is off")
    endif()
    if(BUILD_QHOTKEY)
        set(BUILD_QHOTKEY OFF)
        message(WARNING "disabling BUILD_QHOTKEY because WITH_X11_FEATURES is off")
    endif()
endif()

set(id "dsnote")
set(info_name "Speech Note")
set(info_org "net.mkiol")
set(info_domain "mkiol.net")
set(info_dbus_speech_service "net.mkiol.Speech")
set(info_dbus_speech_interface "net.mkiol.Speech")
set(info_dbus_app_service "net.mkiol.dsnote")
set(info_dbus_app_path "/net/mkiol/dsnote")
set(info_dbus_app_interface "net.mkiol.dsnote")
set(info_dbus_app_id ${id})
set(info_id ${id})
set(info_binary_id ${id})
set(info_translations_id ${id})
set(info_app_icon_id ${id})
if(WITH_SFOS)
    set(info_org "org.mkiol")
    set(info_domain "mkiol.org")
    set(info_dbus_speech_service "org.mkiol.Speech")
    set(info_dbus_speech_interface "org.mkiol.Speech")
    set(info_dbus_app_service "org.mkiol.dsnote")
    set(info_dbus_app_path "/org/mkiol/dsnote")
    set(info_dbus_app_interface "org.mkiol.dsnote")
    set(info_binary_id "harbour-dsnote")
    set(info_app_icon_id "harbour-dsnote")
endif()
if(WITH_FLATPAK)
    set(info_dbus_app_service "net.mkiol.SpeechNote")
    set(info_dbus_app_path "/net/mkiol/SpeechNote")
    set(info_dbus_app_interface "net.mkiol.SpeechNote")
    set(info_dbus_app_id "SpeechNote")
    set(info_app_icon_id "net.mkiol.SpeechNote")
endif()
set(info_author "Michal Kosciesza")
set(info_author_email "michal@mkiol.net")
set(info_copyright_year "2021-2025")
set(info_support_email "${id}@mkiol.net")
set(info_webpage "https://github.com/mkiol/${id}")
set(info_webpage_additional "https://gitlab.com/mkiol/${id}")
set(info_license "Mozilla Public License 2.0")
set(info_license_url "http://mozilla.org/MPL/2.0")
set(info_license_spdx "MPL-2.0")
set(info_conf_version "111")
set(info_addon_version "1.4") # if run as a flatpak pkg, the required add-on version
#set(info_version_type "-beta")
set(info_version_type "")
set(info_translators_str "Heimen Stoffels · Bérenger Arnaud · Karry · dashinfantry · \
Åke Engelbrektson · L'Africain · Albano Battistella · Locness3 · DevSJR · Vovkiv · Niklas Halvorsen · \
Matjaž Jeran · Victor Alonso Barberan · Bruno Veilleux · Abdullah Albaroty · 李 阳 · Erdem Uygun · Armin Maier")
set(info_libs_str "Qt · Coqui STT · Vosk · whisper.cpp · WebRTC-VAD · libarchive · RNNoise-nu · \
{fmt} · Piper · RHVoice · ssplit-cpp · nlohmann JSON · eSpeak NG · Bergamot Translator · \
Rubber Band Library · simdjson · astrunc · FFmpeg · LAME · Vorbis · Opus · TagLib · \
libnumbertext · april-asr · html2md · maddy · SAM")
if(WITH_DESKTOP)
    set(info_libs_str "${info_libs_str} · KDBusAddons")

    if(WITH_X11_FEATURES)
        set(info_libs_str "${info_libs_str} · QHotkey · libxdo")
    endif()
endif()

project(${id} VERSION 4.8.1)

set(arch_arm32 0)
set(arch_arm64 0)
set(arch_x8664 0)
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

if(WITH_PY)
    if(WITH_FLATPAK)
        set(info_libs_str "${info_libs_str} · Hugging Face Transformers · Coqui TTS · faster-whisper · \
Mimic 3 · Unikud · WhisperSpeech · Parler-TTS · Kokoro TTS · F5-TTS")
    endif()
endif()

if(NOT ${arch_x8664})
    if(BUILD_WHISPERCPP_CUBLAS)
        set(BUILD_WHISPERCPP_CUBLAS OFF)
        message(WARNING "disabling BUILD_WHISPERCPP_CUBLAS because it is supported only on x86_64")
    endif()
    if(BUILD_WHISPERCPP_HIPBLAS)
        set(BUILD_WHISPERCPP_HIPBLAS OFF)
        message(WARNING "disabling BUILD_WHISPERCPP_HIPBLAS because it is supported only on x86_64")
    endif()
    if(BUILD_WHISPERCPP_CLBLAST)
        set(BUILD_WHISPERCPP_CLBLAST OFF)
        message(WARNING "disabling BUILD_WHISPERCPP_CLBLAST because it is supported only on x86_64")
    endif()
    if(BUILD_WHISPERCPP_OPENVINO)
        set(BUILD_WHISPERCPP_OPENVINO OFF)
        message(WARNING "disabling BUILD_WHISPERCPP_OPENVINO because it is supported only on x86_64")
    endif()
endif()
if(arch_arm32)
    if(BUILD_WHISPERCPP_VULKAN)
        message(WARNING "disabling BUILD_WHISPERCPP_VULKAN because it is not supported only on arm32")
    endif()
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
set(binary_path "${CMAKE_INSTALL_PREFIX}/bin/${info_binary_id}")
set(cmake_path "${PROJECT_SOURCE_DIR}/cmake")
set(install_dir "share/${info_binary_id}")
set(module_install_dir ${install_dir})
set(share_install_dir "${install_dir}/share")
set(lib_install_dir "${install_dir}/lib")
set(bin_install_dir "${install_dir}/bin")
set(rpath_install_dir "${CMAKE_INSTALL_PREFIX}/${lib_install_dir}")
set(resources_dir "${PROJECT_SOURCE_DIR}/resources")
set(resources_install_dir ${install_dir})
set(translations_resource_file "${CMAKE_BINARY_DIR}/translations.qrc")

set(xz_path "xz")
if(WITH_FLATPAK)
    set(rpath_install_dir "${CMAKE_INSTALL_PREFIX}/lib")
elseif(BUILD_XZ)
    set(xz_path "${external_bin_dir}/xz")
endif()

set(deps "") # dependencies of dsnote_lib and dsnote
set(deps_libs "") # libs to link to dsnote_lib and dsnote

# global config
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_SKIP_RPATH ON)
set(ENV{PKG_CONFIG_PATH} $ENV{PKG_CONFIG_PATH}:${PROJECT_BINARY_DIR})
message(STATUS "PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")

# includes

set(includes ${sources_dir} ${PROJECT_BINARY_DIR} ${external_include_dir})

# dsnote_lib

set(dsnote_lib_sources
    ${sources_dir}/audio_source.h
    ${sources_dir}/dbus_speech_adaptor.cpp
    ${sources_dir}/dbus_speech_adaptor.h
    ${sources_dir}/dbus_speech_inf.cpp
    ${sources_dir}/dbus_speech_inf.h
    ${sources_dir}/dbus_application_adaptor.cpp
    ${sources_dir}/dbus_application_adaptor.h
    ${sources_dir}/dbus_application_inf.cpp
    ${sources_dir}/dbus_application_inf.h
    ${sources_dir}/dbus_notifications_inf.cpp
    ${sources_dir}/dbus_notifications_inf.h
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
    ${sources_dir}/module_tools.hpp
    ${sources_dir}/module_tools.cpp
    ${sources_dir}/tts_engine.hpp
    ${sources_dir}/tts_engine.cpp
    ${sources_dir}/piper_engine.hpp
    ${sources_dir}/piper_engine.cpp
    ${sources_dir}/coqui_engine.hpp
    ${sources_dir}/coqui_engine.cpp
    ${sources_dir}/espeak_engine.hpp
    ${sources_dir}/espeak_engine.cpp
    ${sources_dir}/rhvoice_engine.hpp
    ${sources_dir}/rhvoice_engine.cpp
    ${sources_dir}/nlohmann/json.hpp
    ${sources_dir}/simdjson.h
    ${sources_dir}/simdjson.cpp
    ${sources_dir}/py_executor.hpp
    ${sources_dir}/py_executor.cpp
    ${sources_dir}/text_tools.hpp
    ${sources_dir}/text_tools.cpp
    ${sources_dir}/mnt_engine.hpp
    ${sources_dir}/mnt_engine.cpp
    ${sources_dir}/gpu_tools.hpp
    ${sources_dir}/gpu_tools.cpp
    ${sources_dir}/astrunc/astrunc.h
    ${sources_dir}/media_compressor.hpp
    ${sources_dir}/media_compressor.cpp
    ${sources_dir}/mtag_tools.hpp
    ${sources_dir}/mtag_tools.cpp
    ${sources_dir}/app_server.hpp
    ${sources_dir}/app_server.cpp
    ${sources_dir}/fasterwhisper_engine.hpp
    ${sources_dir}/fasterwhisper_engine.cpp
    ${sources_dir}/mimic3_engine.hpp
    ${sources_dir}/mimic3_engine.cpp
    ${sources_dir}/april_engine.hpp
    ${sources_dir}/april_engine.cpp
    ${sources_dir}/avlogger.hpp
    ${sources_dir}/avlogger.cpp
    ${sources_dir}/downloader.hpp
    ${sources_dir}/downloader.cpp
    ${sources_dir}/recorder.hpp
    ${sources_dir}/recorder.cpp
    ${sources_dir}/media_converter.hpp
    ${sources_dir}/media_converter.cpp
    ${sources_dir}/text_repair_engine.hpp
    ${sources_dir}/text_repair_engine.cpp
    ${sources_dir}/whisperspeech_engine.hpp
    ${sources_dir}/whisperspeech_engine.cpp
    ${sources_dir}/audio_device_manager.cpp
    ${sources_dir}/audio_device_manager.hpp
    ${sources_dir}/cmd_options.hpp
    ${sources_dir}/sam_engine.cpp
    ${sources_dir}/sam_engine.hpp
    ${sources_dir}/parler_engine.cpp
    ${sources_dir}/parler_engine.hpp
    ${sources_dir}/f5_engine.cpp
    ${sources_dir}/f5_engine.hpp
    ${sources_dir}/kokoro_engine.cpp
    ${sources_dir}/kokoro_engine.hpp
)

if(WITH_DESKTOP)
    list(APPEND dsnote_lib_sources
        ${sources_dir}/tray_icon.hpp
        ${sources_dir}/tray_icon.cpp
        ${sources_dir}/fake_keyboard.hpp
        ${sources_dir}/fake_keyboard.cpp
        ${sources_dir}/global_hotkeys_manager.cpp
        ${sources_dir}/global_hotkeys_manager.hpp
        ${sources_dir}/dbus_portal_globalshortcuts_inf.cpp
        ${sources_dir}/dbus_portal_globalshortcuts_inf.h
        ${sources_dir}/dbus_portal_request_inf.cpp
        ${sources_dir}/dbus_portal_request_inf.h)
endif()

configure_file(config.h.in config.h)

include(${cmake_path}/dbus_api.cmake)

add_library(dsnote_lib STATIC ${dsnote_lib_sources})

# resources

set(resources "")
list(APPEND resources
    "${desktop_dir}/resources.qrc"
    ${translations_resource_file})

# executable

add_executable(${info_binary_id} ${resources} "${sources_dir}/main.cpp")
set_property(TARGET ${info_binary_id} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
set_property(TARGET ${info_binary_id} PROPERTY AUTORCC TRUE)

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
    target_compile_options(compiler_flags INTERFACE "-pie;-Wall;-Wextra;-Wpedantic;-Wno-attributes;-O3;-g")
    target_link_options(compiler_flags INTERFACE "-pie;-flto;-fno-fat-lto-objects")
endif()

set(link_opts "-Wl,--enable-new-dtags;-Wl,-pie;-Wl,-rpath,${rpath_install_dir}")
target_link_options(compiler_flags INTERFACE ${link_opts})

target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_TRACE_LOGS}>:USE_TRACE_LOGS>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_SFOS}>:USE_SFOS>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_DESKTOP}>:USE_DESKTOP>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_PY}>:USE_PY>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_FLATPAK}>:USE_FLATPAK>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${WITH_X11_FEATURES}>:USE_X11_FEATURES>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${BUILD_PYTHON_MODULE}>:USE_PYTHON_MODULE>")
target_compile_definitions(compiler_flags INTERFACE "$<$<CONFIG:Debug>:DEBUG>")
target_compile_definitions(compiler_flags INTERFACE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_x8664}>:ARCH_X86_64>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_arm32}>:ARCH_ARM_32>")
target_compile_definitions(compiler_flags INTERFACE "$<$<BOOL:${arch_arm64}>:ARCH_ARM_64>")
target_compile_definitions(compiler_flags INTERFACE "INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

# translations

include(${cmake_path}/translations.cmake)

# dependencies

list(APPEND deps_libs
    compiler_flags
    pthread
    ${CMAKE_DL_LIBS})

set(deps_dirs "")

include(FindPkgConfig)
include(ExternalProject)

unset(meson_bin CACHE)
find_program(meson_bin meson)
unset(autoconf_bin CACHE)
find_program(autoconf_bin autoconf)
unset(automake_bin CACHE)
find_program(automake_bin automake)
unset(libtool_bin CACHE)
find_program(libtool_bin libtool)
unset(patchelf_bin CACHE)
find_program(patchelf_bin patchelf)

pkg_search_module(zlib REQUIRED zlib)
list(APPEND deps_libs ${zlib_LIBRARIES})
list(APPEND includes ${zlib_INCLUDE_DIRS})

pkg_search_module(pulse REQUIRED libpulse)
list(APPEND deps_libs ${pulse_LIBRARIES})
list(APPEND includes ${pulse_INCLUDE_DIRS})

find_package(Qt5 COMPONENTS Core Network Multimedia Qml Xml Sql Gui Quick DBus LinguistTools REQUIRED)
list(APPEND deps_libs Qt5::Core Qt5::Network Qt5::Multimedia Qt5::Gui Qt5::Quick Qt5::DBus Qt5::Xml)

if(WITH_DESKTOP)
    find_package(Qt5 COMPONENTS QuickControls2 Widgets REQUIRED)
    list(APPEND deps_libs Qt5::QuickControls2 Qt5::Widgets)

    pkg_search_module(wayland REQUIRED wayland-client)
    list(APPEND deps_libs ${wayland_LIBRARIES})
    list(APPEND includes ${wayland_INCLUDE_DIRS})
    include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

    if(BUILD_XKBCOMMON)
        include(${cmake_path}/xkbcommon.cmake)
    else()
        pkg_search_module(xkbcommon REQUIRED xkbcommon>=1.6.0)
        list(APPEND deps_libs ${xkbcommon_LIBRARIES})
        list(APPEND includes ${xkbcommon_INCLUDE_DIRS})

        pkg_search_module(xkbcommonx11 REQUIRED xkbcommon-x11>=1.6.0)
        list(APPEND deps_libs ${xkbcommonx11_LIBRARIES})
        list(APPEND includes ${xkbcommonx11_INCLUDE_DIRS})
    endif()

    if(WITH_X11_FEATURES)
        find_package(X11 REQUIRED)
        list(APPEND deps_libs X11)

        find_package(Qt5 COMPONENTS X11Extras REQUIRED)
        list(APPEND deps_libs Qt5::X11Extras)

        if(BUILD_QHOTKEY)
            include(${cmake_path}/qhotkey.cmake)
        else()
            find_library(qhotkey_path qhotkey REQUIRED)
            list(APPEND deps_libs ${qhotkey_path})
        endif()

        if(BUILD_XDO)
            include(${cmake_path}/xdo.cmake)
        else()
            pkg_search_module(xdo REQUIRED libxdo)
            list(APPEND deps_libs ${xdo_LIBRARIES})
            list(APPEND includes ${xdo_INCLUDE_DIRS})
        endif()
    endif()
endif()

if(BUILD_PYTHON_MODULE)
    include(${cmake_path}/python_module.cmake)
endif()

if(BUILD_RNNOISE)
    include(${cmake_path}/rnnoise.cmake)
else()
    find_library(rnnoise_path rnnoise-nu REQUIRED)
    list(APPEND deps_libs ${rnnoise_path})
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

if(${BUILD_VOSK} OR ${DOWNLOAD_VOSK})
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
endif()

if(BUILD_WEBRTCVAD)
    include(${cmake_path}/webrtcvad.cmake)
else()
    find_library(webrtcvad_path vad REQUIRED)
    list(APPEND deps_libs ${webrtcvad_path})
endif()

if(BUILD_XZ)
    include(${cmake_path}/xz.cmake)
else()
    pkg_search_module(xz REQUIRED liblzma>=5.4.1)
    list(APPEND deps_libs ${xz_LIBRARIES})
    list(APPEND includes ${xz_INCLUDE_DIRS})
endif()

if(BUILD_RHVOICE)
    include(${cmake_path}/rhvoice.cmake)
endif()

if(BUILD_RHVOICE_MODULE)
    include(${cmake_path}/rhvoice_module.cmake)
endif()

if(BUILD_PIPER OR BUILD_ESPEAK)
    include(${cmake_path}/espeak.cmake)
else()
    pkg_search_module(espeak REQUIRED espeak-ng)
    list(APPEND deps_libs ${espeak_LIBRARIES})
    list(APPEND includes ${espeak_INCLUDE_DIRS})
endif()

if(BUILD_ESPEAK_MODULE)
    include(${cmake_path}/espeak_module.cmake)
endif()

if(BUILD_PIPER)
    include(${cmake_path}/piper.cmake)
else()
    pkg_search_module(spdlog_path REQUIRED spdlog)
    list(APPEND deps_libs ${spdlog_path_LIBRARIES})
    list(APPEND includes ${spdlog_path_INCLUDE_DIRS})

    find_library(piper_path piper_api REQUIRED)
    find_library(piperphonemize_path piper_phonemize REQUIRED)
    find_library(onnxruntime_path onnxruntime REQUIRED)
    list(APPEND deps_libs ${piper_path} ${piperphonemize_path} ${onnxruntime_path})
endif()

if(BUILD_SSPLITCPP)
    include(${cmake_path}/ssplitcpp.cmake)
else()
    find_library(ssplitcpp_path ssplit REQUIRED)
    find_library(pcre2_path pcre2-8 REQUIRED)
    find_library(pcre2posix_path pcre2-posix REQUIRED)
    list(APPEND deps_libs ${ssplitcpp_path} ${pcre2_path} ${pcre2posix_path})
endif()

if(${DEV_USE_PYTHON311} OR ${DEV_USE_PYTHON312})
    if(DEV_USE_PYTHON311)
        pkg_search_module(python REQUIRED python-3.11)
        set(python_LIBRARIES -lpython3.11)
    else()
        pkg_search_module(python REQUIRED python-3.12)
        set(python_LIBRARIES -lpython3.12)
    endif()
    list(APPEND deps_libs ${python_LIBRARIES})
    list(APPEND includes ${python_INCLUDE_DIRS})
else()
    find_package(Python COMPONENTS Development REQUIRED)
    list(APPEND deps_dirs ${Python_LIBRARY_DIRS})
    list(APPEND deps_libs ${Python_LIBRARIES})
    list(APPEND includes ${Python_INCLUDE_DIRS})
endif()

if(BUILD_PYBIND11)
    include(${cmake_path}/pybind11.cmake)
else()
    pkg_search_module(pybind11 REQUIRED pybind11)
    list(APPEND deps_libs ${pybind11_LIBRARIES})
    list(APPEND includes ${pybind11_INCLUDE_DIRS})
endif()

if(BUILD_BERGAMOT)
    include(${cmake_path}/bergamot.cmake)
endif()

if(BUILD_OPENBLAS)
    include(${cmake_path}/openblas.cmake)
else()
    include(${cmake_path}/openblas_pkgconfig.cmake)
endif()

if(BUILD_APRILASR)
    include(${cmake_path}/aprilasr.cmake)
else()
    pkg_search_module(aprilasr REQUIRED april-asr)
    list(APPEND deps_libs ${aprilasr_LIBRARIES})
    list(APPEND includes ${aprilasr_INCLUDE_DIRS})
endif()

if(BUILD_RUBBERBAND)
    include(${cmake_path}/rubberband.cmake)
else()
    pkg_search_module(rubberband REQUIRED rubberband)
    list(APPEND deps_libs ${rubberband_LIBRARIES})
    list(APPEND includes ${rubberband_INCLUDE_DIRS})
endif()

if(BUILD_FFMPEG)
    include(${cmake_path}/ffmpeg.cmake)
else()
    find_library(lame_path mp3lame)
    list(APPEND deps_libs ${lame_path})

    pkg_search_module(ogg REQUIRED ogg)
    list(APPEND deps_libs ${ogg_LIBRARIES})
    list(APPEND includes ${ogg_INCLUDE_DIRS})

    pkg_search_module(vorbis REQUIRED vorbis)
    list(APPEND deps_libs ${vorbis_LIBRARIES})
    list(APPEND includes ${vorbis_INCLUDE_DIRS})

    pkg_search_module(vorbisenc REQUIRED vorbisenc)
    list(APPEND deps_libs ${vorbisenc_LIBRARIES})
    list(APPEND includes ${vorbisenc_INCLUDE_DIRS})

    pkg_search_module(ffmpeg_avutil REQUIRED libavutil>=57.17.100)
    pkg_search_module(ffmpeg_avcodec REQUIRED libavcodec>=59.18.100)
    pkg_search_module(ffmpeg_avformat REQUIRED libavformat>=59.16.100)
    pkg_search_module(ffmpeg_avdevice REQUIRED libavdevice>=59.7.100)
    pkg_search_module(ffmpeg_avfilter REQUIRED libavfilter>=8.44.100)
    pkg_search_module(ffmpeg_swresample REQUIRED libswresample>=4.3.100)
    pkg_search_module(ffmpeg_swscale REQUIRED libswscale>=6.7.100)

    string(REGEX REPLACE "^([0-9]+).*$" "\\1" ffmpeg_ver ${ffmpeg_avformat_VERSION})
    message(STATUS "found ffmpeg version: ${ffmpeg_ver}")

    list(APPEND deps_libs
        ${ffmpeg_avdevice_LIBRARIES}
        ${ffmpeg_avfilter_LIBRARIES}
        ${ffmpeg_avformat_LIBRARIES}
        ${ffmpeg_avcodec_LIBRARIES}
        ${ffmpeg_swresample_LIBRARIES}
        ${ffmpeg_swscale_LIBRARIES}
        ${ffmpeg_avutil_LIBRARIES})
    list(APPEND includes
        ${ffmpeg_avutil_INCLUDE_DIRS}
        ${ffmpeg_avcodec_INCLUDE_DIRS}
        ${ffmpeg_avformat_INCLUDE_DIRS}
        ${ffmpeg_avdevice_INCLUDE_DIRS}
        ${ffmpeg_avfilter_INCLUDE_DIRS}
        ${ffmpeg_swresample_INCLUDE_DIRS}
        ${ffmpeg_swscale_INCLUDE_DIRS})
endif()

if(BUILD_TAGLIB)
    include(${cmake_path}/taglib.cmake)
else()
    pkg_search_module(taglib REQUIRED taglib)
    list(APPEND deps_libs ${taglib_LIBRARIES})
    list(APPEND includes ${taglib_INCLUDE_DIRS})
endif()

if(BUILD_LIBNUMBERTEXT)
    include(${cmake_path}/libnumbertext.cmake)
else()
    pkg_search_module(libnumbertext REQUIRED libnumbertext)
    list(APPEND deps_libs ${libnumbertext_LIBRARIES})
    list(APPEND includes ${libnumbertext_INCLUDE_DIRS})
endif()

if(BUILD_HTML2MD)
    include(${cmake_path}/html2md.cmake)
else()
    pkg_search_module(html2md REQUIRED html2md)
    list(APPEND deps_libs ${html2md_LIBRARIES})
    list(APPEND includes ${html2md_INCLUDE_DIRS})
endif()

if(BUILD_MADDY)
    include(${cmake_path}/maddy.cmake)
endif()

if(BUILD_SAM)
    include(${cmake_path}/sam.cmake)
else()
    find_library(sam_path sam_api REQUIRED)
    list(APPEND deps_libs ${sam_path})
endif()

if(WITH_SFOS)
    pkg_search_module(sailfish REQUIRED sailfishapp)
    list(APPEND deps_libs ${sailfish_LIBRARIES})
    list(APPEND includes ${sailfish_INCLUDE_DIRS})

    pkg_search_module(mlite5 REQUIRED mlite5)
    list(APPEND deps_libs ${mlite5_LIBRARIES})
    list(APPEND includes ${mlite5_INCLUDE_DIRS})
endif()

if(BUILD_QQC2_BREEZE_STYLE)
    include(${cmake_path}/qqc2-breeze-style.cmake)
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

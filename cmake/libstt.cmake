set(libstt_x8664_url "https://github.com/coqui-ai/STT/releases/download/v1.1.0/native_client.tflite.Linux.tar.xz")
set(libstt_x8664_checksum "a6965f80cbfc92e403b2e4ab92e13650c76f131cbefb951b314df046c9e11799")
set(libstt_arm64_url "https://github.com/coqui-ai/STT/releases/download/v1.1.0/native_client.tflite.linux.aarch64.tar.xz")
set(libstt_arm64_checksum "5c8ce5e4bb2d8fa55f5a6ac58103d9aee6a080369bc99cd5910d2615783e1ba3")
set(libstt_arm32_url "https://github.com/mkiol/dsnote/releases/download/v3.1.4.1/native_client.tflite.linux.armv7.tar.xz")
set(libstt_arm32_checksum "07ebbe9819c1eb4051ecdfa596d11b726a4ccb52479ea64e78622dd4d61526e1")

if(arch_x8664)
    set(libstt_url ${libstt_x8664_url})
    set(libstt_checksum ${libstt_x8664_checksum})
elseif(arch_arm32)
    set(libstt_url ${libstt_arm32_url})
    set(libstt_checksum ${libstt_arm32_checksum})
elseif(arch_arm64)
    set(libstt_url ${libstt_arm64_url})
    set(libstt_checksum ${libstt_arm64_checksum})
endif()

set(libstt_archive "${PROJECT_BINARY_DIR}/libstt.tar.xz")

file(DOWNLOAD ${libstt_url} ${libstt_archive}
    EXPECTED_HASH SHA256=${libstt_checksum}
    SHOW_PROGRESS
    STATUS libstt_status)
list(GET libstt_status 0 libstt_status_code)
list(GET libstt_status 1 libstt_status_message)
if(NOT libstt_status_code EQUAL 0)
    message(FATAL_ERROR "libstt download failed: ${libstt_status_message}")
endif()
message(STATUS "libstt download status: ${libstt_status_message}")

file(ARCHIVE_EXTRACT INPUT ${libstt_archive} DESTINATION ${external_lib_dir} PATTERNS *.so VERBOSE)
file(CHMOD_RECURSE ${external_lib_dir} FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

add_library(stt SHARED IMPORTED)
set_property(TARGET stt PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libstt.so)
list(APPEND deps stt)

add_library(kenlm SHARED IMPORTED)
set_property(TARGET kenlm PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libkenlm.so)
list(APPEND deps kenlm)

add_library(tensorflowlite SHARED IMPORTED)
set_property(TARGET tensorflowlite PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libtensorflowlite.so)
list(APPEND deps tensorflowlite)

add_library(tflitedelegates SHARED IMPORTED)
set_property(TARGET tflitedelegates PROPERTY IMPORTED_LOCATION ${external_lib_dir}/libtflitedelegates.so)
list(APPEND deps tflitedelegates)

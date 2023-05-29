if(BUILD_RHVOICE)
    set(espeak_data_dir ${external_share_dir})
else()
    set(espeak_data_dir ${CMAKE_INSTALL_PREFIX}/share)
endif()

add_custom_command(
  OUTPUT espeakdata.tar.xz
  COMMAND sh -c "${tools_dir}/make_espeakdata_module.sh ${espeak_data_dir}/espeak-ng-data ${CMAKE_BINARY_DIR}/external/espeakdata_module ${PROJECT_BINARY_DIR}/espeakdata.tar.xz ${xz_path}"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_library(espeakdata_module STATIC "${CMAKE_BINARY_DIR}/espeakdata.tar.xz")

list(APPEND deps espeakdata_module)

if(BUILD_RHVOICE)
    add_dependencies(espeakdata_module espeak)
endif()

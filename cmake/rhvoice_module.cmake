if(BUILD_RHVOICE)
    set(rhvoice_data_dir ${external_dir}/rhvoice)
else()
    set(rhvoice_data_dir ${CMAKE_INSTALL_PREFIX}/share/rhvoice)
endif()

add_custom_command(
  OUTPUT rhvoicedata.tar.xz
  COMMAND sh -c "${tools_dir}/make_rhvoicedata_module.sh ${rhvoice_data_dir}/data ${CMAKE_BINARY_DIR}/external/rhvoicedata_module ${PROJECT_BINARY_DIR}/rhvoicedata.tar.xz ${xz_path}"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_library(rhvoicedata_module STATIC "${CMAKE_BINARY_DIR}/rhvoicedata.tar.xz")

add_custom_command(
  OUTPUT rhvoiceconfig.tar.xz
  COMMAND sh -c "${tools_dir}/make_rhvoiceconfig_module.sh ${rhvoice_data_dir}/config ${CMAKE_BINARY_DIR}/external/rhvoiceconfig_module ${PROJECT_BINARY_DIR}/rhvoiceconfig.tar.xz ${xz_path}"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_library(rhvoiceconfig_module STATIC "${CMAKE_BINARY_DIR}/rhvoiceconfig.tar.xz")

list(APPEND deps rhvoicedata_module rhvoiceconfig_module)

if(BUILD_RHVOICE)
    add_dependencies(rhvoicedata_module rhvoice)
    add_dependencies(rhvoiceconfig_module rhvoice)
endif()

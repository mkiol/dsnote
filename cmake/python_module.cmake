add_custom_command(
  OUTPUT python.tar.xz
  COMMAND sh -c "${tools_dir}/make_python_module.sh ${CMAKE_BINARY_DIR}/external/python_module ${PROJECT_BINARY_DIR}/python.tar.xz ${external_bin_dir}/xz"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_library(python_module STATIC "${CMAKE_BINARY_DIR}/python.tar.xz")

list(APPEND deps python_module)

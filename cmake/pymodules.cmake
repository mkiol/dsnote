set(py_ver "3.10")

if(WITH_SFOS)
    set(py_ver "3.8")
endif()

add_custom_command(
  OUTPUT python.tar.xz
  COMMAND sh -c "${tools_dir}/make_pymodules.sh ${CMAKE_BINARY_DIR}/external/pymodules ${py_ver} ${PROJECT_BINARY_DIR}/python.tar.xz ${external_bin_dir}/xz"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_library(pymodules STATIC "${CMAKE_BINARY_DIR}/python.tar.xz")

list(APPEND deps pymodules)

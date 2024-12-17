set(dbus_dsnote_interface_file "${PROJECT_BINARY_DIR}/${info_dbus_app_interface}.xml")

configure_file(${dbus_dir}/dsnote.xml.in ${dbus_dsnote_interface_file})

find_package(Qt5 COMPONENTS DBus REQUIRED)

unset(qdbusxml2cpp_bin CACHE)
find_program(qdbusxml2cpp_bin qdbusxml2cpp)
if(${qdbusxml2cpp_bin} MATCHES "-NOTFOUND$")
   find_program(qdbusxml2cpp_bin qdbusxml2cpp-qt5)
   if(${qdbusxml2cpp_bin} MATCHES "-NOTFOUND$")
      message(FATAL_ERROR "qdbusxml2cpp not found but it is required")
   endif()
endif()

add_custom_command(
    OUTPUT dbus_dsnote_adaptor.h dbus_dsnote_adaptor.cpp
    COMMAND ${qdbusxml2cpp_bin} ${info_dbus_app_interface}.xml -m -a dbus_dsnote_adaptor -c DsnoteAdaptor
    DEPENDS ${info_dbus_app_interface}.xml
    COMMENT "generate dbus app adaptor sources"
)
qt5_generate_moc("${PROJECT_BINARY_DIR}/dbus_dsnote_adaptor.h" dbus_dsnote_adaptor.moc)
macro_add_file_dependencies(dbus_dsnote_adaptor.cpp dbus_dsnote_adaptor.moc)

add_custom_command(
    OUTPUT dbus_dsnote_inf.h dbus_dsnote_inf.cpp
    COMMAND ${qdbusxml2cpp_bin} ${info_dbus_app_interface}.xml -m -p dbus_dsnote_inf -c DsnoteDbusInterface
    DEPENDS ${info_dbus_app_interface}.xml
    COMMENT "generate dbus app inf sources"
)
qt5_generate_moc("${PROJECT_BINARY_DIR}/dbus_dsnote_inf.h" dbus_dsnote_inf.moc)
macro_add_file_dependencies(dbus_dsnote_inf.cpp dbus_dsnote_inf.moc)

list(APPEND dsnote_lib_sources
    dbus_dsnote_adaptor.cpp dbus_dsnote_adaptor.h
    dbus_dsnote_inf.cpp dbus_dsnote_inf.h)

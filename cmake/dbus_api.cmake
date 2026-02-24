set(dbus_dsnote_interface_file "${PROJECT_BINARY_DIR}/${info_dbus_app_interface}.xml")

configure_file(${dbus_dir}/dsnote.xml.in ${dbus_dsnote_interface_file})

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)

add_custom_command(
    OUTPUT dbus_dsnote_adaptor.h dbus_dsnote_adaptor.cpp
    COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp ${info_dbus_app_interface}.xml -a dbus_dsnote_adaptor -c DsnoteAdaptor
    DEPENDS ${info_dbus_app_interface}.xml ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp
    COMMENT "generate dbus app adaptor sources"
)
add_custom_command(
    OUTPUT dbus_dsnote_inf.h dbus_dsnote_inf.cpp
    COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp ${info_dbus_app_interface}.xml -p dbus_dsnote_inf -c DsnoteDbusInterface
    DEPENDS ${info_dbus_app_interface}.xml ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp
    COMMENT "generate dbus app inf sources"
)

list(APPEND dsnote_lib_sources
    dbus_dsnote_adaptor.cpp dbus_dsnote_adaptor.h
    dbus_dsnote_inf.cpp dbus_dsnote_inf.h)

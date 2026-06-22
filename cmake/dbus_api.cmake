set(dbus_dsnote_interface_file "${PROJECT_BINARY_DIR}/${info_dbus_app_interface}.xml")

configure_file(${dbus_dir}/dsnote.xml.in ${dbus_dsnote_interface_file})

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)

set(qdbusxml2cpp_target Qt${QT_VERSION_MAJOR}::qdbusxml2cpp)
if(TARGET ${qdbusxml2cpp_target})
    set(qdbusxml2cpp_bin ${qdbusxml2cpp_target})
elseif(DEFINED QT_CMAKE_EXPORT_NAMESPACE AND
       TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp)
    set(qdbusxml2cpp_bin ${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp)
else()
    unset(qdbusxml2cpp_bin CACHE)
    find_program(qdbusxml2cpp_bin
        NAMES qdbusxml2cpp qdbusxml2cpp-qt5
        HINTS
            "/usr/lib64/qt${QT_VERSION_MAJOR}/bin"
            "/usr/lib/qt${QT_VERSION_MAJOR}/bin"
            "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/qt${QT_VERSION_MAJOR}/bin")
    if(NOT qdbusxml2cpp_bin)
       message(FATAL_ERROR "qdbusxml2cpp not found but it is required")
    endif()
endif()

add_custom_command(
    OUTPUT dbus_dsnote_adaptor.h dbus_dsnote_adaptor.cpp
    COMMAND ${qdbusxml2cpp_bin} ${info_dbus_app_interface}.xml -a dbus_dsnote_adaptor -c DsnoteAdaptor
    DEPENDS ${info_dbus_app_interface}.xml ${qdbusxml2cpp_bin}
    COMMENT "generate dbus app adaptor sources"
)
add_custom_command(
    OUTPUT dbus_dsnote_inf.h dbus_dsnote_inf.cpp
    COMMAND ${qdbusxml2cpp_bin} ${info_dbus_app_interface}.xml -p dbus_dsnote_inf -c DsnoteDbusInterface
    DEPENDS ${info_dbus_app_interface}.xml ${qdbusxml2cpp_bin}
    COMMENT "generate dbus app inf sources"
)

list(APPEND dsnote_lib_sources
    dbus_dsnote_adaptor.cpp dbus_dsnote_adaptor.h
    dbus_dsnote_inf.cpp dbus_dsnote_inf.h)

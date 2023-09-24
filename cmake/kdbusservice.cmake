set(KDE_INSTALL_LIBDIR ${external_lib_dir})

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDECMakeSettings)

find_package(KF5 COMPONENTS DBusAddons REQUIRED)

list(APPEND deps_libs KF5::DBusAddons)

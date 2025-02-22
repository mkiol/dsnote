set(xkbcommon_source_url "https://xkbcommon.org/download/libxkbcommon-1.7.0.tar.xz")
set(xkbcommon_checksum "65782f0a10a4b455af9c6baab7040e2f537520caa2ec2092805cdfd36863b247")

if(${meson_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "meson not found but it is required to build xkbcommon")
endif()

set(DSNOTE_XKB_CONFIG_ROOT "/usr/share/X11/xkb" CACHE PATH "Path of xkbcommon config root")

ExternalProject_Add(xkbcommon
    SOURCE_DIR ${external_dir}/xkbcommon
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/xkbcommon
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL ${xkbcommon_source_url}
    URL_HASH SHA256=${xkbcommon_checksum}
    CONFIGURE_COMMAND ${meson_bin} setup --prefix=<INSTALL_DIR> --buildtype=release --libdir=lib
        -Denable-wayland=true
        -Denable-tools=false
        -Denable-x11=$<IF:$<BOOL:${WITH_X11_FEATURES}>,true,false>
        -Dxkb-config-root=${DSNOTE_XKB_CONFIG_ROOT}
        -Denable-bash-completion=false
        -Ddefault_library=static
        <BINARY_DIR> <SOURCE_DIR>
    BUILD_COMMAND ninja -C <BINARY_DIR>
    BUILD_ALWAYS False
    INSTALL_COMMAND ninja -C <BINARY_DIR> install
)

list(APPEND deps_libs "${external_lib_dir}/libxkbcommon.a")
list(APPEND deps xkbcommon)

if(WITH_X11_FEATURES)
    list(APPEND deps_libs xcb xcb-xkb "${external_lib_dir}/libxkbcommon-x11.a")
endif()

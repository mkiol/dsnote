set(wl_clipboard_source_url "https://github.com/bugaevc/wl-clipboard/archive/refs/tags/v2.2.1.tar.gz")
set(wl_clipboard_checksum "6eb8081207fb5581d1d82c4bcd9587205a31a3d47bea3ebeb7f41aa1143783eb")

if(${meson_bin} MATCHES "-NOTFOUND$")
   message(FATAL_ERROR "meson not found but it is required to build wl-clipboard")
endif()

ExternalProject_Add(wl_clipboard
    SOURCE_DIR ${external_dir}/wl_clipboard
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/wl_clipboard
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external/
    URL ${wl_clipboard_source_url}
    URL_HASH SHA256=${wl_clipboard_checksum}
    CONFIGURE_COMMAND ${meson_bin} setup --prefix=<INSTALL_DIR> --buildtype=release --libdir=<SOURCE_DIR>/lib-dir 
        --datadir=<SOURCE_DIR>/data-dir  -Dfishcompletiondir=<SOURCE_DIR>/completions
        <BINARY_DIR> <SOURCE_DIR>
    BUILD_COMMAND ninja -C <BINARY_DIR>
    BUILD_ALWAYS False
    INSTALL_COMMAND ninja -C <BINARY_DIR> install
)
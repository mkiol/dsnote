set(ffmpeg_source_url "https://ffmpeg.org/releases/ffmpeg-6.0.tar.xz")
set(ffmpeg_checksum "57be87c22d9b49c112b6d24bc67d42508660e6b718b3db89c44e47e289137082")

set(lame_source_url "https://altushost-swe.dl.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz")
set(lame_checksum "ddfe36cab873794038ae2c1210557ad34857a4b6bdc515785d1da9e175b1da1e")

set(nasm_source_url "https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.gz")
set(nasm_checksum "9182a118244b058651c576baa9d0366ee05983c4d4ae1d9ddd3236a9f2304997")

ExternalProject_Add(nasm
    SOURCE_DIR ${external_dir}/nasm
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/nasm
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${nasm_source_url}"
    URL_HASH SHA256=${nasm_checksum}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

ExternalProject_Add(lame
    SOURCE_DIR ${external_dir}/lame
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/lame
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${lame_source_url}"
    URL_HASH SHA256=${lame_checksum}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --bindir=<INSTALL_DIR>/bin
        --enable-static --enable-nasm --disable-decoder --disable-analyzer-hooks
        --disable-frontend --with-pic=yes
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

set(ffmpeg_opts
    --disable-autodetect
    --disable-doc
    --disable-programs
    --disable-everything
    --enable-static
    --disable-shared
    --enable-nonfree
    --enable-gpl
    --enable-pic
    --enable-protocol=file
    --enable-filter=dynaudnorm
    --enable-filter=aresample
    --enable-filter=aformat
    --enable-encoder=libmp3lame
    --enable-decoder=pcm_u8
    --enable-decoder=pcm_u32le
    --enable-decoder=pcm_u32be
    --enable-decoder=pcm_u24le
    --enable-decoder=pcm_u24be
    --enable-decoder=pcm_u16le
    --enable-decoder=pcm_u16be
    --enable-decoder=pcm_s8
    --enable-decoder=pcm_s32le
    --enable-decoder=pcm_s32be
    --enable-decoder=pcm_s24le
    --enable-decoder=pcm_s24be
    --enable-decoder=pcm_s16le
    --enable-decoder=pcm_s16be
    --enable-decoder=pcm_f64le
    --enable-decoder=pcm_f64be
    --enable-decoder=pcm_f32le
    --enable-decoder=pcm_f32be
    --enable-decoder=aac
    --enable-decoder=aac_fixed
    --enable-decoder=aac_latm
    --enable-decoder=mp3
    --enable-decoder=mp3adu
    --enable-decoder=mp3adufloat
    --enable-decoder=mp3float
    --enable-decoder=mp3on4
    --enable-decoder=mp3on4float
    --enable-muxer=mp3
    --enable-demuxer=mpegts
    --enable-demuxer=aac
    --enable-demuxer=mp3
    --enable-demuxer=mov
    --enable-demuxer=ogg
    --enable-demuxer=matroska
    --enable-demuxer=flac
    --enable-demuxer=wav
    --enable-demuxer=mpegvideo
    --enable-parser=h264
    --enable-parser=aac
    --enable-parser=aac_latm
    --enable-parser=ac3
    --enable-libmp3lame)

ExternalProject_Add(ffmpeg
    SOURCE_DIR ${external_dir}/ffmpeg
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/ffmpeg
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${ffmpeg_source_url}"
    URL_HASH SHA256=${ffmpeg_checksum}
    CONFIGURE_COMMAND CPATH=${external_include_dir}
        LIBRARY_PATH=${external_lib_dir}
        PATH=$ENV{PATH}:${external_bin_dir} PKG_CONFIG_PATH=${external_lib_dir}/pkgconfig
        <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> ${ffmpeg_opts} --extra-ldflags=-L${external_lib_dir}
    BUILD_COMMAND CPATH=${external_include_dir}
        LIBRARY_PATH=${external_lib_dir}
        PATH=$ENV{PATH}:${external_bin_dir} ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND CPATH=${external_include_dir}
        LIBRARY_PATH=${external_lib_dir}
        PATH=$ENV{PATH}:${external_bin_dir} make DESTDIR=/ install
)

ExternalProject_Add_StepDependencies(ffmpeg configure nasm)
ExternalProject_Add_StepDependencies(lame configure nasm)
ExternalProject_Add_StepDependencies(ffmpeg configure lame)

list(APPEND deps_libs
    ${external_lib_dir}/libmp3lame.a
    ${external_lib_dir}/libavfilter.a
    ${external_lib_dir}/libavdevice.a
    ${external_lib_dir}/libavformat.a
    ${external_lib_dir}/libavcodec.a
    ${external_lib_dir}/libswresample.a
    ${external_lib_dir}/libswscale.a
    ${external_lib_dir}/libavutil.a)
list(APPEND deps ffmpeg lame)

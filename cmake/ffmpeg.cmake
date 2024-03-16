set(ffmpeg_source_url "https://ffmpeg.org/releases/ffmpeg-6.1.1.tar.xz")
set(ffmpeg_checksum "8684f4b00f94b85461884c3719382f1261f0d9eb3d59640a1f4ac0873616f968")

set(lame_source_url "https://altushost-swe.dl.sourceforge.net/project/lame/lame/3.100/lame-3.100.tar.gz")
set(lame_checksum "ddfe36cab873794038ae2c1210557ad34857a4b6bdc515785d1da9e175b1da1e")

set(ogg_source_url "https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.xz")
set(ogg_checksum "c4d91be36fc8e54deae7575241e03f4211eb102afb3fc0775fbbc1b740016705")

set(vorbis_source_url "https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-1.3.7.tar.xz")
set(vorbis_checksum "b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b")

set(nasm_source_url "https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.gz")
set(nasm_checksum "9182a118244b058651c576baa9d0366ee05983c4d4ae1d9ddd3236a9f2304997")

set(opus_source_url "https://downloads.xiph.org/releases/opus/opus-1.4.tar.gz")
set(opus_checksum "c9b32b4253be5ae63d1ff16eea06b94b5f0f2951b7a02aceef58e3a3ce49c51f")

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
        --enable-static=true --enable-shared=false
        --enable-nasm --disable-decoder --disable-analyzer-hooks
        --disable-frontend --with-pic=yes
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

ExternalProject_Add(ogg
    SOURCE_DIR ${external_dir}/ogg
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/ogg
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${ogg_source_url}"
    URL_HASH SHA256=${ogg_checksum}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --bindir=<INSTALL_DIR>/bin
        --enable-static=true --enable-shared=false --with-pic=yes
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

ExternalProject_Add(vorbis
    SOURCE_DIR ${external_dir}/vorbis
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/vorbis
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${vorbis_source_url}"
    URL_HASH SHA256=${vorbis_checksum}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --bindir=<INSTALL_DIR>/bin
        --enable-static=true --enable-shared=false
        --disable-oggtest --with-pic=yes
    BUILD_COMMAND ${MAKE}
    BUILD_ALWAYS False
    INSTALL_COMMAND make DESTDIR=/ install
)

ExternalProject_Add(opus
    SOURCE_DIR ${external_dir}/opus
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/opus
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${opus_source_url}"
    URL_HASH SHA256=${opus_checksum}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_INSTALL_LIBDIR=${external_lib_dir}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    BUILD_ALWAYS False
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
    --enable-filter=aresample
    --enable-filter=aformat
    --enable-filter=amix
    --enable-filter=anull
    --enable-filter=volume
    --enable-filter=rubberband
    --enable-encoder=libmp3lame
    --enable-encoder=libvorbis
    --enable-encoder=pcm_s16le
    --enable-encoder=libopus
    --enable-encoder=flac
    --enable-encoder=srt
    --enable-encoder=subrip
    --enable-encoder=ass
    --enable-encoder=ssa
    --enable-encoder=webvtt
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
    --enable-decoder=libvorbis
    --enable-decoder=flac
    --enable-decoder=libopus
    --enable-decoder=speex
    --enable-decoder=ac3
    --enable-decoder=ac3_fixed
    --enable-decoder=eac3
    --enable-decoder=srt
    --enable-decoder=subrip
    --enable-decoder=ass
    --enable-decoder=ssa
    --enable-decoder=webvtt
    --enable-decoder=vplayer
    --enable-decoder=microdvd
    --enable-muxer=mp3
    --enable-muxer=ogg
    --enable-muxer=wav
    --enable-muxer=flac
    --enable-muxer=srt
    --enable-muxer=ass
    --enable-muxer=webvtt
    --enable-demuxer=aac
    --enable-demuxer=mp3
    --enable-demuxer=mov
    --enable-demuxer=ogg
    --enable-demuxer=matroska
    --enable-demuxer=flac
    --enable-demuxer=wav
    --enable-demuxer=mpegvideo
    --enable-demuxer=srt
    --enable-demuxer=ass
    --enable-demuxer=webvtt
    --enable-demuxer=microdvd
    --enable-parser=aac
    --enable-parser=aac_latm
    --enable-parser=ac3
    --enable-libmp3lame
    --enable-libvorbis
    --enable-libopus
    --enable-librubberband)

set(ffmpeg_extra_ldflags -L${external_lib_dir})
set(ffmpeg_extra_libs "-lvorbis -logg -lm")

ExternalProject_Add(ffmpeg
    SOURCE_DIR ${external_dir}/ffmpeg
    BINARY_DIR ${PROJECT_BINARY_DIR}/external/ffmpeg
    INSTALL_DIR ${PROJECT_BINARY_DIR}/external
    URL "${ffmpeg_source_url}"
    URL_HASH SHA256=${ffmpeg_checksum}
    PATCH_COMMAND patch --batch --unified -p1 --directory=<SOURCE_DIR>
                -i ${patches_dir}/ffmpeg.patch ||
                    echo "patch cmd failed, likely already patched"
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND CPATH=${external_include_dir}
        LIBRARY_PATH=${external_lib_dir}
        PATH=$ENV{PATH}:${external_bin_dir} PKG_CONFIG_PATH=${external_lib_dir}/pkgconfig
        <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> ${ffmpeg_opts}
        --extra-ldflags=${ffmpeg_extra_ldflags}
        --extra-libs=${ffmpeg_extra_libs}
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
ExternalProject_Add_StepDependencies(vorbis configure ogg)
ExternalProject_Add_StepDependencies(ffmpeg configure vorbis)
ExternalProject_Add_StepDependencies(ffmpeg configure opus)

list(APPEND deps_libs
    ${external_lib_dir}/libavfilter.a
    ${external_lib_dir}/libavdevice.a
    ${external_lib_dir}/libavformat.a
    ${external_lib_dir}/libavcodec.a
    ${external_lib_dir}/libswresample.a
    ${external_lib_dir}/libswscale.a
    ${external_lib_dir}/libavutil.a
    ${external_lib_dir}/libmp3lame.a
    ${external_lib_dir}/libvorbis.a
    ${external_lib_dir}/libvorbisenc.a
    ${external_lib_dir}/libvorbisfile.a
    ${external_lib_dir}/libogg.a
    ${external_lib_dir}/libopus.a)
list(APPEND deps ffmpeg lame vorbis opus ogg)

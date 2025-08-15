#!/usr/bin/env bash

set -e

VERSION="4.8.3"
REV="1"
ARCH="amd64"
DIST="plucky"

LDIR="$(
    cd "$(dirname "$0")"
    pwd -P
)"
DS_DIR="dsnote_${VERSION}-${REV}+${DIST}_${ARCH}"
DS_DEB="${DS_DIR}/DEBIAN"
DS_DOC="${DS_DIR}/usr/share/doc/dsnote"
SOURCEDIR="dsnote-${VERSION}"
SOURCEFILE="v${VERSION}.tar.gz"
SOURCEURL="https://github.com/mkiol/dsnote/archive/refs/tags/${SOURCEFILE}"
SHA256SUM="da96b7f95a85d14e0e96d4f2ba5c00cde4c54b74d165bd50b7ebefb7bcbf814d"

[ -f "$SOURCEFILE" ] || wget -c -q --show-progress "$SOURCEURL"
echo "${SHA256SUM}  ${SOURCEFILE}" | sha256sum --check

[ -d "$SOURCEDIR" ] || tar xf "$SOURCEFILE"
cd "$SOURCEDIR"

mkdir -p build && cd build

CMAKE="-DCMAKE_BUILD_TYPE=Release -DWITH_DESKTOP=ON \
        -DWITH_PY=ON \
        -DBUILD_LIBARCHIVE=OFF \
        -DBUILD_FMT=OFF \
        -DBUILD_CATCH2=OFF \
        -DBUILD_OPENBLAS=OFF \
        -DBUILD_XZ=OFF \
        -DBUILD_PYBIND11=OFF \
        -DBUILD_RUBBERBAND=OFF \
        -DBUILD_FFMPEG=ON \
        -DBUILD_TAGLIB=OFF \
        -DBUILD_VOSK=OFF \
        -DBUILD_WHISPERCPP_VULKAN=ON \
        -DBUILD_QQC2_BREEZE_STYLE=ON \
        -DDOWNLOAD_VOSK=ON \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -Wno-dev"

# Build for CUDA if needed packages are found
dpkg -s nvidia-cuda-dev &> /dev/null && CUDA1=true || CUDA1=false
dpkg -s nvidia-cuda-toolkit &> /dev/null && CUDA2=true || CUDA2=false
$CUDA1 && $CUDA2 && HAS_CUDA=true || HAS_CUDA=false
if [[ $HAS_CUDA = true ]]; then
    CMAKE+=" -DBUILD_WHISPERCPP_CUBLAS=ON -DCMAKE_CUDA_ARCHITECTURES=native"
    export NVCC_APPEND_FLAGS="-std=c++17 -Wno-deprecated-gpu-targets"
fi

TEST_BUILD=false
# Disable bergamot and RHVoice (shorter build time - for test only)
$TEST_BUILD && CMAKE+=" -DBUILD_BERGAMOT=OFF -DBUILD_RHVOICE=OFF -DBUILD_RHVOICE_MODULE=OFF"

cmake ../ $CMAKE

test $(nproc) -gt 2 && make -j$(($(nproc)-2)) || make

mkdir -p "$DS_DEB"

make DESTDIR="$DS_DIR" install

cp -av "${LDIR}/debian/control" "$DS_DEB"
cp -av "${LDIR}/debian/postinst" "$DS_DEB"

cp -av "${LDIR}/debian/changelog" changelog.Debian
gzip --best -n changelog.Debian
mkdir -p "$DS_DOC"
mv changelog.Debian.gz "$DS_DOC"

cp -av "${LDIR}/debian/copyright" "$DS_DOC"

dpkg-deb --root-owner-group --build "$DS_DIR"

mv "${DS_DIR}.deb" ../../

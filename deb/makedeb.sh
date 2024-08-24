#!/usr/bin/env bash

set -e

VERSION="4.6.1"
REV="1"
ARCH="amd64"
LDIR="$(
    cd "$(dirname "$0")"
    pwd -P
)"
DS_DIR="dsnote_${VERSION}-${REV}_${ARCH}"
DS_DEB="${DS_DIR}/DEBIAN"
DS_DOC="${DS_DIR}/usr/share/doc/dsnote"
SOURCEDIR="dsnote-${VERSION}"
SOURCEFILE="v${VERSION}.tar.gz"
SOURCEURL="https://github.com/mkiol/dsnote/archive/refs/tags/${SOURCEFILE}"
SHA256SUM="301ec08dff6afa8ea321c74fc25aa1b42423d3d2ee1da840ac80b40e391332b3"

wget -c -q --show-progress "$SOURCEURL"
echo "${SHA256SUM}  ${SOURCEFILE}" | sha256sum --check

tar xf "$SOURCEFILE"
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
        -DBUILD_QQC2_BREEZE_STYLE=ON \
        -DDOWNLOAD_VOSK=ON \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -Wno-dev"

# Do not build for CUDA if needed packages are not found
dpkg -s nvidia-cuda-dev &> /dev/null && CUDA1=true || CUDA1=false
dpkg -s nvidia-cuda-toolkit &> /dev/null && CUDA2=true || CUDA2=false
$CUDA1 && $CUDA2 || CMAKE+=" -DBUILD_WHISPERCPP_CUBLAS=OFF"

# Do not build for HIP if needed package is not found
dpkg -s rocm-hip-sdk &> /dev/null && ROCM=true || ROCM=false
$ROCM || CMAKE+=" -DBUILD_WHISPERCPP_HIPBLAS=OFF"

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

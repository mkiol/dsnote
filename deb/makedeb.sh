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
SOURCEDIR=".."

# Build from current repository source
cd "$SOURCEDIR"

mkdir -p build && cd build

CMAKE="-DCMAKE_BUILD_TYPE=Release -DWITH_DESKTOP=ON \
        -DWITH_PY=ON \
        -DBUILD_LIBARCHIVE=ON \
        -DBUILD_FMT=ON \
        -DBUILD_CATCH2=ON \
        -DBUILD_OPENBLAS=ON \
        -DBUILD_XZ=ON \
        -DBUILD_PYBIND11=ON \
        -DBUILD_RUBBERBAND=ON \
        -DBUILD_FFMPEG=ON \
        -DBUILD_TAGLIB=ON \
        -DBUILD_VOSK=OFF \
        -DBUILD_WHISPERCPP_VULKAN=ON \
        -DBUILD_QQC2_BREEZE_STYLE=OFF \
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

# Create wrapper script to activate venv
mkdir -p "$DS_DIR/usr/bin"
mv "$DS_DIR/usr/bin/dsnote" "$DS_DIR/usr/bin/dsnote.bin"
cat > "$DS_DIR/usr/bin/dsnote" << 'EOF'
#!/bin/bash
VENV_DIR="/opt/dsnote/venv"
if [ -d "$VENV_DIR" ]; then
    export PYTHONPATH="$VENV_DIR/lib/python3.12/site-packages:$PYTHONPATH"
    export PATH="$VENV_DIR/bin:$PATH"
fi
exec /usr/bin/dsnote.bin "$@"
EOF
chmod +x "$DS_DIR/usr/bin/dsnote"

cp -av "${LDIR}/debian/control" "$DS_DEB"
cp -av "${LDIR}/debian/postinst" "$DS_DEB"

cp -av "${LDIR}/debian/changelog" changelog.Debian
gzip --best -n changelog.Debian
mkdir -p "$DS_DOC"
mv changelog.Debian.gz "$DS_DOC"

cp -av "${LDIR}/debian/copyright" "$DS_DOC"

dpkg-deb --root-owner-group --build "$DS_DIR"

mv "${DS_DIR}.deb" "${LDIR}/"

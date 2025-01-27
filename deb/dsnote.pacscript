maintainer=("LFdev <lfdev+deb@envs.net>")
compatible=('*:oracular' '*:24.10')

_pkgname='dsnote'
pkgname="${_pkgname}"
pkgver=4.7.1
pkgrel=1
pkgdesc="Note taking, reading and translating with offline Speech to Text, Text to Speech and Machine Translation"
arch=(
    'amd64'
)
depends=(
    'breeze-icon-theme'
    'hicolor-icon-theme'
    'libarchive13t64'
    'libc6'
    'libfmt9'
    'libkf5dbusaddons5'
    'liblzma5'
    'libopenblas0'
    'libpython3.12t64'
    'libqt5multimedia5'
    'libqt5multimedia5-plugins'
    'libqt5quickcontrols2-5'
    'libqt5x11extras5'
    'librubberband2'
    'libtag1v5'
    'libvulkan1'
    'libxinerama1'
    'libxtst6'
    'ocl-icd-libopencl1'
    'perl'
    'python3'
#    'qml-module-org-kde-qqc2breezestyle'
    'qml-module-org-kde-qqc2desktopstyle'
    'qml-module-org-kde-quickcharts'
    'qml-module-qtquick-controls'
    'qml-module-qtquick-dialogs'
    'zlib1g'
)
makedepends=(
    'appstream'
    'autoconf'
    'build-essential'
    'cmake'
    'extra-cmake-modules'
    'git'
    'kirigami2-dev'
    'libarchive-dev'
    'libboost-all-dev'
    'libfmt-dev'
    'libkf5dbusaddons-dev'
    'libkf5qqc2desktopstyle-dev'
    'libkf5iconthemes-dev'
    'liblzma-dev'
    'libopenblas-dev'
    'libpulse-dev'
    'libqt5x11extras5-dev'
    'librubberband-dev'
    'libtag1-dev'
    'libtool'
    'libtool-bin'
    'libvulkan-dev'
    'libxinerama-dev'
    'libxkbcommon-x11-dev'
    'libxtst-dev'
    'ocl-icd-opencl-dev'
    'patchelf'
    'pybind11-dev'
    'python3-dev'
    'qtbase5-dev'
    'qtdeclarative5-dev'
    'qtmultimedia5-dev'
    'qtquickcontrols2-5-dev'
    'qttools5-dev'
    'zlib1g-dev'
    'nvidia-cuda-dev'     # Support for GPU acceleration on NVidia GPU
    'nvidia-cuda-toolkit' # Support for GPU acceleration on NVidia GPU
)
optdepends=(
    'intel-opencl-icd: Support for Intel GPU acceleration with OpenCL'
    'libcudart12: Support for GPU acceleration on NVidia GPU'
    'mesa-vulkan-drivers: Vulkan support'
    'nvidia-cudnn: Support for GPU acceleration on NVidia GPU'
)

conflicts=(${_pkgname}-git)
url="https://github.com/mkiol/dsnote"
license=('MPL-2.0')

source=(https://github.com/mkiol/dsnote/archive/refs/tags/v${pkgver}.tar.gz)
sha256sums=('6b3c5029b32b272bd5238fb830c3aadce0cec438f7bece13bb304a7e52676b1b')

build() {
    cd "${srcdir}/${_pkgname}-${pkgver}"
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
    $CUDA1 && $CUDA2 && CMAKE+=" -DBUILD_WHISPERCPP_CUBLAS=ON -DCMAKE_CUDA_ARCHITECTURES=native"

    TEST_BUILD=true
    # Disable bergamot and RHVoice (shorter build time - for test only)
    $TEST_BUILD && CMAKE+=" -DBUILD_BERGAMOT=OFF -DBUILD_RHVOICE=OFF -DBUILD_RHVOICE_MODULE=OFF"

    cmake ../ $CMAKE

    test $(nproc) -gt 2 && make -j$(($(nproc)-2)) || make
}

package() {
    cd "${srcdir}/${_pkgname}-${pkgver}/build"
    
    make DESTDIR="${pkgdir}/" install

    # Install LICENSE from source root
    install -d "${pkgdir}/usr/share/doc/dsnote/"
    install -Dm644 "${srcdir}/${_pkgname}-${pkgver}/LICENSE" "${pkgdir}/usr/share/doc/dsnote/"
    
    # Start the app using X even on Wayland so all features work
    sed -i "s/Exec=dsnote/Exec=env QT_QPA_PLATFORM=xcb dsnote/" \
        "${pkgdir}/usr/share/applications/dsnote.desktop"
}

post_install() {
    echo "** Some features of Speech Note may require optional Python packages"
    echo "** Please check the help or project page for more info"
}

# vim: set sw=4 expandtab:

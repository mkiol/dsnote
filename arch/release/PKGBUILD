# Maintainer: LFdev <lfdev+aur at envs dot net>

_pkgname='dsnote'
pkgname="${_pkgname}"
pkgver=4.8.3
pkgrel=1
pkgdesc="Note taking, reading and translating with offline Speech to Text, Text to Speech and Machine Translation"
arch=(
  'x86_64'
  'aarch64'
)
url="https://github.com/mkiol/dsnote"
license=('MPL2')
depends=(
  'boost-libs'
  'fmt'
  'glslang'
  'hicolor-icon-theme'
  'kconfigwidgets5'
  'kdbusaddons5'
  'kiconthemes5'
  'kirigami2'
  'libarchive'
  'libpulse'
  'libxinerama'
  'libxtst'
  'ocl-icd'
  'openblas'
  'python>=3.11'
  'qt5-base'
  'qt5-declarative'
  'qt5-multimedia'
  'qt5-quickcontrols2'
  'qt5-x11extras'
  'rubberband'
  'taglib'
  'vulkan-icd-loader'
  'wayland'
  'xz'
)
makedepends=(
  'boost'
  'cmake'
  'extra-cmake-modules'
  'git'
  'meson'
  'patchelf'
  'pybind11'
  'qt5-tools'
  'vulkan-headers'
)
makedepends_x86_64=(
#  'cuda'
)
optdepends=(
  'python-accelerate: Support for Punctuation and Hebrew Diacritics restoration'
  'python-torchaudio: Support for Coqui TTS models'
  'python-transformers: Support for Punctuation and Hebrew Diacritics restoration'
  'ydotool: Support for inserting text into active window in Wayland'
)
optdepends_x86_64=(
  'amdvlk: Vulkan support for AMD GPU (AMDVLK Open)'
  'coqui-tts: Support for Coqui TTS models'
  'cuda: Support for GPU acceleration on NVidia graphic cards'
  'cudnn: Support for GPU acceleration on NVidia graphic cards'
  'nvidia-utils: Vulkan support for NVidia GPU'
  'python-faster-whisper: Support for FasterWhisper TTS models'
  'vulkan-intel: Vulkan support for Intel GPU'
  'vulkan-radeon: Vulkan support for AMD RADV'
)
options=('!debug')
provides=(${_pkgname})
conflicts=(${_pkgname}-git)
install=${_pkgname}.install
source=(https://github.com/mkiol/dsnote/archive/refs/tags/v${pkgver}.tar.gz)
sha256sums=('da96b7f95a85d14e0e96d4f2ba5c00cde4c54b74d165bd50b7ebefb7bcbf814d')

build() {
  cd "${srcdir}/${_pkgname}-${pkgver}"
  mkdir -p build
  cd build

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

  #################### !! IMPORTANT!! ######################
  # CUDA support is experimental and may break!
  # In order to enable support for it, please uncomment
  # the respective entry in makedepends_x86_64 (or install
  # the dependency package first) *before* making the build.
  # You can disable CUDA support by changing FULL_BUILD
  # below to false before building as well.
  ##########################################################

  FULL_BUILD=true

  if [[ "${CARCH}" == "x86_64" ]]; then
    pacman -Qi cuda &> /dev/null && HAS_CUDA=true || HAS_CUDA=false

    if [[ $HAS_CUDA = true ]] && [[ $FULL_BUILD = true ]]; then
      CMAKE+=" -DBUILD_WHISPERCPP_CUBLAS=ON -DCMAKE_CUDA_ARCHITECTURES=native"
      export NVCC_APPEND_FLAGS="-std=c++17 --compress-mode=size --split-compile=0 -Wno-deprecated-gpu-targets"
    fi
  fi

  CI_BUILD=false

  # Disable bergamot and RHVoice (used for CI builds only)
  $CI_BUILD && CMAKE+=" -DBUILD_BERGAMOT=OFF -DBUILD_RHVOICE=OFF -DBUILD_RHVOICE_MODULE=OFF"

  cmake ../ $CMAKE

  test $(nproc) -gt 2 && make -j$(($(nproc)-2)) || make
}

package() {
  cd "${srcdir}/${_pkgname}-${pkgver}"
  cd build
  make DESTDIR="$pkgdir" install
}

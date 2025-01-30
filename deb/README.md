### Build dependencies on Ubuntu

```
$ sudo apt install appstream autoconf build-essential cmake extra-cmake-modules \
    git kirigami2-dev libarchive-dev libboost-all-dev libfmt-dev \
    libkf5dbusaddons-dev libkf5qqc2desktopstyle-dev libkf5iconthemes-dev \
    liblzma-dev libopenblas-dev libpulse-dev libqt5x11extras5-dev \
    librubberband-dev libtag1-dev libtool libtool-bin libvulkan-dev \
    libxinerama-dev libxkbcommon-x11-dev libxtst-dev \
    ocl-icd-opencl-dev patchelf pybind11-dev python3-dev qtbase5-dev \
    qtdeclarative5-dev qtmultimedia5-dev qtquickcontrols2-5-dev qttools5-dev \
    zlib1g-dev
```

For NVidia CUDA:
```
$ sudo apt install nvidia-cuda-dev nvidia-cuda-toolkit nvidia-cudnn
```

Finally, run:
```
$ ./makedeb.sh
```

Then install the .deb file created in the current directory:
```
sudo apt install dsnote_*_amd64.deb
```

PS: Support for GPU acceleration is experimental and may break. The build is tested on Ubuntu 24.10 only, but may work with the current LTS (24.04) as well (if it does not work, try removing the CUDA packages to build without CUDA support).


### Build dependencies on Ubuntu 24.04

```
$ sudo apt appstream autoconf build-essential cmake extra-cmake-modules \
    git kirigami2-dev libarchive-dev libboost-all-dev libfmt-dev \
    libkf5dbusaddons-dev libkf5qqc2desktopstyle-dev libkf5iconthemes-dev \
    liblzma-dev libopenblas-dev libpulse-dev libqt5x11extras5-dev \
    librubberband-dev libtag1-dev libtool libxkbcommon-x11-dev \
    ocl-icd-opencl-dev patchelf pybind11-dev python3-dev qtbase5-dev \
    qtdeclarative5-dev qtmultimedia5-dev qtquickcontrols2-5-dev qttools5-dev \
    zlib1g-dev
```

For NVidia CUDA:
```
$ sudo apt install nvidia-cuda-dev nvidia-cuda-toolkit nvidia-cudnn
```

For AMD, follow [this guide](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/how-to/native-install/ubuntu.html) and add the repository for Ubuntu 22.04 (`jammy`) and:
```
$ sudo apt install rocm-hip-sdk rocm-opencl-runtime
```

Finally, run:
```
$ ./makedeb.sh
```

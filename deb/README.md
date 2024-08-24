### Build dependencies on Ubuntu

```
$ sudo apt install appstream autoconf build-essential cmake extra-cmake-modules \
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

For AMD, follow [this guide](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/install/native-install/ubuntu.html) and add the repositories for your Ubuntu version (as of now, there is support for 22.04/`jammy` and 24.04/`noble`), then install the necessary packages to build:
```
$ sudo apt install rocm-hip-sdk rocm-opencl-runtime
```

Finally, run:
```
$ ./makedeb.sh
```

PS: Support for GPU acceleration is experimental and may break. The build is tested on Ubuntu 24.04 only, but may work with older versions (if it does not work, try removing the CUDA/ROCm packages to build without GPU support).


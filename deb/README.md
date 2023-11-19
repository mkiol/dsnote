### Build dependencies on Ubuntu 23.10

```
$ sudo apt install build-essential cmake git autoconf libtool appstream qtbase5-dev \
    qttools5-dev zlib1g-dev qtdeclarative5-dev qtmultimedia5-dev qtquickcontrols2-5-dev \
    python3-dev libboost-all-dev libqt5x11extras5-dev libxkbcommon-x11-dev \
    extra-cmake-modules libkf5dbusaddons-dev ocl-icd-opencl-dev libarchive-dev libfmt-dev \
    libopenblas-dev librubberband-dev libtag1-dev liblzma-dev pybind11-dev \
    libkf5qqc2desktopstyle-dev libkf5iconthemes-dev kirigami2-dev
```

For NVidia CUDA:
```
$ sudo apt install nvidia-cuda-dev nvidia-cuda-toolkit
```

For AMD, follow [this guide](https://rocmdocs.amd.com/en/latest/deploy/linux/quick_start.html) and add the repository for Ubuntu 22.04 (`jammy`) and:
```
$ sudo apt install rocm-hip-sdk
```

Finally, run:
```
$ ./makedeb.sh
```
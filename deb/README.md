### Build dependencies on Ubuntu

```
$ sudo apt install appstream autoconf build-essential cmake extra-cmake-modules \
    git kirigami2-dev libarchive-dev libboost-all-dev libfmt-dev \
    libkf5dbusaddons-dev libkf5qqc2desktopstyle-dev libkf5iconthemes-dev \
    liblzma-dev libopenblas-dev libpulse-dev libqt5x11extras5-dev \
    librubberband-dev libtag1-dev libtool-bin libwayland-dev libxinerama-dev \
    libxkbcommon-x11-dev libxtst-dev ocl-icd-opencl-dev patchelf pybind11-dev \
    python3-dev qtbase5-dev qtbase5-private-dev qtdeclarative5-dev \
    qtmultimedia5-dev qtquickcontrols2-5-dev qttools5-dev zlib1g-dev
```

For NVidia CUDA:

```
$ sudo apt install nvidia-cuda-dev nvidia-cuda-toolkit nvidia-cudnn
```

For AMD, there is now built-in support for Vulkan that should work out of the box with all GPUs.

Finally, run:

```
$ ./makedeb.sh
```

Then install the .deb file created in the current directory:

```
$ sudo apt install dsnote_*_amd64.deb
```

PS: Support for CUDA acceleration is experimental and may break. The build is tested on Ubuntu 25.04 only, but may work with older versions (if it does not work, try removing the CUDA packages, if you added them, and build again). But the generated .deb probably will not install due to the required dependencies, you can try running the program with:

```
$ cd dsnote-'version'/build   # replace 'version' with the actual version number
$ LD_LIBRARY_PATH=./external/lib ./dsnote 
```

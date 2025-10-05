### Build dependencies on Ubuntu

**Note:** The project has migrated to Qt6. For older versions with Qt5, please check out an earlier release.

#### Qt6 Build (Current)

```bash
# First install OpenCL headers (required dependency)
sudo apt install opencl-headers

# Then install all build dependencies
sudo apt install appstream autoconf build-essential cmake \
    git libboost-all-dev libpulse-dev libwayland-dev libxinerama-dev \
    libxkbcommon-x11-dev libxtst-dev ocl-icd-opencl-dev patchelf \
    python3-dev qt6-base-dev qt6-declarative-dev qt6-multimedia-dev \
    qml6-module-qtquick qml6-module-qtquick-window \
    qml6-module-qtquick-controls qml6-module-qtquick-dialogs \
    qml6-module-qtquick-layouts qml6-module-qtqml-workerscript \
    qml6-module-qtquick-templates \
    zlib1g-dev

# Optional set: in case anything is missing/broken
sudo apt-get install \
    libtool libtool-bin automake \
    libnoise-dev meson \
    qt6-l10n-tools qt6-base-dev-tools \
    qt6-multimedia-dev qt6-declarative-dev qt6-tools-dev \
    catch2

```

**Note:** The makedeb.sh script now builds most dependencies statically (libarchive, libfmt, libopenblas, librubberband, libtag, etc.) to ensure compatibility across different Ubuntu versions.

For NVidia CUDA:

```bash
sudo apt install nvidia-cuda-dev nvidia-cuda-toolkit nvidia-cudnn
```

For AMD, there is now built-in support for Vulkan that should work out of the box with all GPUs.

Finally, run:

```bash
./makedeb.sh
```

Then install the .deb file created in the deb directory:

```bash
sudo dpkg -i dsnote_*_amd64.deb
sudo apt-get install -f  # Install any missing dependencies
```

PS: Support for CUDA acceleration is experimental and may break. The build is tested on Ubuntu 25.04 only, but may work with older versions (if it does not work, try removing the CUDA packages, if you added them, and build again). But the generated .deb probably will not install due to the required dependencies, you can try running the program with:

```bash
cd dsnote-'version'/build   # replace 'version' with the actual version number
LD_LIBRARY_PATH=./external/lib ./dsnote 
```

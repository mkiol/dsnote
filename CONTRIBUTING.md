# Contributing to Speech Note

Thank you for your interest in contributing to Speech Note! This document will help you get started with contributing to this open-source speech-to-text, text-to-speech, and translation application.

## Table of Contents

- [Ways to Contribute](#ways-to-contribute)
- [Getting Started](#getting-started)
- [Project Structure](#project-structure)
- [Development Setup](#development-setup)
- [Pull/Merge request](#pull-merge-request)
- [Code Style Guidelines](#code-style-guidelines)
- [Testing](#testing)
- [Translation](#translation)
- [Getting Help](#getting-help)

## Ways to Contribute

There are many ways to contribute to Speech Note:

- **Code contributions**: Bug fixes, new features, performance improvements
- **Translation**: Help translate the app into your language
- **Documentation**: Improve README, add examples, write tutorials
- **Bug reports**: Report issues you encounter
- **Feature requests**: Suggest new features or improvements
- **Testing**: Test beta releases and provide feedback
- **Reviews**: Review pull/merge requests

## Getting Started

Speech Note is hosted on both [GitHub](https://github.com/mkiol/dsnote) and [GitLab](https://gitlab.com/mkiol/dsnote). You can contribute on either platform - choose the one you're most comfortable with.

### Prerequisites

Before you start, you should have:

- Basic knowledge of C++ and/or QML (for code contributions)
- Familiarity with Git and GitHub/GitLab workflows
- A development environment set up (see [Development Setup](#development-setup))

## Project Structure

Understanding the project structure will help you navigate the codebase:

```text
dsnote/
├── src/                    # C++ backend source code
│   ├── *_engine.cpp/hpp    # Speech/TTS/translation engines
│   ├── dsnote_app.cpp/h    # Main application logic
│   ├── settings.cpp/h      # Settings management
│   └── ...                 # Other core components
├── desktop/                # Desktop UI files
│   └── qml/                # QML files for desktop UI
├── sfos/                   # Sailfish OS UI files
│   └── qml/                # QML files for Sailfish OS UI
├── translations/           # Translation files (*.ts)
├── tests/                  # Unit tests
├── cmake/                  # CMake build scripts
├── flatpak/               # Flatpak build configurations
├── arch/                  # Arch Linux PKGBUILD files
├── fedora/                # Fedora/RHEL spec files
├── config/                # Configuration files (e.g., models.json)
├── tools/                 # Build helper scripts
└── CMakeLists.txt         # Main CMake configuration
```

### Key Components

- **Speech Engines**: Located in `src/*_engine.cpp/hpp` files
  - STT engines: whisper, vosk, coqui, fasterwhisper, april-asr
  - TTS engines: piper, espeak, rhvoice, coqui, mimic3, etc.
  - Translation: bergamot
- **UI Layer**: QML files in `desktop/qml/` (Desktop) and `sfos/qml/` (Sailfish OS)
- **Application Core**: `src/dsnote_app.cpp` and `src/speech_service.cpp`
- **Settings**: `src/settings.cpp` handles all application settings

## Development Setup

### Building from Source

Speech Note uses CMake as its build system and has many dependencies. The recommended way to build is using the Flatpak toolchain, but direct builds are also possible.

#### Arch Linux

The easiest way to build on Arch Linux:

```bash
git clone https://github.com/mkiol/dsnote.git  # or GitLab URL
cd dsnote/arch/git
makepkg -si
```

This will build and install the latest git version with all dependencies.

For more information, see the [arch/git/PKGBUILD](arch/git/PKGBUILD) file.

#### Fedora/RHEL/Rocky Linux

```bash
git clone https://github.com/mkiol/dsnote.git
cd dsnote/fedora

# Install build dependencies (optional but recommended)
dnf install rpmdevtools autoconf automake boost-devel cmake git \
    kf5-kdbusaddons-devel libarchive-devel libxdo-devel \
    libXinerama-devel libxkbcommon-x11-devel libXtst-devel \
    libtool meson openblas-devel patchelf pybind11-devel \
    python3-devel python3-pybind11 qt6-linguist \
    qt6-qtmultimedia-devel qt6-qtquickcontrols2-devel \
    qt6-qtx11extras-devel rubberband-devel taglib-devel \
    vulkan-headers

./make_rpm.sh
```

For more information, see the [fedora/make_rpm.sh](fedora/make_rpm.sh) and [fedora/dsnote.spec](fedora/dsnote.spec) files.

#### Ubuntu/Debian/Mint

**Note:** The project has migrated to Qt6. For older versions with Qt5, please check out an earlier release.

```bash
# First install OpenCL headers (required dependency)
sudo apt install opencl-headers

# Then install all build dependencies
sudo apt install appstream autoconf build-essential cmake \
    git libboost-all-dev libpulse-dev libwayland-dev libxinerama-dev \
    libxkbcommon-x11-dev libxtst-dev ocl-icd-opencl-dev patchelf \
    python3-dev qt6-base-dev qt6-declarative-dev qt6-multimedia-dev \
    qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs \
    zlib1g-dev
```

**Optional - For NVIDIA CUDA acceleration (experimental):**

```bash
sudo apt install nvidia-cuda-dev nvidia-cuda-toolkit nvidia-cudnn
```

**Note:** CUDA support is experimental and tested only on Ubuntu 25.04. AMD GPUs have built-in Vulkan support that should work out of the box.

**Build the .deb package:**

```bash
git clone https://github.com/mkiol/dsnote.git
cd dsnote/deb
./makedeb.sh
```

**Install the .deb package:**

```bash
sudo dpkg -i dsnote_*_amd64.deb
sudo apt-get install -f  # Install any missing dependencies
```

**Alternative - Running without installing (if .deb installation fails):**

```bash
cd dsnote-<version>/build   # replace <version> with the actual version number
LD_LIBRARY_PATH=./external/lib ./dsnote
```

For more detailed information, see [deb/README.md](deb/README.md).

#### Flatpak (Recommended for Development)

```bash
git clone https://github.com/mkiol/dsnote.git
cd dsnote/flatpak

# Build base package
flatpak-builder --force-clean --user \
    --install-deps-from=flathub \
    --repo=<local-repo-name> \
    build-dir \
    net.mkiol.SpeechNote.yaml
```

For more information, see the [flatpak/net.mkiol.SpeechNote.yaml](flatpak/net.mkiol.SpeechNote.yaml) file.

#### Direct Build (Advanced)

You can do a direct build without packaging.
**Note**: Direct builds require many dependencies to be pre-installed. Check `CMakeLists.txt`, or the next section, for build options.

```bash
git clone https://github.com/mkiol/dsnote.git
cd dsnote
mkdir build && cd build

# Basic desktop build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DWITH_DESKTOP=ON

# Build without Python components (faster)
cmake ../ -DCMAKE_BUILD_TYPE=Release -DWITH_DESKTOP=ON -DWITH_PY=OFF

make -j$(nproc)
```

### Build Options

Key CMake options you can use:

- `-DWITH_DESKTOP=ON/OFF` - Enable desktop UI
- `-DWITH_SFOS=ON/OFF` - Enable Sailfish OS UI
- `-DWITH_PY=ON/OFF` - Enable Python libraries support
- `-DWITH_TESTS=ON/OFF` - Build tests
- `-DWITH_FLATPAK=ON/OFF` - Flatpak-specific build

See `CMakeLists.txt` for all available options.

### Launching the Application

After building, you can launch the application in different ways depending on your build method:

#### From Direct Build

```bash
# From the build directory
./dsnote

# Or with verbose logging for debugging
./dsnote --verbose
```

#### From Flatpak Build

```bash
# Install the built Flatpak locally
flatpak --user install <local-repo-name> net.mkiol.SpeechNote

# Run the installed Flatpak
flatpak run net.mkiol.SpeechNote

# Or with verbose logging
flatpak run net.mkiol.SpeechNote --verbose
```

#### From Package Build (Arch/Fedora/Debian)

If you built and installed via `makepkg -si`, `make_rpm.sh`, or `makedeb.sh` + `dpkg -i`, the application will be installed system-wide:

```bash
# Launch from application menu or command line
dsnote

# Or with verbose logging
dsnote --verbose
```

#### Alternative - Running Without Installing (Debian/Ubuntu)

If the .deb package installation fails due to dependency issues, you can run the application directly from the build directory:

```bash
cd dsnote-<version>/build   # replace <version> with the actual version number
LD_LIBRARY_PATH=./external/lib ./dsnote

# Or with verbose logging
LD_LIBRARY_PATH=./external/lib ./dsnote --verbose
```

## Pull-Merge request

- Write your code following the [Code Style Guidelines](#code-style-guidelines)
- Add tests if applicable
- Update documentation if needed
- Test your changes thoroughly

## Code Style Guidelines

### C++ Code Style

Speech Note uses `.clang-format` for C++ code formatting (based on Google style).

- Format your code before committing:

  ```bash
  clang-format -i src/your_file.cpp
  ```

- Follow existing code patterns in the project
- Use descriptive variable and function names
- Add comments for complex logic
- Prefer RAII and modern C++ features

### QML Code Style

- Use 4 spaces for indentation
- Follow Qt QML coding conventions
- Keep components modular and reusable
- Use property bindings when appropriate
- Add comments for complex UI logic

### General Guidelines

- **Keep changes focused**: One PR/MR should address one issue or feature
- **Write clean code**: Easy to read and maintain
- **Document public APIs**: Add documentation for public functions/classes
- **Handle errors gracefully**: Don't crash on unexpected input
- **Be consistent**: Follow the existing code style

## Testing

### Running Tests

Speech Note includes unit tests in the `tests/` directory:

```bash
# Ensure Qt6 tools are in PATH
export PATH="/usr/lib/qt6/bin:$PATH"

# Configure build with tests enabled
mkdir -p build && cd build
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DWITH_TESTS=ON -DWITH_DESKTOP=ON -DBUILD_CATCH2=OFF

# Build the project
make

# Run tests
ctest --output-on-failure
```

**Note**: We use `-DBUILD_CATCH2=OFF` to use the system-installed Catch2 package instead of downloading it.

### Writing Tests

- Add tests for new features
- Tests use the Catch2 framework
- Place tests in `tests/` directory
- Follow existing test patterns

### Manual Testing

Before submitting:

1. Build and run the application
2. Test your changes thoroughly
3. Test on different platforms if possible
4. Check for memory leaks (use valgrind or similar)
5. Ensure no regressions in existing functionality

## Translation

Speech Note supports multiple languages. Translation is done via Qt's translation system.

### Via Transifex (Recommended)

The preferred way to contribute translations is through [Transifex](https://explore.transifex.com/mkiol/dsnote/):

1. Create a Transifex account
2. Join the Speech Note project
3. Select your language (or request a new one)
4. Translate strings in the web interface

### Direct Translation

If you prefer to work directly with `.ts` files:

1. Translation files are in `translations/` directory
2. File format: `dsnote-{language_code}.ts` (e.g., `dsnote-fr.ts`)
3. Use Qt Linguist to edit `.ts` files:

   ```bash
   linguist translations/dsnote-fr.ts
   ```

4. Submit a PR/MR with your updated `.ts` file

### Adding a New Language

1. Copy an existing `.ts` file or generate a new one
2. Translate all strings
3. Update the build configuration if needed
4. Submit a PR/MR

## Getting Help

### Questions and Discussions

- **GitHub Issues**: <https://github.com/mkiol/dsnote/issues>
- **GitLab Issues**: <https://gitlab.com/mkiol/dsnote/-/issues>
- Check existing issues before creating a new one
- For questions, create an issue with the "question" label

### Reporting Bugs

When reporting a bug, include:

1. **Description**: What went wrong?
2. **Steps to reproduce**: How to trigger the bug?
3. **Expected behavior**: What should happen?
4. **Actual behavior**: What actually happens?
5. **Environment**:
   - OS and version
   - Application version
   - Installation method (Flatpak, AUR, etc.)
6. **Logs**: Include relevant error messages or logs

### Feature Requests

When suggesting a feature:

1. **Use case**: Why is this feature needed?
2. **Description**: What should it do?
3. **Alternatives**: Any alternative solutions you've considered?
4. **Additional context**: Screenshots, mockups, etc.

## Code of Conduct

- Be respectful and inclusive
- Welcome newcomers and help them learn
- Focus on constructive feedback
- Give credit where it's due
- Follow the project's technical decisions

## License

By contributing to Speech Note, you agree that your contributions will be licensed under the [Mozilla Public License Version 2.0](LICENSE).

---

Thank you for contributing to Speech Note! Your help makes this project better for everyone. 🎉

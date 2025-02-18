Name:           %{?NAME}%{!?NAME:dsnote}
Version:        %{?VERSION}%{!?VERSION:%(date +%%Y%%m%%d)}
Release:        1%{?dist}
Summary:        Speech Note: Take, read and translate notes in multiple languages

License:        MPL-2.0
URL:            https://github.com/mkiol/dsnote
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  boost-devel
BuildRequires:  cmake
BuildRequires:  git
BuildRequires:  kf5-kdbusaddons-devel
BuildRequires:  libarchive-devel
BuildRequires:  libxdo-devel
BuildRequires:  libXinerama-devel
BuildRequires:  libxkbcommon-x11-devel
BuildRequires:  libXtst-devel
BuildRequires:  libtool
BuildRequires:  meson
BuildRequires:  openblas-devel
BuildRequires:  patchelf
BuildRequires:  pybind11-devel
BuildRequires:  python3-devel
BuildRequires:  python3-pybind11
BuildRequires:  qt5-linguist
BuildRequires:  qt5-qtmultimedia-devel
BuildRequires:  qt5-qtquickcontrols2-devel
BuildRequires:  qt5-qtx11extras-devel
BuildRequires:  rubberband-devel
BuildRequires:  taglib-devel
BuildRequires:  vulkan-headers
Requires:       kf5-kquickcharts
Requires:       kf5-sonnet
Requires:       python3
Requires:       (qqc2-breeze-style < 6 or kf5-qqc2-breeze-style)
Requires:       (qqc2-desktop-style < 6 or kf5-qqc2-desktop-style)
Requires:       qt5-qtquickcontrols
Requires:       qt5-qtquickcontrols2
Requires:       vulkan-loader
Recommends:     ydotool

%description
Speech Note let you take, read and translate notes in multiple languages.
It uses Speech to Text, Text to Speech and Machine Translation to do so.
Text and voice processing take place entirely offline, locally on your computer,
without using a network connection. Your privacy is always respected.
No data is sent to the Internet.

%global debug_package %{nil}
%global source_date_epoch_from_changelog  %{nil}

%prep
%autosetup

%build
%cmake3 \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_DESKTOP=ON \
    -DBUILD_VOSK=OFF \
    -DDOWNLOAD_VOSK=ON \
    -DBUILD_LIBARCHIVE=OFF \
    -DBUILD_OPENBLAS=OFF \
    -DBUILD_RUBBERBAND=OFF \
    -DBUILD_TAGLIB=OFF \
    -DBUILD_XDO=OFF \
    -DBUILD_WHISPERCPP_VULKAN=ON \
    -DBUILD_XKBCOMMON=ON
%cmake_build

%install
rm -rf $RPM_BUILD_ROOT
%cmake_install

%files
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.*
%{_datadir}/metainfo/%{name}.metainfo.xml
%{_datadir}/dbus-1/services/dsnote.service

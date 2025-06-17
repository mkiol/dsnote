Name:       harbour-dsnote

# >> macros
%define __provides_exclude_from ^%{_datadir}/.*$
%define __requires_exclude ^libstt.*|libkenlm.*|libtensorflowlite.*|libtflitedelegates.*|libopenblas.*|libwhisper*|libbergamota*|libvosk.*|libonnxruntime.*|libRHVoice*|libaprilasr.*$
%define _unpackaged_files_terminate_build 0
# << macros

Summary:        Speech Note
Version:        4.8.0
Release:        1
Group:          Qt/Qt
License:        LICENSE
URL:            https://github.com/mkiol/dsnote
Source0:        %{name}-%{version}.tar.bz2
Requires:       sailfishsilica-qt5 >= 0.10.9
Requires:       python3-gobject
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(zlib)
BuildRequires:  pkgconfig(libpulse)
BuildRequires:  desktop-file-utils
BuildRequires:  curl
BuildRequires:  cmake
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  m4
BuildRequires:  patch
BuildRequires:  python3-pip
BuildRequires:  python3-devel
BuildRequires:  git
BuildRequires:  boost-devel
BuildRequires:  meson

%description
Speech Note let you take, read and translate notes in multiple languages.
It uses Speech to Text, Text to Speech and Machine Translation to do so.
Text and voice processing take place entirely offline, locally on your computer,
without using a network connection. Your privacy is always respected.
No data is sent to the Internet.

%prep
%setup -q -n %{name}-%{version}

# >> setup
# << setup

%build
# >> build pre
# << build pre
%cmake .
make %{?_smp_mflags}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%make_install

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

%post
systemctl-user stop %{name}.service >/dev/null 2>&1 || :
systemctl-user daemon-reload >/dev/null 2>&1 || :

%preun
systemctl-user stop %{name}.service >/dev/null 2>&1 || :
systemctl-user daemon-reload >/dev/null 2>&1 || :

%files
%defattr(-,root,root,-)
%{_bindir}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/dbus-1/services/org.mkiol.Speech.service
%{_datadir}/dbus-1/services/org.mkiol.dsnote.service
%{_userunitdir}/%{name}.service
# >> files
# << files

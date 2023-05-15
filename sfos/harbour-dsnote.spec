Name:       harbour-dsnote

# >> macros
%define __provides_exclude_from ^%{_datadir}/.*$
%define __requires_exclude ^libstt.*|libkenlm.*|libopenblas.*|libvosk.*|libonnxruntime.*$
%define _unpackaged_files_terminate_build 0
# << macros

Summary:        Note taking with speech to text
Version:        2.0.1
Release:        1
Group:          Qt/Qt
License:        LICENSE
URL:            https://github.com/mkiol/dsnote
Source0:        %{name}-%{version}.tar.bz2
Requires:       sailfishsilica-qt5 >= 0.10.9, qt5-qtmultimedia-plugin-mediaservice-gstaudiodecoder
Requires:       python3-gobject
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(zlib)
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

%description
Note taking with speech to text


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
%{_userunitdir}/%{name}.service
# >> files
# << files

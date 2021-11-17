Name:       harbour-dsnote

# >> macros
%define __provides_exclude_from ^%{_datadir}/.*$
%define __requires_exclude ^libstt.*|libtensorflowlite.*|libtflitedelegates.*|libkenlm.*$
# << macros

Summary:        Note taking with speech to text
Version:        1.5.1
Release:        1
Group:          Qt/Qt
License:        LICENSE
URL:            https://github.com/mkiol/dsnote
Source0:        %{name}-%{version}.tar.bz2
Requires:       sailfishsilica-qt5 >= 0.10.9, qt5-qtmultimedia-plugin-mediaservice-gstaudiodecoder
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(liblzma)
BuildRequires:  pkgconfig(zlib)
BuildRequires:  pkgconfig(libarchive)
BuildRequires:  desktop-file-utils

%description
Note taking with speech to text


%prep
%setup -q -n %{name}-%{version}

# >> setup
# << setup

%build
# >> build pre
# << build pre

%qmake5 

make %{?_smp_mflags}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%qmake5_install

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

%post
systemctl-user stop %{name}.service >/dev/null 2>&1 || :

%preun
systemctl-user stop %{name}.service >/dev/null 2>&1 || :

%files
%defattr(-,root,root,-)
%{_bindir}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/dbus-1/services/org.mkiol.Stt.service
%{_userunitdir}/%{name}.service
# >> files
# << files

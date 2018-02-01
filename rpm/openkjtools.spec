Name:           openkjtools
Version:        0.11.0 
Release:        1%{?dist}
Summary:        Tool for managing karoaoke files 

License:        GPL
URL:            https://karaokerg.org
Source0:        openkjtools-0.11.0.tar.bz2 

BuildRequires:  qt5-qtbase-devel
Requires:       qt5-qtbase mp3gain p7zip p7zip-plugins
Obsoletes:	karaokerg
%description
Simple GUI tool for managing karaoke files

%prep
%setup -q


%build
%{qmake_qt5} PREFIX=$RPM_BUILD_ROOT/usr
%make_build


%install
%make_install

%files
/usr/bin/OpenKJTools
/usr/share/applications/openkjtools.desktop
/usr/share/pixmaps/openkjtools.png

%changelog
* Wed Dec 20 2017 T. Isaac Lightburn <isaac@hozed.net>
- 

Name:           lipstick2vnc
Summary:        A VNC server for Mer QA
Version:        0.1
Release:        1
Group:          System Environment/Daemons
License:        GPLv2+
URL:            https://github.com/mer-qa/lipstick2vnc
Source0:        %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(libvncserver)
BuildRequires:  pkgconfig(libsystemd)
BuildRequires:  libjpeg-turbo-devel
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  qt5-qtwayland-wayland_egl-devel
BuildRequires:  oneshot
Requires:       oneshot
Requires:       jolla-sessions-qt5 >= 1.2.7
%{_oneshot_requires_post}


%description
A VNC server for Mer QA

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

%prep
%setup -q -n %{name}-%{version}


%build
%qtc_qmake5 CONFIG+=release
%qtc_make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%qmake5_install

# systemd integration
mkdir -p %{buildroot}/%{_lib}/systemd/system/multi-user.target.wants/
ln -s ../vnc.socket %{buildroot}/%{_lib}/systemd/system/multi-user.target.wants/vnc.socket

%post
systemctl daemon-reload

%{_bindir}/add-oneshot 20-lipstick2vnc-configurator

%preun
systemctl stop vnc.service
systemctl stop vnc.socket

%postun
systemctl daemon-reload

%files
%defattr(-,root,root,-)
%attr(755, root, privileged) %{_bindir}/%{name}
%attr(755, root, root) %{_oneshotdir}/20-lipstick2vnc-configurator
/%{_lib}/systemd/system/vnc.socket
/%{_lib}/systemd/system/vnc.service
/%{_lib}/systemd/system/multi-user.target.wants/vnc.socket

Name:           mervncserver
Summary:        A VNC server for Mer QA
Version:        0.1
Release:        1
Group:          System Environment/Daemons
License:        GPLv2+
URL:            https://github.com/mer-qa/mervncserver
Source0:        %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(libvncserver)
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  libjpeg-turbo-devel

%description
A VNC server for Mer QA

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}

%prep
%setup -q -n %{name}-%{version}


%build
%qtc_qmake5 
%qtc_make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%qmake5_install

# systemd integration
mkdir -p %{buildroot}/%{_lib}/systemd/system/multi-user.target.wants/
ln -s ../vnc.socket %{buildroot}/%{_lib}/systemd/system/multi-user.target.wants/vnc.socket

%post
systemctl daemon-reload

%preun
systemctl stop vnc.service
systemctl stop vnc.socket

%postun
systemctl daemon-reload

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
/%{_lib}/systemd/system/vnc.socket
/%{_lib}/systemd/system/vnc.service
/%{_lib}/systemd/system/multi-user.target.wants/vnc.socket
